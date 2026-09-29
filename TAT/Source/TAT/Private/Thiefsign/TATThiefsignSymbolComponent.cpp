// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignSymbolComponent.h"

// tat
#include "Thiefsign/TATThiefsignSettings.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/AssetManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefsignSymbolComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefsignSymbolComponent, Log, All);

UTATThiefsignSymbolComponent::UTATThiefsignSymbolComponent()
{
   // Component should only tick when animating the opacity or floating motion
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;

   // Will be created by gameplay cues on clients
   SetIsReplicatedByDefault(false);
}

#if WITH_EDITOR
EDataValidationResult UTATThiefsignSymbolComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!PlayerUnconsciousTag.IsValid())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] had no PlayerUnconsciousTag supplied!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void UTATThiefsignSymbolComponent::BeginPlay()
{
   Super::BeginPlay();

   if (PlayerUnconsciousTag.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         asc->RegisterGameplayTagEvent(PlayerUnconsciousTag).AddUObject(this, &UTATThiefsignSymbolComponent::_OnPlayerConditionChanged);
      }
      else
      {
         UE_LOG(LogTATThiefsignSymbolComponent, Error, TEXT("[%s] | Failed to find UAbilitySystemComponent to bind to player condition tag listener!")
            , *GetOwner()->GetName());
      }
   }
   else
   {
      UE_LOG(LogTATThiefsignSymbolComponent, Error, TEXT("[%s] | PlayerUnconsciousTag was not supplied!")
         , *GetOwner()->GetName());
   }
}

void UTATThiefsignSymbolComponent::EndPlay(EEndPlayReason::Type reason)
{
   if (PlayerUnconsciousTag.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         asc->RegisterGameplayTagEvent(PlayerUnconsciousTag).RemoveAll(this);
      }
   }

   // Ensure we clean up any listeners if we're being removed early
   UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::GetMutable();
   thiefsignSettings.UnregisterSymbolsLoadedDelegates(this);

   Super::EndPlay(reason);
}

void UTATThiefsignSymbolComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   check(IsValid(_dynamicMaterialInstance));

   float currentOpacity = -1.f;
   if (!_dynamicMaterialInstance->GetScalarParameterValue(MaterialOpacityParamName, currentOpacity))
   {
      UE_LOG(LogTATThiefsignSymbolComponent, Error, TEXT("[%s] | Failed to query opacity scalar parameter value! Ensure that the parameter data in _materialOpacityParam matches the material's opacity parameter")
         , *GetOwner()->GetName());
      PrimaryComponentTick.SetTickFunctionEnable(false);
      return;
   }

   // Have the billboard float upwards if not in screen space (i.e. first person)
   if (_ShouldApplyFloatingMotion())
   {
      AddRelativeLocation(FVector(0.0f, 0.0f, FloatUpSpeed) * deltaTime);
   }

   const float targetOpacity = _isVisible ? 1.0f : 0.0f;
   if (!FMath::IsNearlyEqual(currentOpacity, targetOpacity))
   {
      // Increase/decrease current opacity towards target
      float delta = deltaTime * (1.f / FMath::Max(OpacityFadeSeconds, 0.1f));
      if (!_isVisible)
      {
         delta *= -1.f;
      }
      currentOpacity = FMath::Clamp(currentOpacity + delta, 0.f, 1.f);
      _dynamicMaterialInstance->SetScalarParameterValue(MaterialOpacityParamName, currentOpacity);

      // Disable rendering when opacity reaches 0 (enable otherwise)
      const bool isVisible = currentOpacity != 0.f;
      SetVisibility(isVisible);
   }
   else if (!_ShouldApplyFloatingMotion())
   {
      // If we don't have to worry about motion, disable tick if opacity has been reached
      UE_LOG(LogTATThiefsignSymbolComponent, Verbose, TEXT("[%s] | Reached target opacity %f! Disabling tick..."), *GetOwner()->GetName(), currentOpacity);
      PrimaryComponentTick.SetTickFunctionEnable(false);
   }
}

void UTATThiefsignSymbolComponent::ConfigureSymbol(FGameplayTag thiefsignIdentifier, const FVector2D& size, bool isFirstPerson)
{
   SymbolMaterialElement.BaseSizeX = size.X;
   SymbolMaterialElement.BaseSizeY = size.Y;

   UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::GetMutable();
   const bool loadSymbolsIfUnloaded = true;
   if (thiefsignSettings.AreSymbolsLoaded(loadSymbolsIfUnloaded))
   {
      _isFirstPerson = isFirstPerson;

      const FTATThiefsignInfo* thiefsignInfo = thiefsignSettings.FindThiefsignInfo(thiefsignIdentifier);
      if (thiefsignInfo == nullptr)
      {
         UE_LOG(LogTATThiefsignSymbolComponent, Error, TEXT("[%s] Failed to find %s Thiefsign identifier!")
            , *GetOwner()->GetName(), *thiefsignIdentifier.ToString());

         // Destroy if we're unable to display the symbol
         _ReadyForDestruction();

         return;
      }

      FTATThiefsignVFXConfig vfxConfig = thiefsignSettings.HandVFXConfig;
      vfxConfig.Merge(thiefsignInfo->HandVFXConfigOverrides);

      _cachedMaterialParameters = vfxConfig.MaterialParams;

      TSoftObjectPtr<UMaterialInterface> material = vfxConfig.MaterialClass;
      if (!material.IsNull())
      {
         UAssetManager::GetStreamableManager().RequestAsyncLoad(vfxConfig.MaterialClass.ToSoftObjectPath(), [weakThis = MakeWeakObjectPtr(this), material]
         {
            if (weakThis.IsValid())
            {
               weakThis->_MaterialLoaded(material.Get());
            }
         });
      }
      else
      {
         // If a material wasn't provided, just go with whatever we already had
         _MaterialLoaded(SymbolMaterialElement.Material);
      }

      GetWorld()->GetTimerManager().SetTimer(_lifespanTimerHandle, this, &UTATThiefsignSymbolComponent::_OnLifespanTimeElapsed, thiefsignSettings.HandSymbolLifespan);
   }
   else
   {
      // Load the symbols table and then re-run this method
      FSimpleMulticastDelegate::FDelegate onLoadedCallback;
      onLoadedCallback.BindWeakLambda(this, [weakThis = MakeWeakObjectPtr(this), thiefsignIdentifier, size, isFirstPerson]
      {
         if (weakThis.IsValid())
         {
            weakThis->ConfigureSymbol(thiefsignIdentifier, size, isFirstPerson);
         }
      });
      thiefsignSettings.CallOrRegisterSymbolsLoadedDelegate(onLoadedCallback);
   }
}

UMaterialInstanceDynamic* UTATThiefsignSymbolComponent::GetOrCreateDynamicMaterialInstance()
{
   if (!IsValid(_dynamicMaterialInstance))
   {
      _dynamicMaterialInstance = ConstructDynamicMaterialInstance();
   }
   return _dynamicMaterialInstance;
}

UMaterialInstanceDynamic* UTATThiefsignSymbolComponent::ConstructDynamicMaterialInstance()
{
   // Make sure we haven't already constructed a material instance
   check(!IsValid(_dynamicMaterialInstance));

   if (!IsValid(SymbolMaterialElement.Material))
   {
      UE_LOG(LogTATThiefsignSymbolComponent, Warning, TEXT("[%s] Unassigned material! Could not construct dynamic material instance"), *GetOwner()->GetName());
      PrimaryComponentTick.SetTickFunctionEnable(false);
      return nullptr;
   }

   UE_LOG(LogTATThiefsignSymbolComponent, Verbose, TEXT("[%s] | Constructing dynamic material instance with material %s...")
      , *GetOwner()->GetName()
      , *SymbolMaterialElement.Material->GetName());

   UMaterialInstanceDynamic* dynamicMaterialInstance = CreateDynamicMaterialInstance(0, SymbolMaterialElement.Material);
   check(dynamicMaterialInstance != nullptr);

   _cachedMaterialParameters.ApplyToMaterial(dynamicMaterialInstance, _isFirstPerson, []()
   {
      UE_LOG(LogTATThiefsignSymbolComponent, Verbose, TEXT("Material params applied to dynamic material instance.!"));
   });

   // Init opacity to 0, so it can fade into visibility (rather than snapping on)
   dynamicMaterialInstance->SetScalarParameterValue(MaterialOpacityParamName, 0.0f);

   // Start ticking once the dynamic material instance is created
   PrimaryComponentTick.SetTickFunctionEnable(true);

   return dynamicMaterialInstance;
}

void UTATThiefsignSymbolComponent::_MaterialLoaded(UMaterialInterface* material)
{
   SymbolMaterialElement.Material = material;

   // Reset our elements after modifying _symbolMaterialElement
   TArray<FMaterialSpriteElement> materialElements;
   materialElements.Add(SymbolMaterialElement);
   SetElements(materialElements);

   // Set our visibility which will also create the dynamic material instance with the material just loaded
   _SetSymbolVisibility(true);
}

void UTATThiefsignSymbolComponent::_SetSymbolVisibility(bool visibility)
{
   if (visibility != _isVisible)
   {
      // Update visibility
      _isVisible = visibility;

      // Construct material instance if we haven't already
      if (!IsValid(_dynamicMaterialInstance))
      {
         _dynamicMaterialInstance = GetOrCreateDynamicMaterialInstance();
         if (!_dynamicMaterialInstance)
         {
            UE_LOG(LogTATThiefsignSymbolComponent, Error, TEXT("[%s] Failed to construct dynamic material instance!"), *GetOwner()->GetName());
            return;
         }
      }

      UE_LOG(LogTATThiefsignSymbolComponent, Verbose, TEXT("[%s] animating visibility to %s")
         , *GetOwner()->GetName()
         , visibility ? TEXT("visible") : TEXT("invisible"));

      // Make sure tick enabled to animate opacity fade
      PrimaryComponentTick.SetTickFunctionEnable(true);
   }
}

void UTATThiefsignSymbolComponent::_OnPlayerConditionChanged(const FGameplayTag tag, int32 newTagCount)
{
   // End the VFX lifespan early if the owning player is knocked out
   const bool tagAdded = newTagCount > 0;
   if (tagAdded)
   {
      GetWorld()->GetTimerManager().ClearTimer(_lifespanTimerHandle);
      _OnLifespanTimeElapsed();
   }
}

void UTATThiefsignSymbolComponent::_OnLifespanTimeElapsed()
{
   // Have the symbol fade out
   _SetSymbolVisibility(false);

   // Destroy this component shortly after the amount of time it takes to fade out
   GetWorld()->GetTimerManager().SetTimer(_destructionTimerHandle, this, &UTATThiefsignSymbolComponent::_ReadyForDestruction, OpacityFadeSeconds + 0.25f);
}

void UTATThiefsignSymbolComponent::_ReadyForDestruction()
{
   // Ensure the timer is cleared (if it was started) if we're being destroyed early
   GetWorld()->GetTimerManager().ClearTimer(_destructionTimerHandle);

   DestroyComponent();
}
