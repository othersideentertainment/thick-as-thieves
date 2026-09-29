// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/TATToolSetComponent.h"

// tat
#include "Abilities/TATAbilityMetadata.h"
#include "UI/TATUIFunctionLibrary.h"
#include "Tools/TATToolComponent.h"
#include "Developer/TATProjectSettings.h"
#include "Player/TATCharacter.h"

// ose
#include "Abilities/OSEAbilityInfo.h"
#include "Character/OSECharacterBase.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolSetComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATToolSetComponent, Log, All)

UTATToolSetComponent::UTATToolSetComponent() : Super()
{

}

void UTATToolSetComponent::BeginPlay()
{
   Super::BeginPlay();

   if (CurrentToolData.ToolObject)
   {
      _lastEquippedTool = CurrentToolData.ToolObject->GetClass();
   }

   OnEquippedToolChanged.AddUniqueDynamic(this, &UTATToolSetComponent::_OnEquippedToolChanged);

   OnToolRemoved.AddUniqueDynamic(this, &UTATToolSetComponent::_OnToolRemoved);


   if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
   {
      if (AutoUnequipBehavior != ETATToolSetAutoUnequipBehavior::None)
      {

         if (ownerCharacter->HasAuthority())
         {
            // If we're on authority, we need to wait for:
            //  - our owner's ability system to be initialized
            //  - the base UToolSetComponent to create its default tools
            _CallOrRegisterAuthorityDefaultToolsCreatedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATToolSetComponent::_OnAuthorityDefaultToolsHaveBeenCreated));
         }
         else
         {
            // If we're not on authority, we need to wait the "character ready" callback
            // This will only happen on local players, and only once we finished replicating the ASC, tools,
            // and other character state
            if (ownerCharacter->IsCharacterReady())
            {
               _OnOwnerCharacterReady(ownerCharacter);
            }
            else
            {
               ownerCharacter->OnCharacterReady.AddUniqueDynamic(this, &UTATToolSetComponent::_OnOwnerCharacterReady);
            }
         }
      }

      ownerCharacter->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATToolSetComponent::_OnAbilitiesInitialized));
      ownerCharacter->RegisterAbilitiesResetDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATToolSetComponent::_OnAbilitiesReset));
   }
}

void UTATToolSetComponent::EndPlay(const EEndPlayReason::Type reason)
{
   OnEquippedToolChanged.RemoveAll(this);
   OnToolRemoved.RemoveAll(this);

   if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
   {
      ownerCharacter->OnCharacterReady.RemoveAll(this);
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      for (const TPair<FGameplayTag, FDelegateHandle>& tagAndDelegate : _tagChangedDelegateHandles)
      {
         asc->RegisterGameplayTagEvent(tagAndDelegate.Key).Remove(tagAndDelegate.Value);
      }
   }

   Super::EndPlay(reason);
}

void UTATToolSetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   // For now, only replicate starting gear to the owning player
   params.Condition = COND_OwnerOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _startingGear, params)
}

void UTATToolSetComponent::EquipDefaultTool()
{
   // NB: Don't try to equip a default tool if we're in the middle of another equip/unequip
   // The toolset is not currently designed to be re-entrant safe, and we'd just stomp on an in-flight equip either way
   if (_IsCurrentlyChangingEquippedTool())
   {
      return;
   }

   // If we don't already have the default tool for the physical realm equipped, equip it now
   if (!CurrentToolData.ToolObject || CurrentToolData.ToolObject->GetClass() != DefaultToolPhysical)
   {
      if (!EquipToolByClass(DefaultToolPhysical))
      {
         UE_LOG(LogTATToolSetComponent, Error, TEXT("Toolset on '%s' has default tool '%s', but couldn't be equipped"),
            *GetOwner()->GetName(), *GetNameSafe(DefaultToolPhysical));
      }
   }
}

void UTATToolSetComponent::EquipLastUsedToolIfNoToolEquipped()
{
   // If we already have a tool, or are in the middle of equipping/unequipping one, don't overwrite it
   if (CurrentToolData.ToolObject || _IsCurrentlyChangingEquippedTool())
   {
      return;
   }

   bool didEquip = false;
   if (_lastEquippedTool)
   {
      didEquip = EquipToolByClass(_lastEquippedTool);
   }

   if (!didEquip && EquipDefaultToolsAutomatically)
   {
      EquipDefaultTool();
   }
}

int32 UTATToolSetComponent::GetNumGearTools() const
{
   int32 count = 0;
   for (const UToolComponent* comp : Tools)
   {
      if (!comp || !comp->GetToolInfo().HideInUI)
      {
         count++;
      }
   }
   return count;
}

UToolComponent* UTATToolSetComponent::GetGearToolAtIndex(int32 toolIndex) const
{
   int32 idx = 0;
   for (UToolComponent* comp : Tools)
   {
      if (comp && comp->GetToolInfo().HideInUI)
      {
         continue;
      }
      else if (idx == toolIndex)
      {
         return comp;
      }
      idx++;
   }
   return nullptr;
}

bool UTATToolSetComponent::ToggleEquippedTool(TSubclassOf<UToolComponent> toolClass)
{
   if (toolClass.Get() == nullptr || !HasToolClass(toolClass))
   {
      return false;
   }

   // check if the current tool is the tool we want to toggle
   if (UToolComponent* currentTool = GetCurrentTool())
   {
      if (currentTool->IsA(toolClass))
      {
         // switch back to the previously equipped tool, the default tool, or no tool
         bool didEquip = false;
         if (_toolEquippedBeforeToggle && EquipToolByClass(_toolEquippedBeforeToggle))
         {
            didEquip = true;
         }

         _toolEquippedBeforeToggle = nullptr;

         if (!didEquip)
         {
            if (EquipDefaultToolsAutomatically)
            {
               EquipDefaultTool();
            }
            else
            {
               return UnequipCurrentTool();
            }
         }

         // returning false indicates that the toggled tool was unequipped
         return false;
      }
      else
      {
         // current tool is not the specified tool
         _toolEquippedBeforeToggle = currentTool->GetClass();
      }
   }
   else
   {
      // no current tool at all
      _toolEquippedBeforeToggle = nullptr;
   }

   // equip the specified tool
   return EquipToolByClass(toolClass);
}

bool UTATToolSetComponent::CanAmmoBeRefilled() const
{
   for (UTATToolComponent* gear : _startingGear)
   {
      if (gear)
      {
         if (gear->GetCurrentAmmo() < gear->GetMaxAmmo())
         {
            // Only return false here if the tool can actually have it's ammo refilled by external sources
            if (gear->AmmoRefillType == EAmmoRefillType::CanBeRefilledExternally)
            {
               return false;
            }
         }
      }
   }

   return true;
}

bool UTATToolSetComponent::AuthorityTryAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass, int32 ammoCount, bool removeOnOutOfAmmo)
{
   check(GetOwner()->HasAuthority());
   check(toolClass);
   checkf(ammoCount >= 0, TEXT("AuthorityTryAddToolWithAmmo called with an unexpected ammo count: %d"), ammoCount);

   if (toolClass->GetDefaultObject<UTATToolComponent>()->AmmoType != ETATToolAmmoType::Finite)
   {
      UE_LOG(LogTATToolSetComponent, Error, TEXT("Calling AuthorityTryAddToolWithAmmo with tool '%s', which does not have finite ammo"), *toolClass->GetName());
      return false;
   }

   if (toolClass->GetDefaultObject<UTATToolComponent>()->AmmoRefillType != EAmmoRefillType::CanBeRefilledExternally)
   {
      UE_LOG(LogTATToolSetComponent, Error, TEXT("Calling AuthorityTryAddToolWithAmmo with tool '%s', which does not allow ammo refills from external sources"), *toolClass->GetName());
      return false;
   }

   if (ammoCount == 0)
   {
      UE_LOG(LogTATToolSetComponent, Warning, TEXT("Calling AuthorityTryAddToolWithAmmo with tool '%s' and 0 ammo: this will do nothing"), *toolClass->GetName());
      return false;
   }

   // If we already have this tool in our toolset, just increase the ammo
   if (UTATToolComponent* tatToolExisting = Cast<UTATToolComponent>(_FindToolOfExactClass(toolClass)))
   {
      return tatToolExisting->AuthorityIncreaseAmmoCount(ammoCount);
   }
   else
   {
      // Check if the requested ammo count is beyond the max ammo for the tool
      // NB: This currently is done to enforce atomic updates to ammo, instead of allowing partial changes
      // We could support this, but it would require extra logic on calling code, and potentially be more complicated
      // For now, it's moot since all our ammo increases on the server are increments of one
      if (ammoCount > toolClass->GetDefaultObject<UTATToolComponent>()->GetMaxAmmo())
      {
         return false;
      }

      // Otherwise, add it and set the ammo to the exact amount requested
      if (AuthorityAddToolClass(toolClass))
      {
         if (UTATToolComponent* tatToolNew = Cast<UTATToolComponent>(_FindToolOfExactClass(toolClass)))
         {
            // only enable when true
            if (removeOnOutOfAmmo)
            {
               tatToolNew->RemoveWhenOutOfAmmo = removeOnOutOfAmmo;
            }
            return tatToolNew->AuthoritySetAmmoCount(ammoCount);
         }
         else
         {
            ensureMsgf(false, TEXT("AuthorityTryAddToolWithAmmo: Successfully added tool '%s' but could not find it afterward"), *toolClass->GetName());
            return false;
         }
      }
      else
      {
         UE_LOG(LogTATToolSetComponent, Error, TEXT("AuthorityTryAddToolWithAmmo: Unable to add tool '%s' to toolset"), *toolClass->GetName());
         return false;
      }
   }
}

bool UTATToolSetComponent::HasRoomToAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass, int32 ammoCount) const
{
   check(toolClass);

   const int room = GetRoomToAddToolWithAmmo(toolClass);
   return room > 0 && ammoCount <= room;
}

int32 UTATToolSetComponent::GetRoomToAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass) const
{
   check(toolClass);

   if (toolClass->GetDefaultObject<UTATToolComponent>()->AmmoType != ETATToolAmmoType::Finite)
   {
      UE_LOG(LogTATToolSetComponent, Error, TEXT("Calling GetRoomToAddToolWithAmmo with tool '%s', which does not have finite ammo"), *toolClass->GetName());
      return 0;
   }

   if (toolClass->GetDefaultObject<UTATToolComponent>()->AmmoRefillType != EAmmoRefillType::CanBeRefilledExternally)
   {
      UE_LOG(LogTATToolSetComponent, Error, TEXT("Calling GetRoomToAddToolWithAmmo with tool '%s', which does not allow ammo refills from external sources"), *toolClass->GetName());
      return 0;
   }

   if (const UTATToolComponent* tatToolExisting = Cast<UTATToolComponent>(_FindToolOfExactClass(toolClass)))
   {
      return tatToolExisting->GetRoomToIncreaseAmmoCount();
   }
   else
   {
      return toolClass->GetDefaultObject<UTATToolComponent>()->GetMaxAmmo();
   }
}

int32 UTATToolSetComponent::GetAmmoCostByUsageType(TSubclassOf<UTATToolComponent> toolClass, FGameplayTag usageType) const
{
   check(toolClass);

   if (const UTATToolComponent* tatToolExisting = Cast<UTATToolComponent>(_FindToolOfExactClass(toolClass)))
   {
      return tatToolExisting->GetToolCostByUsageType(usageType);
   }

   UE_LOG(LogTATToolSetComponent, Error, TEXT("Calling GetAmmoCostByUsageType with tool '%s', which does not exist on our toolset!"), *toolClass->GetName());

   return 0;
}

void UTATToolSetComponent::AuthorityAddStartingToolClass(TSubclassOf<UToolComponent> toolClass)
{
   check(GetOwner()->HasAuthority());

   const int32 previousNumTools = Tools.Num();
   const bool wasAdded = AuthorityAddToolClass(toolClass);
   const int32 newNumTools = Tools.Num();

   if (wasAdded)
   {
      checkf(newNumTools == previousNumTools + 1,
         TEXT("Called AuthorityAddToolClass on '%s', but tool count went from %d -> %d"),
         *GetOwner()->GetName(), previousNumTools, newNumTools);

      if (UTATToolComponent* tatToolComponent = Cast<UTATToolComponent>(Tools.Last()))
      {
         _startingGear.Add(tatToolComponent);
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _startingGear, this);
      }
      else
      {
         UE_LOG(LogTATToolSetComponent, Error,
            TEXT("Adding starting tool class '%s' to toolset on '%s', which is not a UTATToolComponent"),
            *toolClass->GetName(),
            *GetOwner()->GetName());
      }
   }
   else
   {
      UE_LOG(LogTATToolSetComponent, Warning,
         TEXT("Tried to add starting tool class '%s' to toolset on '%s', but was not added. Is it a duplicate?"),
         *toolClass->GetName(),
         *GetOwner()->GetName());
   }
}

bool UTATToolSetComponent::IsSuppressingAnimations() const
{
   if (const ITraversalInterface* traversalInterface = GetOwner<ITraversalInterface>())
   {
      // suppress equip animations so not to pre-empt any traversal anims
      if (traversalInterface->IsMantling() || traversalInterface->IsScrambling() || traversalInterface->IsWallClimbing())
      {
         return true;
      }
   }

   return Super::IsSuppressingAnimations();
}

void UTATToolSetComponent::AuthorityRefillAmmoForStartingGear()
{
   check(GetOwner()->HasAuthority());

   for (UTATToolComponent* gear : _startingGear)
   {
      if (gear)
      {
         gear->AuthorityRefillAmmo();
      }
      else
      {
         UE_LOG(LogTATToolSetComponent, Error,
            TEXT("Refilling ammo for starting gear on '%s' and found a null tool. Did we remove one of our starting gear?"),
            *GetOwner()->GetName());
      }
   }
}

void UTATToolSetComponent::AuthorityResetStartingGear()
{
   check(GetOwner()->HasAuthority());

   for (UTATToolComponent* gear : _startingGear)
   {
      if (gear)
      {
         gear->AuthorityResetTool();
      }
      else
      {
         UE_LOG(LogTATToolSetComponent, Error,
            TEXT("Resetting starting gear on '%s' and found a null tool. Did we remove one of our starting gear?"),
            *GetOwner()->GetName());
      }
   }
}

bool UTATToolSetComponent::ShouldCreateDefaultTools() const
{
   AActor* owner = GetOwner();
   // AllowDefaultPlayerTools only applies to player pawns, not NPCs
   if (owner != nullptr && owner->IsA<ATATCharacter>())
   {
      if (const FTATMapTypeSettings* mapTypeSettings = UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorld(this))
      {
         return mapTypeSettings->AllowDefaultPlayerTools;
      }
   }
   return Super::ShouldCreateDefaultTools();
}


void UTATToolSetComponent::_OnEquippedToolChanged()
{
   if (CurrentToolData.ToolObject)
   {
      _lastEquippedTool = CurrentToolData.ToolObject->GetClass();
   }
}

void UTATToolSetComponent::_OnToolRemoved(UToolComponent* tool)
{
   // If we're removing the tool we currently have equipped, switch back to the default one
   if (CurrentToolData.ToolObject == tool)
   {
      UnequipCurrentTool();
      if (EquipDefaultToolsAutomatically)
      {
         EquipDefaultTool();
      }
   }
}

void UTATToolSetComponent::_OnOwnerCharacterReady(AOSECharacterBase* ownerCharacter)
{
   _DoInitialSetup(ownerCharacter);

   // We don't need to get further callbacks once we've done our setup
   // Otherwise, we would get called again, e.g. on re-possession after astral projection
   ownerCharacter->OnCharacterReady.RemoveAll(this);
}

void UTATToolSetComponent::_DoInitialSetup(AOSECharacterBase* ownerCharacter)
{
   if (EquipDefaultToolsAutomatically)
   {
      // N.B. If we already have a tool, there's no point in clobbering it
      if (CurrentToolData.ToolObject == nullptr)
      {
         EquipDefaultTool();
      }
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      const bool isRequestingUnequip = asc->HasAnyMatchingGameplayTags(TagsToCauseUnequip);
      _SetIsRequestingUnequip(isRequestingUnequip);

      for (const FGameplayTag& tag : TagsToCauseUnequip)
      {
         FDelegateHandle handle = asc->RegisterGameplayTagEvent(tag).AddUObject(this, &UTATToolSetComponent::_OnUnequipTagsChanged);
         _tagChangedDelegateHandles.Add(tag, handle);
      }
   }
}

void UTATToolSetComponent::_OnUnequipTagsChanged(const FGameplayTag tag, int32 newTagCount)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      const bool isRequestingUnequip = asc->HasAnyMatchingGameplayTags(TagsToCauseUnequip);
      _SetIsRequestingUnequip(isRequestingUnequip);
   }
}

void UTATToolSetComponent::_OnAuthorityDefaultToolsHaveBeenCreated()
{
   if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
   {
      // We know that the default tools have been created, so now we just need to wait for the ability system to be initialized
      // This probably has already happened, but we should still use CallOrRegisterAbilitiesInitializedDelegate() to be sure
      ownerCharacter->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATToolSetComponent::_AuthorityOnAbilitySystemComponentReady));
   }
}

void UTATToolSetComponent::_AuthorityOnAbilitySystemComponentReady()
{
   if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
   {
      _DoInitialSetup(ownerCharacter);
   }
}

void UTATToolSetComponent::_OnAbilitiesInitialized()
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      const bool isAutoStowed = asc->HasAnyMatchingGameplayTags(UTATProjectSettings::Get().AutoStowTags);
      _SetIsAutoStowed(isAutoStowed);

      for (const FGameplayTag& tag : UTATProjectSettings::Get().AutoStowTags)
      {
         FDelegateHandle handle = asc->RegisterGameplayTagEvent(tag).AddUObject(this, &UTATToolSetComponent::_OnAutoStowTagsChanged);
         _tagChangedDelegateHandles.Add(tag, handle);
      }
   }
}

void UTATToolSetComponent::_OnAbilitiesReset()
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      // remove any loose tags that had been set on reset
      if (_isAutoStowed)
      {
         asc->RemoveLooseGameplayTags(UTATProjectSettings::Get().AddedTagsWhenAutoStowed);
      }
   }
}

void UTATToolSetComponent::_OnAutoStowTagsChanged(const FGameplayTag tag, int32 newTagCount)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      const bool isAutoStowed = asc->HasAnyMatchingGameplayTags(UTATProjectSettings::Get().AutoStowTags);
      _SetIsAutoStowed(isAutoStowed);
   }
}

void UTATToolSetComponent::_SetIsAutoStowed(bool isAutoStowed)
{
   if (_isAutoStowed == isAutoStowed)
   {
      return;
   }

   _isAutoStowed = isAutoStowed;

   StowCurrentTool(isAutoStowed);

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (isAutoStowed)
      {
         asc->AddLooseGameplayTags(projectSettings.AddedTagsWhenAutoStowed);
         asc->CancelAbilities(&projectSettings.AbilitiesToCancelWhenAutoStowed);
      }
      else
      {
         asc->RemoveLooseGameplayTags(projectSettings.AddedTagsWhenAutoStowed);
      }
   }
}

void UTATToolSetComponent::_SetIsRequestingUnequip(bool isRequestingUnequip)
{
   ensureMsgf(AutoUnequipBehavior != ETATToolSetAutoUnequipBehavior::None,
      TEXT("UTATToolSetComponent for '%s' is running _SetIsRequestingUnequip despite AutoUnequipBehavior being set to None"),
      *GetOwner()->GetName());

   if (_isRequestingUnequip != isRequestingUnequip)
   {
      // If we're requesting unequip and want to suppress animations, set that before we actually change our tool
      if (SuppressAnimationsOnAutomaticUnequip)
      {
         CurrentToolData.SuppressAnimations = isRequestingUnequip;
      }

      if (isRequestingUnequip)
      {
         _toolEquippedBeforeRequestingUnequip = GetCurrentTool() != nullptr;
         UnequipCurrentTool();
      }
      else
      {
         if (AutoUnequipBehavior == ETATToolSetAutoUnequipBehavior::AutoUnequipAndReEquip && _toolEquippedBeforeRequestingUnequip)
         {
            EquipLastUsedToolIfNoToolEquipped();
         }
      }

      _isRequestingUnequip = isRequestingUnequip;
   }
}
