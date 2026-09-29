// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATToolComponent.h"

// tat
#include "Animation/TATAnimSetOverride.h"
#include "Developer/TATToolSettings.h"
#include "Disguise/TATDisguiseComponent.h"
#include "Disguise/TATDisguisableCharacterInterface.h"
#include "Tools/TATToolSetComponent.h"
#include "UI/TATHUDToolWidget.h"

// ose
#include "Abilities/Effects/OSEGameplayEffectSet.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEGameplayAbilitySet.h"
#include "Items/ToolAbility.h"
#include "Player/OSEPlayerCharacter1P.h"

// ue
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "UI/TATHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATToolComponent, Log, All)

namespace TATToolHelpers
{
   static bool HasActiveToolAbility(const UAbilitySystemComponent* asc, const UTATToolComponent* tool)
   {
      check(asc && tool);
      for (const FGameplayAbilitySpec& abilitySpec : asc->GetActivatableAbilities())
      {
         if (abilitySpec.SourceObject == tool && abilitySpec.ActiveCount > 0)
         {
            return true;
         }
      }

      return false;
   }

   static bool SafeToRemoveOnOutOfAmmo(const UAbilitySystemComponent* asc, const UTATToolComponent* tool)
   {
      return !TATToolHelpers::HasActiveToolAbility(asc, tool) && tool->GetCurrentAmmo() == 0;
   }
}

void UTATToolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // Replicate _currentAmmo to all players, since that can now affect if it appears stowed
   // Can still get away without MaxAmmo for now

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _currentAmmo, params);

   params.Condition = COND_OwnerOnly;
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MaxAmmo, params);
}

#if WITH_EDITOR
EDataValidationResult UTATToolComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   // Check that if we're specifying gear metadata, it will actually be loadable at runtime
   if (FTATGearMetadataTableRow* metadata = MetadataTableRow.GetRow<FTATGearMetadataTableRow>(TEXT("UTATToolComponent::IsDataValid")))
   {
      TSoftObjectPtr<UDataTable> gearMetadataPtr = UTATToolSettings::Get().GearMetadataTable;

      if (UDataTable* gearMetadata = gearMetadataPtr.LoadSynchronous())
      {
         TSoftClassPtr<UTATToolComponent> gearClassToLoad = UTATToolSettings::LookupToolClassByToolType(gearMetadata, metadata->ToolID);

         if (gearClassToLoad.IsNull())
         {
            context.AddError(FText::FromString(FString::Printf(
               TEXT("Tool '%s' specifys a MetadataTableRow '%s', but its tool tag '%s' cannot be located in the project's gear metadata table. ")
               TEXT("Check that the data table you referenced is part of GearMetadataTable in TAT Tool settings, and that the tags match what you expect"),
               *GetName(),
               *MetadataTableRow.ToDebugString(),
               *metadata->ToolID.ToString()
            )));
            result = EDataValidationResult::Invalid;
         }
      }
   }

   const FGameplayTagContainer& requiredBlockingTags = UTATToolSettings::Get().TagsToBlockActivationForToolUsageAbilities;
   const FGameplayTagContainer& requiredAbilityTags = UTATToolSettings::Get().RequiredAbilityTagsForToolUsageAbilities;

   auto checkAbility = [&](TSubclassOf<UGameplayAbility> abilityClass) {
      if (UOSEGameplayAbility* ability = Cast<UOSEGameplayAbility>(abilityClass.GetDefaultObject()))
      {
         // Make sure it is blocked by the required tags
         for (const FGameplayTag& requiredTag : requiredBlockingTags)
         {
            if (!ability->GetActivationBlockedTags().HasTag(requiredTag))
            {
               context.AddError(FText::FromString(FString::Printf(
                  TEXT("Tool '%s' grants ability '%s' which is not blocked by tag '%s' as expected "),
                  *GetName(),
                  *ability->GetName(),
                  *requiredTag.ToString()
               )));
               result = EDataValidationResult::Invalid;
            }
         }

         // Make sure any required ability tags are present
         for (const FGameplayTag& requiredTag : requiredAbilityTags)
         {
            if (!ability->GetAssetTags().HasTag(requiredTag))
            {
               context.AddError(FText::FromString(FString::Printf(
                  TEXT("Tool '%s' grants ability '%s' does not have ability tag '%s' as expected "),
                  *GetName(),
                  *ability->GetName(),
                  *requiredTag.ToString()
               )));
               result = EDataValidationResult::Invalid;
            }
         }
      }
   };

   for (UOSEGameplayAbilitySet* abilitySet : EquippedAbilitySets)
   {
      if (abilitySet == nullptr) continue;

      abilitySet->ForEachAbility([&](const FOSEAbilityBindInfo& abilityBind) {
         // Only bother checking for abilities bound to a player input
         if (abilityBind.bBindToInput && abilityBind.AbilityClass)
         {
            checkAbility(abilityBind.AbilityClass);
         }
      });
   }

   if (UToolAbility* toolAbility = ToolAbilityClass.GetDefaultObject())
   {
      if (UGameplayEffect* toolEffect = toolAbility->ChildEffectClass.GetDefaultObject())
      {
         if (toolEffect->FindComponent<UAbilitiesGameplayEffectComponent>())
         {
            context.AddWarning(FText::FromString(FString::Printf(
               TEXT("Tool '%s' has tool ability with child effect that grants abilities. Prefer to use EquippedAbilitySets to grant abilities while the tool is equipped."),
               *GetName()
            )));
         }
      }
   }

   return result;
}

void UTATToolComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propertyName = propertyChangedEvent.MemberProperty ? propertyChangedEvent.MemberProperty->GetFName() : NAME_None;
   if (propertyName == GET_MEMBER_NAME_CHECKED(UTATToolComponent, MetadataTableRow))
   {
      // If we've potentially changed the metadata table/row we're pointing at, reload our metadata from it and bind to
      // the changed callback
      _ReloadMetadataFromTable();
      _BindToMetadataTableChanged();
   }
}

bool UTATToolComponent::CanEditChange(const FProperty* inProperty) const
{
   if (!MetadataTableRow.IsNull())
   {
      // Metadata properties cannot be directly set if we have a reference to the metadata table:
      // in that case the table is the canonical value
      if (inProperty)
      {
         const FName propName = inProperty->GetFName();
         if (propName == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolName))
         {
            return false;
         }
         if (propName == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolDescription))
         {
            return false;
         }
         if (propName == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, SourceSprite))
         {
            return false;
         }
      }
   }

   return Super::CanEditChange(inProperty);
}
#endif

void UTATToolComponent::PostLoad()
{
   Super::PostLoad();

   _ReloadMetadataFromTable();

#if WITH_EDITOR
   _BindToMetadataTableChanged();
#endif
}

void UTATToolComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      if (_currentAmmo != MaxAmmo)
      {
         _currentAmmo = MaxAmmo;
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _currentAmmo, this);
      }
   }
}

bool UTATToolComponent::AuthorityUseAmmo(int32 ammoToUse)
{
   return _AuthoritySetAmmoInternal(_currentAmmo - ammoToUse);
}

void UTATToolComponent::AuthorityRefillAmmo()
{
   if (AmmoRefillType == EAmmoRefillType::CanBeRefilledExternally)
   {
      _AuthorityRefillAmmoInternal();
   }
}

bool UTATToolComponent::AuthoritySetAmmoCount(int32 count)
{
   return _AuthoritySetAmmoInternal(count);
}

bool UTATToolComponent::AuthorityIncreaseAmmoCount(int32 count)
{
   checkf(count >= 0, TEXT("AuthorityIncreaseAmmoCount called with an unexpected ammo count: %d"), count);

   if (count == 0)
   {
      UE_LOG(LogTATToolComponent, Warning, TEXT("AuthorityIncreaseAmmoCount called on tool '%s' with a value of 0: this will do nothing"), *GetName());
      return false;
   }

   return _AuthoritySetAmmoInternal(_currentAmmo + count);
}

int32 UTATToolComponent::GetRoomToIncreaseAmmoCount() const
{
   if (AmmoType != ETATToolAmmoType::Finite)
   {
      UE_LOG(LogTATToolComponent, Warning, TEXT("Trying to check room for ammo for tool w/ infinite ammo '%s'"), *GetName());
      return 0;
   }

   return MaxAmmo - _currentAmmo;
}

void UTATToolComponent::AuthorityOverrideMaxAmmo(int32 newMaxAmmo)
{
   if (MaxAmmo != newMaxAmmo)
   {
      MaxAmmo = newMaxAmmo;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MaxAmmo, this);
   }
}

#if OSE_CHEATS_ENABLED
void UTATToolComponent::AuthorityRefillAmmo_CHEAT()
{
   if (ensure(GetOwner()->HasAuthority()))
   {
      // Refill ammo even if we ordinarily wouldn't be able to
      _AuthorityRefillAmmoInternal();
   }
}
#endif

void UTATToolComponent::AuthorityResetTool()
{
   AuthorityRefillAmmo();
   BP_AuthorityOnResetTool();
}

int32 UTATToolComponent::GetToolCostByUsageType(FGameplayTag usageTypeTag) const
{
   if (const int32* toolCost = AmmoCostsByUsageTag.Find(usageTypeTag))
   {
      return *toolCost;
   }

   UE_LOG(LogTATToolComponent, Error, TEXT("Calling GetToolCostByUsageType with tool '%s' and UsageType %s, which is not defined on this tool"), *GetName(), *usageTypeTag.ToString());
   
   return 0;
}

bool UTATToolComponent::CanCurrentlyBeUsed() const
{
   // Infinite ammo means we never run out
   if (AmmoType == ETATToolAmmoType::Infinite)
   {
      return true;
   }
   else if (AmmoType == ETATToolAmmoType::Finite)
   {
      return _currentAmmo > 0;
   }
   else
   {
      // Currently only two ammo types, if we add more add more branches
      checkNoEntry();
      return false;
   }
}

bool UTATToolComponent::IsToolStowedOnEquip() const
{
   if (const UTATToolSetComponent* toolset = Cast<UTATToolSetComponent>(_GetOwningToolSet()))
   {
      if (toolset->IsAutoStowed())
      {
         return true;
      }
   }

   return Super::IsToolStowedOnEquip();
}

bool UTATToolComponent::AuthorityGetParametersForWorldActor(FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const
{
   // By default, we don't provide parameters for a world actor. Subclasses should override this as needed
   return false;
}

void UTATToolComponent::_ApplyEquippedGameplayEffects()
{
   if (!EquippedEffectClass && !EquippedEffectSet)
   {
      return;
   }

   UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetOwner());
   if (asc == nullptr || !asc->IsOwnerActorAuthoritative())
   {
      return;
   }

   if (EquippedEffectSet)
   {
      _equippedEffectHandles = EquippedEffectSet->ApplyEffects(asc);
   }

   if (EquippedEffectClass)
   {
      int32 effectLevel = 1;

      // Check the upgrade requirement if there is one
      if (EquippedEffectUpgradeTag.IsValid())
      {
         effectLevel = asc->GetUpgradeValue(EquippedEffectUpgradeTag);
      }

      if (effectLevel >= EquippedEffectUpgradeLevel)
      {
         FGameplayEffectContextHandle ctx = asc->MakeEffectContext();
         const UGameplayEffect* gameplayEffect = EquippedEffectClass->GetDefaultObject<UGameplayEffect>();
         _equippedEffectHandles.Add(asc->ApplyGameplayEffectToSelf(gameplayEffect, static_cast<float>(effectLevel), ctx));
      }
   }
}

void UTATToolComponent::_RemoveEquippedGameplayEffects()
{
   if (_equippedEffectHandles.IsEmpty())
   {
      return;
   }

   for (const FActiveGameplayEffectHandle& effectHandle : _equippedEffectHandles)
   {
      if (UAbilitySystemComponent* asc = effectHandle.GetOwningAbilitySystemComponent())
      {
         asc->RemoveActiveGameplayEffect(effectHandle, 1);
      }
   }
   _equippedEffectHandles.Reset();
}

void UTATToolComponent::_RequestAnimSet() const
{
   if(!AnimSetTag.IsValid())
   {
      return;
   }
   if(ITATAnimSetOverrideInterface* animSetOverride = GetOwner<ITATAnimSetOverrideInterface>())
   {
      animSetOverride->AddAnimSetRequest(FTATAnimSetRequest{
         .AnimSetTag = AnimSetTag,
         .Source = this
      });
   }
}

void UTATToolComponent::_RemoveAnimSet() const
{
   if(!AnimSetTag.IsValid())
   {
      return;
   }
   
   if(ITATAnimSetOverrideInterface* animSetOverride = GetOwner<ITATAnimSetOverrideInterface>())
   {
      animSetOverride->RemoveAnimSetBySource(this);
   }
}

void UTATToolComponent::_AddToolHUDWidget()
{
   // Only show the HUD widget if the tool's owner is locally controlled (otherwise we're showing this on a remote player's HUD) 
   ACharacter* ownerCharacter = GetOwnerCharacter();
   if (ownerCharacter == nullptr || !ownerCharacter->IsLocallyControlled())
   {
      return;
   }

   if (HUDWidgetWhenEquipped.IsNull())
   {
      return;
   }

   // We're likely to already have loaded this class, as a blueprint using this system would probably have at least one cast to the type
   TSubclassOf<UTATHUDToolWidget> widgetClass = HUDWidgetWhenEquipped.LoadSynchronous();
   if (!widgetClass)
   {
      return;
   }

   ATATHUD* hud = ATATHUD::TryGetLocalTATHUD(GetOwner());
   if (hud == nullptr)
   {
      return;
   }

   // Create the widget if needed
   if (_hudToolWidget == nullptr)
   {
      if (UTATHUDToolWidget* newWidget = CreateWidget<UTATHUDToolWidget>(hud->GetOwningPlayerController(), widgetClass))
      {
         _hudToolWidget = newWidget;

         newWidget->PostConstructSetupToolWidget(this);

         // Let the tool configure the widget before passing it to the HUD
         OnHUDToolWidgetCreated(newWidget);
      }
   }

   if (_hudToolWidget == nullptr)
   {
      return;
   }

   // Ask the HUD to display the widget
   hud->OnAddToolWidget(_hudToolWidget);
}

void UTATToolComponent::_RemoveToolHUDWidget()
{
   if (HUDWidgetWhenEquipped.IsNull() || _hudToolWidget == nullptr)
   {
      return;
   }

   ATATHUD* hud = ATATHUD::TryGetLocalTATHUD(GetOwner());
   if (hud == nullptr)
   {
      return;
   }

   // Ask the HUD to remove the widget
   hud->OnRemoveToolWidget(_hudToolWidget);
}

bool UTATToolComponent::OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool)
{
   _ApplyEquippedGameplayEffects();
   _RequestAnimSet();
   _AddToolHUDWidget();
   if (_hudToolWidget != nullptr)
   {
      _hudToolWidget->OnToolEquipped(this);
   }
   return Super::OnEquip_Implementation(prevTool);
}

bool UTATToolComponent::OnUnequip_Implementation()
{
   _RemoveEquippedGameplayEffects();
   _RemoveAnimSet();
   if (_hudToolWidget != nullptr)
   {
      _hudToolWidget->OnToolUnequipped(this);
   }
   _RemoveToolHUDWidget();
   return Super::OnUnequip_Implementation();
}

void UTATToolComponent::ModifyVisuals_Implementation(EMeshPerspective perspective, USkeletalMeshComponent* mesh)
{
   Super::ModifyVisuals_Implementation(perspective, mesh);

   // If running animations, use URO
   // (possibly skip if using budgeted animation component)
   if (mesh->GetAnimClass())
   {
      mesh->bEnableUpdateRateOptimizations = true;
   }
}

namespace ToolHelpers
{
   UTATDisguiseComponent* GetDisguiseComponent(ACharacter* character)
   {
      if (character != nullptr && character->Implements<UTATDisguisableCharacterInterface>())
      {
         if (UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(character))
         {
            return disguiseComponent;
         }
      }
      return nullptr;
   }
}

void UTATToolComponent::_LinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer)
{
   Super::_LinkAnimClassLayers(animClassLayer);

   // let the disguise component handle tool anim class layer changes if it wants to
   UTATDisguiseComponent* disguiseComponent = ToolHelpers::GetDisguiseComponent(GetOwnerCharacter());
   if (disguiseComponent != nullptr && disguiseComponent->IsDisguiseActive() && disguiseComponent->HandleToolLinkAnimClassLayers(this, animClassLayer))
   {
      return;
   }

   Super::_LinkAnimClassLayers(animClassLayer);
}

void UTATToolComponent::_UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer)
{
   // let the disguise component handle tool anim class layer changes if it wants to
   UTATDisguiseComponent* disguiseComponent = ToolHelpers::GetDisguiseComponent(GetOwnerCharacter());
   if (disguiseComponent != nullptr && disguiseComponent->IsDisguiseActive() && disguiseComponent->HandleToolUnlinkAnimClassLayers(this, animClassLayer))
   {
      return;
   }

   Super::_UnlinkAnimClassLayers(animClassLayer);
}

void UTATToolComponent::_StartRemoveOnOutOfAmmo()
{
   check(GetOwner()->HasAuthority());
   if (_abilityEndDelegateHandle.IsValid())
   {
      return;
   }

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      // If removing the tool on out of ammo, we want to wait until the tool abilities are over, so
      // that is does not interrupt something early
      _abilityEndDelegateHandle = asc->AbilityEndedCallbacks.AddUObject(this, &ThisClass::_OnAbilityEndForRemove);
      _TryRemoveOnOutOfAmmo(asc);
   }
}

void UTATToolComponent::_OnAbilityEndForRemove(class UGameplayAbility* endedAbility)
{
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      if (TATToolHelpers::SafeToRemoveOnOutOfAmmo(asc, this))
      {
         // Removing an ability in the middle of its callback seems to be safe, but logs an error, so do it after a frame delay
         GetOwner()->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
            if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
               _TryRemoveOnOutOfAmmo(asc);
            }
         ));
      }
   }
}

void UTATToolComponent::_TryRemoveOnOutOfAmmo(UAbilitySystemComponent* asc)
{
   if (TATToolHelpers::SafeToRemoveOnOutOfAmmo(asc, this))
   {
      asc->AbilityEndedCallbacks.Remove(_abilityEndDelegateHandle);
      _abilityEndDelegateHandle.Reset();
      if (UToolSetComponent* toolset = _GetOwningToolSet())
      {
         toolset->AuthorityRemoveToolsOfClass(GetClass());
         // do nothing after this
      }
   }
}

void UTATToolComponent::_AuthorityRefillAmmoInternal()
{
   // NB: _AuthoritySetAmmoInternal will warn on non-finite ammo types, but we don't want callers of refill to have to check ammo types,
   // so instead we handle it here and just no-op if it's an infinite ammo tool
   if (AmmoType == ETATToolAmmoType::Finite)
   {
      _AuthoritySetAmmoInternal(MaxAmmo);
   }
}

bool UTATToolComponent::_AuthoritySetAmmoInternal(int32 newAmmoCount)
{
   check(GetOwner()->HasAuthority());

   if (AmmoType != ETATToolAmmoType::Finite)
   {
      UE_LOG(LogTATToolComponent, Warning, TEXT("Trying to adjust ammo for tool w/ infinite ammo '%s'"), *GetName());
      return false;
   }

   // If the requested ammo count is out of our range ignore it
   // NB: The requested change is atomic, meaning we either do the full change, or not at all
   // We may want behavior that allows partial updates, but that logic can get more complicated so for now enforce atomicity
   if (newAmmoCount < 0 || newAmmoCount > MaxAmmo)
   {
      return false;
   }

   if (_currentAmmo != newAmmoCount)
   {
      int32 oldAmmoCount = _currentAmmo;

      _currentAmmo = newAmmoCount;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _currentAmmo, this);

      OnAmmoChanged.Broadcast(this, oldAmmoCount, _currentAmmo);

      _UpdateStowedStateBasedOnUsability();

      if (newAmmoCount == 0 && RemoveWhenOutOfAmmo)
      {
         _StartRemoveOnOutOfAmmo();
      }
   }

   return true;
}

void UTATToolComponent::_OnRep_CurrentAmmo(int32 oldAmmo)
{
   OnAmmoChanged.Broadcast(this, oldAmmo, _currentAmmo);

   _UpdateStowedStateBasedOnUsability();
}

void UTATToolComponent::_ReloadMetadataFromTable()
{
   if (FTATGearMetadataTableRow* metadata = MetadataTableRow.GetRow<FTATGearMetadataTableRow>(TEXT("UTATToolComponent::_ReloadMetadataFromTable")))
   {
      ToolInfo.ToolName = metadata->Metadata.ToolName;
      ToolInfo.ToolDescription = metadata->Metadata.ToolDescription;
      ToolInfo.SourceSprite = metadata->Metadata.IconSprite;
   }
}

#if WITH_EDITOR
void UTATToolComponent::_BindToMetadataTableChanged()
{
   // Clear out the old delegate if need be
   if (FTATGearMetadataTableRow* oldMetadataRow = _metadataTableRowForChangedHandle.GetRow<FTATGearMetadataTableRow>(TEXT("UTATToolComponent::_BindToMetadataTableChanged old row")))
   {
      oldMetadataRow->OnMetadataDataTableChanged.Remove(_onMetadataTableChangedHandle);
   }

   _onMetadataTableChangedHandle.Reset();

   if (FTATGearMetadataTableRow* metadataRow = MetadataTableRow.GetRow<FTATGearMetadataTableRow>(TEXT("UTATToolComponent::_BindToMetadataTableChanged new row")))
   {
      _onMetadataTableChangedHandle = metadataRow->OnMetadataDataTableChanged.AddUObject(this, &UTATToolComponent::_ReloadMetadataFromTable);
      _metadataTableRowForChangedHandle = MetadataTableRow;
   }
}
#endif
