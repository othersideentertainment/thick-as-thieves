// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignDecalActor.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirementVisComponent.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Thiefsign/TATThiefsignTypes.h"
#include "Thiefsign/TATThiefsignSettings.h"

// ose
#include "Online/OSEGameState.h"
#include "Player/OSEPlayerState.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefsignDecalActor)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefsignDecalActor, Log, All);

ATATThiefsignDecalActor::ATATThiefsignDecalActor()
{
   _isMaterialLoaded = false;
   _isInstigatorLocal = false;
   _isThiefVisionEnabled = false;
   _isVisible = false;

   // We'll make a root scene component so the decal can be independently rotated and scaled
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent->SetMobility(EComponentMobility::Static);
   SetRootComponent(RootComponent);

   DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
   DecalComponent->SetupAttachment(RootComponent);

   const UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::Get();
   const float decalDepth = thiefsignSettings.DecalBoundingBoxDepth;
   DecalComponent->DecalSize = FVector(decalDepth, Size.X, Size.Y);

   // Can tick to animate opacity
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;

   // Spawned from server, replicate creation to clients
   bReplicates = true;
   SetReplicatingMovement(false);

   // Not visible until material is loaded
   SetActorHiddenInGame(true);

#if WITH_EDITORONLY_DATA
   SceneRequirementVisComponent = CreateEditorOnlyDefaultSubobject<UTATSceneRequirementVisComponent>(TEXT("SceneRequirementVisualizer"));
   ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
   SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

   if (!IsRunningCommandlet())
   {
      // Structure to hold one-time initialization
      struct FConstructorStatics
      {
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> DecalTexture;
         FName ID_Decals;
         FText NAME_Decals;
         FConstructorStatics()
            : DecalTexture(TEXT("/Engine/EditorResources/S_DecalActorIcon"))
            , ID_Decals(TEXT("Decals"))
            , NAME_Decals(NSLOCTEXT("SpriteCategory", "Decals", "Decals"))
         {
         }
      };
      static FConstructorStatics ConstructorStatics;

      if (ArrowComponent)
      {
         ArrowComponent->bTreatAsASprite = true;
         ArrowComponent->ArrowSize = 1.0f;
         ArrowComponent->ArrowColor = FColor(80, 80, 200, 255);
         ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Decals;
         ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Decals;
         ArrowComponent->SetupAttachment(RootComponent);
         ArrowComponent->SetUsingAbsoluteScale(true);
         ArrowComponent->bIsScreenSizeScaled = true;
      }

      if (SpriteComponent)
      {
         SpriteComponent->Sprite = ConstructorStatics.DecalTexture.Get();
         SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
         SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Decals;
         SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Decals;
         SpriteComponent->SetupAttachment(RootComponent);
         SpriteComponent->bIsScreenSizeScaled = true;
         SpriteComponent->SetUsingAbsoluteScale(true);
         SpriteComponent->bReceivesDecals = false;
      }
   }
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
EDataValidationResult ATATThiefsignDecalActor::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = UObject::IsDataValid(context);

   if (!ThiefsignIdentifier.IsValid())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no ThiefsignIdentifier assigned!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   float currentOpacity = -1.f;
   UMaterialInterface* decalMaterial = DecalComponent->GetDecalMaterial();
   if (decalMaterial == nullptr)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no material assigned to the decal component!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }
   else if (!decalMaterial->GetScalarParameterValue(MaterialOpacityParamName, currentOpacity))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] could not find the %s param on the decal material!")
         , *GetName(), *MaterialOpacityParamName.ToString())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}

void ATATThiefsignDecalActor::PostActorCreated()
{
   Super::PostActorCreated();

   if (_IsEditorPreview())
   {
      // Load the decal material and apply any necessary parameters when placed into level
      _LoadMaterial();
   }
}
#endif // WITH_EDITOR

void ATATThiefsignDecalActor::BeginPlay()
{
   Super::BeginPlay();

   if (!_requirement.IsNone() && !UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _requirement))
   {
      // If we don't meet the scene requirement, destroy the actor
      GetOwner()->Destroy();
   }
   else
   {
      _isInstigatorLocal = (InstigatorPlayer != nullptr && InstigatorPlayer->IsLocalPlayerState());

      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         _UpdateThiefVisionEnabledCache(thiefVisionSubsystem->IsThiefVisionEnabledForLocalPlayer());
         thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.AddUniqueDynamic(this, &ATATThiefsignDecalActor::_OnLocalPlayerThiefVisionStatusChanged);
      }
      else
      {
         UE_LOG(LogTATThiefsignDecalActor, Error, TEXT("Failed to get ThiefVisionSubsystem!"));
      }

      // Load the decal material and apply any necessary parameters
      _LoadMaterial();
   }
}

void ATATThiefsignDecalActor::EndPlay(EEndPlayReason::Type reason)
{
   if (UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
   {
      thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.RemoveAll(this);
   }

   // Ensure we clean up any listeners if we're being removed early
   UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::GetMutable();
   thiefsignSettings.UnregisterSymbolsLoadedDelegates(this);

   Super::EndPlay(reason);
}

void ATATThiefsignDecalActor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   check(IsValid(_dynamicMaterialInstance));

   float currentOpacity = -1.f;
   if (!_dynamicMaterialInstance->GetScalarParameterValue(MaterialOpacityParamName, currentOpacity))
   {
      UE_LOG(LogTATThiefsignDecalActor, Error, TEXT("[%s] | Failed to query opacity scalar parameter value! Ensure that the parameter data in _materialOpacityParam matches the material's opacity parameter")
         , *GetOwner()->GetName());
      SetActorTickEnabled(false);
      return;
   }
   
   // Increase/decrease current opacity towards target
   float delta = deltaTime * (1.f / FMath::Max(OpacityFadeSeconds, 0.1f));
   if (!_isVisible)
   {
      delta *= -1.f;
   }
   currentOpacity = FMath::Clamp(currentOpacity + delta, 0.f, 1.f);
   _dynamicMaterialInstance->SetScalarParameterValue(MaterialOpacityParamName, currentOpacity);

   const float targetOpacity = _isVisible ? 1.0f : 0.0f;
   if (currentOpacity == targetOpacity)
   {
      UE_LOG(LogTATThiefsignDecalActor, Verbose, TEXT("[%s] | Reached target opacity %f! Disabling tick..."), *GetOwner()->GetName(), currentOpacity);
      SetActorTickEnabled(false);
   }
}

void ATATThiefsignDecalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // These values set when the actor is first spawned and should not be changed after
   DOREPLIFETIME_CONDITION(ATATThiefsignDecalActor, ThiefsignIdentifier, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATThiefsignDecalActor, InstigatorPlayer, COND_InitialOnly);
}

#if WITH_EDITOR
void ATATThiefsignDecalActor::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   FName propertyName = (propertyChangedEvent.Property != nullptr) ? propertyChangedEvent.Property->GetFName() : NAME_None;
   if (propertyName == GET_MEMBER_NAME_CHECKED(ATATThiefsignDecalActor, ThiefsignIdentifier))
   {
      _LoadMaterial();
   }
}
#endif // WITH_EDITOR

void ATATThiefsignDecalActor::_LoadMaterial()
{
   _isMaterialLoaded = false;

   UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::GetMutable();
   const bool loadSymbolsIfUnloaded = true;
   if (thiefsignSettings.AreSymbolsLoaded(loadSymbolsIfUnloaded))
   {
      const FTATThiefsignInfo* thiefsignInfo = thiefsignSettings.FindThiefsignInfo(ThiefsignIdentifier);
      if (thiefsignInfo == nullptr)
      {
         if (HasAuthority())
         {
            // Destroy the actor on the server if Thiefsign symbol data cannot be found
            UE_LOG(LogTATThiefsignDecalActor, Error, TEXT("[%s] | Failed to find data for %s using UTATThiefsignSettings::FindThiefsignInfo()! Destroying actor...")
               , *GetOwner()->GetName(), *ThiefsignIdentifier.ToString());
            Destroy();
         }
         else
         {
            UE_LOG(LogTATThiefsignDecalActor, Error, TEXT("[%s] | Failed to find data for %s using UTATThiefsignSettings::FindThiefsignInfo()!")
               , *GetOwner()->GetName(), *ThiefsignIdentifier.ToString());
         }

         return;
      }

      _cachedVFXConfig = thiefsignSettings.DecalVFXConfig;
      _cachedVFXConfig.Merge(thiefsignInfo->DecalVFXConfigOverrides);

      const TSoftObjectPtr<UMaterialInterface> materialClass = _cachedVFXConfig.MaterialClass;
      if (materialClass.IsValid() && DecalComponent->GetDecalMaterial() != nullptr
         && DecalComponent->GetDecalMaterial()->GetMaterial() == materialClass.Get()->GetMaterial())
      {
         _LoadDynamicMaterialInstance();
      }
      else
      {
         UAssetManager::GetStreamableManager().RequestAsyncLoad(materialClass.ToSoftObjectPath(), [weakThis = MakeWeakObjectPtr(this)]()
         {
            if (weakThis.IsValid())
            {
               ATATThiefsignDecalActor* thisActor = weakThis.Get();
               thisActor->DecalComponent->SetDecalMaterial(thisActor->_cachedVFXConfig.MaterialClass.Get());

               // Force creation of new dynamic material instance
               thisActor->_dynamicMaterialInstance = nullptr;

               thisActor->_LoadDynamicMaterialInstance();
            }
         });
      }
   }
   else
   {
      // Load the symbols table and then re-run this method
      FSimpleMulticastDelegate::FDelegate onLoadedCallback;
      onLoadedCallback.BindWeakLambda(this, [weakThis = MakeWeakObjectPtr(this)]
      {
         if (weakThis.IsValid())
         {
            weakThis->_LoadMaterial();
         }
      });
      thiefsignSettings.CallOrRegisterSymbolsLoadedDelegate(onLoadedCallback);
   }
}

void ATATThiefsignDecalActor::_LoadDynamicMaterialInstance()
{
   check(_cachedVFXConfig.IsValid());

   // Create the dynamic material instance if it hasn't already been
   if (_dynamicMaterialInstance == nullptr)
   {
      _dynamicMaterialInstance = DecalComponent->CreateDynamicMaterialInstance();
   }

   // We don't want decal Thiefsigns displaying different between the player that created them and observers
   // So just use third person material params.
   const bool isFirstPerson = false;

   _cachedVFXConfig.MaterialParams.ApplyToMaterial(_dynamicMaterialInstance, isFirstPerson, [weakThis = MakeWeakObjectPtr(this)]()
   {
      if (ATATThiefsignDecalActor* self = weakThis.Get())
      {
         self->_OnDynamicMaterialInstanceLoaded();
      }
   });
}

void ATATThiefsignDecalActor::_OnDynamicMaterialInstanceLoaded()
{
   _isMaterialLoaded = true;

   // Initialize opacity to one if always visible, otherwise zero for fade in
   const float initialOpacity = _IsEditorPreview() ? 1.0f : 0.0f;
   _dynamicMaterialInstance->SetScalarParameterValue(MaterialOpacityParamName, initialOpacity);

   _UpdateVisibility();
   SetActorHiddenInGame(false);

   // Force a redraw once the material and it's params have loaded
   DecalComponent->MarkRenderStateDirty();

#if WITH_EDITOR
   // Force the editor window to redraw the new material
   if (GEditor != nullptr && !GEditor->IsPlaySessionInProgress())
   {
      GEditor->RedrawLevelEditingViewports();
   }
#endif // WITH_EDITOR
}

void ATATThiefsignDecalActor::_OnLocalPlayerThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled)
{
   _UpdateThiefVisionEnabledCache(thiefVisionEnabled);
}

void ATATThiefsignDecalActor::_UpdateThiefVisionEnabledCache(bool enabled)
{
   if (_isThiefVisionEnabled != enabled)
   {
      _isThiefVisionEnabled = enabled;
      _UpdateVisibility();
   }
}

void ATATThiefsignDecalActor::_UpdateVisibility()
{
   const bool desiredVisibility = _ShouldBeVisible();
   if (_isVisible != desiredVisibility)
   {
      _isVisible = desiredVisibility;

      // Once our material has been loaded, enable tick to animate opacity
      SetActorTickEnabled(_isMaterialLoaded);
   }
}
