// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootInventory.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Items/Tokens/TATTokenEffect_LootDecoy.h"
#include "Loot/TATLootActor.h"
#include "Loot/TATLootBagActor.h"
#include "Loot/TATLootInterface.h"
#include "Loot/TATLootUtils.h"
#include "Loot/TATStashedLootSubsystem.h"
#include "UI/TATToastBroadcaster.h"
#include "Math/TATMath.h"
#include "Settings/TATMatchSettings.h"
#include "Settings/TATMatchSettingsBase.h"
#include "Loot/TATCallingCard.h"
#include "Player/TATPlayerState.h"
#include "Player/TATPlayerStatsTags.h"
#include "Tools/TATLargeCarryLootToolComponent.h"
#include "Loot/TATLootInventorySubsystem.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Items/ToolSetInterface.h"
#include "Player/OSEPlayerState.h"
#include "OSECommon.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/Count.h"
#include "GameplayEffectTypes.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventory)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventory, Log, All);


namespace InventoryHelpers
{
   template<typename ArrT, typename OutT>
   FORCEINLINE bool GetLootByIndex(const TArray<ArrT>& lootArray, int32 index, OutT& outItem)
   {
      static_assert(std::is_same_v<ArrT, OutT> || std::is_same_v<OutT, FTATLootItemVariant>,
         "Expected output item to be the same as the array item, or FTATLootItemVariant");
      if (lootArray.IsValidIndex(index))
      {
         if constexpr (std::is_same_v<OutT, FTATLootItemVariant>)
         {
            outItem.Set(lootArray[index]);
         }
         else
         {
            outItem = lootArray[index];
         }
         return true;
      }
      outItem = OutT{};
      return false;
   }

   template<typename T>
   FORCEINLINE int32 GetLastLootIndex(const TArray<T>& lootArray)
   {
      return (lootArray.Num() > 0) ? (lootArray.Num() - 1) : INDEX_NONE;
   }

   template<typename T>
   FORCEINLINE bool GetLootIdentifierByIndex(const TArray<T>& lootArray, int32 index, FTATLootIdentifier& outLootId)
   {
      static_assert(std::is_same_v<T, FTATLootIdentifier> || std::is_same_v<T, FTATLootInstance>,
         "GetLootIdentifierByIndex requires an array of FTATLootIdentifier or FTATLootInstance");
      if (lootArray.IsValidIndex(index))
      {
         if constexpr (std::is_same_v<T, FTATLootIdentifier>)
         {
            outLootId = lootArray[index];
         }
         else
         {
            outLootId = lootArray[index].Identifier;
         }
         return true;
      }
      outLootId.Invalidate();
      return false;
   }
}


UTATLootInventoryComponent::UTATLootInventoryComponent()
{
   SetIsReplicatedByDefault(true);
   _team = IOSETeamInterface::kInvalidTeam;
}

void UTATLootInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   // Major loot needs to be visualized on character for other clients, and should be visible in owner's inventory UI
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _majorLoot, params);

   // Large-carried loot may be surfaced on other players outside of the tool (for now)
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _largeCarriedLoot, params);

   // This bag-size is replicated to other clients to avoid having to replicate contents of _minorLoot
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _lootBagFullness, params);

   // Exact contents of minor loot bag only relevant for owner - other clients can make use of _lootBagFullness proxy for visualizing bag on player model
   params.Condition = COND_OwnerOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _minorLoot, params);
}

void UTATLootInventoryComponent::BeginPlay()
{
   Super::BeginPlay();
   
   if (UTATLootInventorySubsystem* lootInventorySubsystem = GetWorld()->GetSubsystem<UTATLootInventorySubsystem>())
   {
      lootInventorySubsystem->RegisterLootInventoryComponent(this);
   }

   TWeakObjectPtr<UTATLootInventoryComponent> weakThis(this);

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();

   if (!lootSettings.GameplayEffectToApplyWhenHoldingMajorLoot.IsNull())
   {
      TSoftClassPtr<UGameplayEffect> majorLootEffectClass = lootSettings.GameplayEffectToApplyWhenHoldingMajorLoot;
      UAssetManager::GetStreamableManager().RequestAsyncLoad(majorLootEffectClass.ToSoftObjectPath(), [weakThis, majorLootEffectClass]
      {
         if (UTATLootInventoryComponent* self = weakThis.Get())
         {
            self->_gameplayEffectToApplyWhenHoldingMajorLoot = majorLootEffectClass.Get();

            // If the character somehow acquired major loot between BeginPlay and when the gameplay effect asset finished async loading, apply it immediately
            if (self->HasMajorLoot() && self->GetOwner() && self->GetOwner()->HasAuthority())
            {
               self->_AddMajorLootGameplayEffect();
            }
         }
      });
   }
}

void UTATLootInventoryComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (GetOwner()->HasAuthority())
   {
      // Clear loot accumulator reset timer
      GetOwner()->GetWorldTimerManager().ClearTimer(_authorityLootDamageEventTimer);

      // Clear major-loot-glyph-spawn timer
      GetWorld()->GetTimerManager().ClearTimer(_carriedMajorLootGlyphSpawnTimerHandle);

      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         // Unbind from ASC health attribute change
         const UAttributeBaseSet* attributeBaseSet = asc->GetSet<UAttributeBaseSet>();
         check(IsValid(attributeBaseSet));
         asc->GetGameplayAttributeValueChangeDelegate(attributeBaseSet->GetHealthAttribute()).RemoveAll(this);

         // If we added any gameplay tags due to holding major loot, remove them
         const FGameplayTagContainer& majorLootTags = UTATLootSettings::Get().CharacterGrantedTagsWhenHoldingMajorLoot;
         if (_majorLoot.Num() > 0 && majorLootTags.Num() > 0)
         {
            asc->RemoveLooseGameplayTags(majorLootTags);
         }
      }
   }

   if (UTATLootInventorySubsystem* lootInventorySubsystem = GetWorld()->GetSubsystem<UTATLootInventorySubsystem>())
   {
      lootInventorySubsystem->UnregisterLootInventoryComponent(this);
   }

   Super::EndPlay(endPlayReason);
}

UTATLootInventoryComponent* UTATLootInventoryComponent::GetForActor(AActor* actor)
{
   if (ITATLootInventoryInterface* lootInterface = Cast<ITATLootInventoryInterface>(actor))
   {
      return lootInterface->GetLootInventoryComponent();
   }

   return nullptr;
}

int UTATLootInventoryComponent::GetTotalLootCount() const
{
#if DO_CHECK
   const APawn* ownerPawn = _GetOwningPawn();
   check(IsValid(ownerPawn));
   if (!ownerPawn->HasAuthority() && !ownerPawn->IsLocallyControlled())
   {
      UE_LOG(LogTATLootInventory, Error, TEXT("GetTotalLootCount() called by client on non-local character %s! ")
         TEXT("Clients do not have access to the exact contents of (or number of) minor/quest loot items held by another character")
         , *GetNameSafe(GetOwner()));
      return -1;
   }
#endif //DO_CHECK
   return _minorLoot.Num() + _majorLoot.Num();
}

const TArray<FTATLootIdentifier>& UTATLootInventoryComponent::GetMinorLoot() const
{
#if DO_CHECK
   const APlayerState* ps = Cast<APlayerState>(GetOwner());
   check(ps->HasAuthority() || ps->HasLocalNetOwner());
#endif // DO_CHECK
   return _minorLoot;
}

bool UTATLootInventoryComponent::GetMinorLootByIndex(int32 index, FTATLootIdentifier& outLoot) const
{
   return InventoryHelpers::GetLootByIndex(_minorLoot, index, outLoot);
}

bool UTATLootInventoryComponent::GetMajorLootByIndex(int32 index, FTATLootInstance& outInstance) const
{
   return InventoryHelpers::GetLootByIndex(_majorLoot, index, outInstance);
}

bool UTATLootInventoryComponent::GetMajorLootById(const FTATLootInstanceId lootId, FTATLootInstance& outInstance) const
{
   for (const FTATLootInstance& majorLoot : _majorLoot)
   {
      if (majorLoot.Id == lootId)
      {
         outInstance = majorLoot;
         return true;
      }
   }
   outInstance.Invalidate();
   return false;
}

bool UTATLootInventoryComponent::IsValidLootIndex(ETATLootType lootType, int32 index) const
{
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      return _minorLoot.IsValidIndex(index);
   case ETATLootType::MajorLoot:
      return _majorLoot.IsValidIndex(index);
   default:
      break;
   }
   return false;
}

bool UTATLootInventoryComponent::GetLootItemByIndex(ETATLootType lootType, int32 index, FTATLootItemVariant& outLoot) const
{
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      if (_minorLoot.IsValidIndex(index))
      {
         outLoot.Set(_minorLoot[index]);
         return true;
      }
      break;
   case ETATLootType::MajorLoot:
      if (_majorLoot.IsValidIndex(index))
      {
         outLoot.Set(_majorLoot[index]);
         return true;
      }
      break;
   default:
      break;
   }
   return false;
}

int32 UTATLootInventoryComponent::GetLootCount(ETATLootType lootType) const
{
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      return _minorLoot.Num();
   case ETATLootType::MajorLoot:
      return _majorLoot.Num();
   default:
      break;
   }
   return INDEX_NONE;
}

int32 UTATLootInventoryComponent::GetLastIndexForLootType(ETATLootType lootType) const
{
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      return InventoryHelpers::GetLastLootIndex(_minorLoot);
   case ETATLootType::MajorLoot:
      return InventoryHelpers::GetLastLootIndex(_majorLoot);
   default:
      break;
   }
   return INDEX_NONE;
}

bool UTATLootInventoryComponent::GetLootIdentifierByIndex(ETATLootType lootType, int32 index, FTATLootIdentifier& outLootId) const
{
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      return InventoryHelpers::GetLootIdentifierByIndex(_minorLoot, index, outLootId);
   case ETATLootType::MajorLoot:
      return InventoryHelpers::GetLootIdentifierByIndex(_majorLoot, index, outLootId);
   default:
      break;
   }
   outLootId.Invalidate();
   return false;
}

int32 UTATLootInventoryComponent::GetInventorySlotsUsed() const
{
   // TODO: Investigate if this is a value that should be cached.
   // Not sure if the cache misses from all these lookups will be an actual problem or not because I'm not sure how frequently this will be called.
   // Caching is non-trivial just because there are so many places that modify _minorLoot, _majorLoot
   // We could consolidate them into a container struct for simpler change tracking, but due to inventory replication needs that's also non-trivial.
   int32 result = 0;
   result += UTATLootSettings::Get().GetSlotSizeCombined(this, _majorLoot);
   result += UTATLootSettings::Get().GetSlotSizeCombined(this, _minorLoot);
   result += _predictedAdditionalUsedSlots;
   return result;
}

int32 UTATLootInventoryComponent::GetInventorySlotsFree() const
{
   if (!UseInventorySlotMaxCapacity)
   {
      return std::numeric_limits<int32>::max();
   }
   return FMath::Max(0, InventorySlotMaxCapacity - GetInventorySlotsUsed());
}

int32 UTATLootInventoryComponent::GetInventorySlotMaxCapacity() const
{
   if (!UseInventorySlotMaxCapacity)
   {
      return std::numeric_limits<int32>::max();
   }
   return InventorySlotMaxCapacity;
}

bool UTATLootInventoryComponent::IsInventoryFull() const
{
   if (!UseInventorySlotMaxCapacity)
   {
      return false;
   }
   return GetInventorySlotsUsed() >= InventorySlotMaxCapacity;
}

const FTATLootInstance& UTATLootInventoryComponent::GetMajorLootRefByIndex(int32 index) const
{
   check(_majorLoot.IsValidIndex(index));
   return _majorLoot[index];
}

void UTATLootInventoryComponent::AuthoritySetToolset(TScriptInterface<IToolSetInterface> toolset)
{
   check(GetOwner()->HasAuthority());
   if (toolset == _toolset)
   {
      return;
   }

   if (_toolset && HasLargeCarryLoot())
   {
      _RemoveLargeCarryTool();
   }

   _toolset = toolset;

   if (_toolset && HasLargeCarryLoot())
   {
      _AddLargeCarryTool();
   }
}

bool UTATLootInventoryComponent::HandlePickupLootItem(const FTATLootIdentifier& lootId, ATATLootActor* lootActor, FPredictionKey& predictionKey)
{
   if (GetOwner()->HasAuthority())
   {
      check(lootActor);

      if (!lootActor->GetLootIdentifier().IsValid())
      {
         return false;
      }

      FTATLootItemVariant lootItem(lootId);
      if (lootActor->HasLootInstanceData())
      {
         check(lootId == lootActor->LootInstance.Identifier);
         lootItem.Set(lootActor->LootInstance);
      }

      if (AuthorityTryPickupLootItem(lootItem, lootActor->GetCanPlaceCallingCard(), lootActor->GetTransform()))
      {
         AuthorityOnLootActorPickedUp.Broadcast(lootActor);
         return true;
      }
   }
   else if (HasRoomForLoot(lootId))
   {
      // ASSUMPTION: Clients return true if their local state indicates room for loot.
      // TODO-LARGECARRY: something?
      PredictAddedLoot(lootId, 1, predictionKey);
      return true;
   }

   return false;
}

void UTATLootInventoryComponent::PredictAddedLoot(const FTATLootIdentifier& lootId, int32 lootCount, FPredictionKey& predictionKey)
{
   // ASSUMPTION: asc is on same actor, so decent probability of being in same bunch
   if (predictionKey.IsValidForMorePrediction() && _IncrementPredictedLoot(lootId, lootCount))
   {
      predictionKey.NewRejectOrCaughtUpDelegate(FPredictionKeyEvent::CreateUObject(this, &UTATLootInventoryComponent::_OnPredictedLootCatchup, lootId, lootCount));
   }
}

bool UTATLootInventoryComponent::AuthorityTryPickupLootItem(const FTATLootItemVariant& lootItem, bool spawnCallingCard, const FTransform& callingCardTransform)
{
   check(GetOwner()->HasAuthority());

   // Only valid loot should be picked up
   const FTATLootIdentifier& lootId = lootItem.GetIdentifier();
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootId);
   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATLootInventory, Warning, TEXT("AuthorityTryPickupLootItem() called with invalid loot identifier '%s'!"), *lootId.LootTag.ToString());
      return false;
   }

   // Add to inventory if there's room
   if (!HasRoomForLoot(*lootInfo))
   {
      UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | No room for loot item %s")
         , *GetOwner()->GetName()
         , *lootInfo->DisplayName.ToString());
      return false;
   }

   if (lootInfo->IsLargeCarry)
   {
      check(!_largeCarriedLoot.IsValid());

      _largeCarriedLoot = lootItem.GetAsInstance();
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _largeCarriedLoot, this);

      _OnLargeCarryLootAdded(_largeCarriedLoot);
   }
   else
   {
      switch (lootInfo->LootType)
      {
      case ETATLootType::MinorLoot:
         _minorLoot.Add(lootInfo->LootIdentifier);
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _minorLoot, this);

         _OnMinorLootChanged(ETATInventoryUpdateEventType::Add);
         break;

      case ETATLootType::MajorLoot:
      {
         // We can assume that the Identifier is valid from FindLootInfo(...) checks above
         // But we need to check that the ID is valid
         FTATLootInstance lootInstance = lootItem.GetInstanceRef();
         if (!lootInstance.IsValid())
         {
            lootInstance = lootInfo->CreateDefaultInstance(this);
         }

         const int32 newlyAddedIndex = _majorLoot.Add(lootInstance);
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _majorLoot, this);

         _AuthorityOnMajorLootAdded(_majorLoot[newlyAddedIndex]);
      }
      break;

      default:
         checkNoEntry();
         return false;
      }
   }

   _AuthorityOnLootAdded(lootId);

   UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | %s added to inventory")
      , *GetOwner()->GetName()
      , *lootInfo->DisplayName.ToString());

   // Leave a calling card in place of the loot we just picked up.
   // If there is a decoy do that instead of the calling card
   if(spawnCallingCard && !FTATTokenEffect_LootDecoy::TryDropDecoy(GetOwner(), lootId, callingCardTransform))
   {
      UAssetManager::GetStreamableManager().RequestAsyncLoad(_callingCardActorClass.ToSoftObjectPath(), [weakThis = MakeWeakObjectPtr(this), callingCardTransform, lootItem, lootInfo]
      {
         if (UTATLootInventoryComponent* self = weakThis.Get())
         {
            self->_AuthoritySpawnCallingCard(weakThis->_callingCardActorClass.Get(), callingCardTransform, lootItem, *lootInfo);
         }
      });
   }

   return true;
}

bool UTATLootInventoryComponent::AuthorityTryPickupLootItem(const FTATLootIdentifier& lootId)
{
   FTATLootItemVariant lootItem(lootId);
   return AuthorityTryPickupLootItem(lootItem);
}

bool UTATLootInventoryComponent::AuthorityTryPickupLootInstance(const FTATLootInstance& lootInstance)
{
   FTATLootItemVariant lootItem(lootInstance);
   return AuthorityTryPickupLootItem(lootItem);
}

bool UTATLootInventoryComponent::HasRoomForLoot(const FTATLootInfo& lootInfo) const
{
   if (lootInfo.IsLargeCarry)
   {
      // There is already one
      return !_largeCarriedLoot.Identifier.IsValid();
   }

   if (!UseInventorySlotMaxCapacity)
   {
      return true;
   }

   const int32 slotSize = lootInfo.GetSlotSize();
   return GetInventorySlotsFree() - slotSize >= 0;
}

bool UTATLootInventoryComponent::HasRoomForLoot(const FTATLootIdentifier& lootId) const
{
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(this, lootId);
   return (lootInfo != nullptr) && HasRoomForLoot(*lootInfo);
}

bool UTATLootInventoryComponent::HasRoomForLoot(const FDataTableRowHandle& lootRowHandle) const
{
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(lootRowHandle);
   if (lootInfo != nullptr)
   {
      return HasRoomForLoot(*lootInfo);
   }
   UE_LOG(LogTATLootInventory, Warning, TEXT("HasRoomForLoot() failed to retrieve FTATLootInfo for given FDataTableRowHandle! Could not select which loot collection to query."));
   return false;
}

bool UTATLootInventoryComponent::AuthorityHasLoot(const FTATLootIdentifier& lootId) const
{
   if (_minorLoot.Contains(lootId) || _majorLoot.Contains(lootId) || _largeCarriedLoot.Identifier == lootId)
   {
      return true;
   }

   return false;
}

int32 UTATLootInventoryComponent::AuthorityCountLoot(const FTATLootIdentifier& lootId) const
{
   int32 total = 0;
   total += Algo::Count(_minorLoot, lootId);
   total += Algo::Count(_majorLoot, lootId);
   if (_largeCarriedLoot.Identifier == lootId)
   {
      total += 1;
   }

   return total;
}

int UTATLootInventoryComponent::AuthorityMoveLootToStash(ETATLootType lootType, TFunctionRef<void(const FTATLootItemVariant&)> deposit)
{
   check(GetOwner()->HasAuthority());
   
   FTATLootInventoryNotifyScope notifyScope(this);
   int itemsDeposited = 0;
   
   auto depositArray = [this, &itemsDeposited, &deposit]
      (ETATLootType lootType, const auto& lootArray)
   {
      for (int i = lootArray.Num() - 1; i >= 0; --i)
      {
         if (!_CanLootBeStashed(lootArray[i]))
         {
            continue;
         }
         
         FTATLootItemVariant removedItem;
         if (_AuthorityRemoveLootItemByIndex(lootType, i, &removedItem))
         {
            itemsDeposited++;
            deposit(removedItem);
         }
      }
   };

   switch (lootType)
   {
   case ETATLootType::MajorLoot:
      depositArray(ETATLootType::MajorLoot, _majorLoot);
      break;
   case ETATLootType::MinorLoot:
      depositArray(ETATLootType::MinorLoot, _minorLoot);
      break;
   default:
      checkNoEntry();
   }
   return itemsDeposited;
}

int UTATLootInventoryComponent::AuthorityMoveLootToStash(TFunctionRef<void(const FTATLootItemVariant&)> deposit)
{
   return AuthorityMoveLootToStash(ETATLootType::MajorLoot, deposit) + AuthorityMoveLootToStash(ETATLootType::MinorLoot, deposit);
}

bool UTATLootInventoryComponent::HasStashableLoot(ETATLootType lootType) const
{
   // auto in param is a template
   auto canBeStashed = [this] (const auto& loot) { return _CanLootBeStashed(loot); };

   switch (lootType)
   {
   case ETATLootType::MajorLoot:
      return _majorLoot.ContainsByPredicate(canBeStashed);
   case ETATLootType::MinorLoot:
      return _minorLoot.ContainsByPredicate(canBeStashed);
   default:
      checkNoEntry();
      return 0;
   }
}

bool UTATLootInventoryComponent::HasStashableLoot() const
{
   return HasStashableLoot(ETATLootType::MinorLoot) || HasStashableLoot(ETATLootType::MajorLoot);
}

void UTATLootInventoryComponent::SetTeam(uint8 team)
{
   if (UTATLootInventorySubsystem* lootInventorySubsystem = GetWorld()->GetSubsystem<UTATLootInventorySubsystem>())
   {
      lootInventorySubsystem->NotifyLootInventorySetTeam(this, team);
   }

   if (_team == team)
   {
      return;
   }

   UTATStashedLootSubsystem* stashSubsystem = GetWorld()->GetSubsystem<UTATStashedLootSubsystem>();
   if (stashSubsystem)
   {
      stashSubsystem->RemoveTeamValueListener(_team, this);
   }

   _team = team;

   if (stashSubsystem)
   {
      stashSubsystem->GetTeamValueListener(_team).AddUObject(this, &UTATLootInventoryComponent::_UpdateTeamStashedValue);
      _UpdateTeamStashedValue(stashSubsystem->GetStashedValueForTeam(_team));
   }
}

int32 UTATLootInventoryComponent::AuthorityMoveMajorLootToOtherInventory(UTATLootInventoryComponent* otherInventory, int32 maxItemsToTransfer)
{
   check(GetOwner()->HasAuthority());
   if (otherInventory == nullptr)
   {
      return 0;
   }

   FTATLootInventoryNotifyScope notifyScope(this);
   FTATLootInventoryNotifyScope otherNotifyScope(otherInventory);

   maxItemsToTransfer = FMath::Clamp(maxItemsToTransfer, 0, _majorLoot.Num());
   if (maxItemsToTransfer <= 0)
   {
      return 0;
   }

   FTATLootItemVariant lootItem;
   int32 numItemsTransferred = 0;
   while (_majorLoot.Num() > 0 && numItemsTransferred < maxItemsToTransfer)
   {
      if (!otherInventory->HasRoomForLoot(_majorLoot.Last().Identifier))
      {
         break;
      }

      if (_AuthorityRemoveLootItemByIndex(ETATLootType::MajorLoot, _GetLastIndexOfStashableMajorLoot(), &lootItem))
      {
         check(lootItem.IsValid());
         const bool pickupSuccess = otherInventory->AuthorityTryPickupLootItem(lootItem);
         ensure(pickupSuccess);
         numItemsTransferred++;
      }
      else
      {
         // On failure, reduce the max count so we don't end up in an infinite loop
         maxItemsToTransfer--;
      }
   }
   return numItemsTransferred;
}

bool UTATLootInventoryComponent::AuthorityMoveLootToOtherInventory(UTATLootInventoryComponent* otherInventory, ETATLootType lootType, int32 lootIndex,
   bool& lootDroppedOnGround, ETATInventoryMoveItemFailureMode failureMode, bool sendPickpocketToasts)
{
   lootDroppedOnGround = false;

   check(GetOwner()->HasAuthority());
   if (otherInventory == nullptr)
   {
      return false;
   }

   FTATLootItemVariant lootItem;
   if (!GetLootItemByIndex(lootType, lootIndex, lootItem))
   {
      return false;
   }

   // lootItem should be valid because GetLootItemByIndex returned true
   if (!ensure(lootItem.IsValid()))
   {
      return false;
   }

   const bool otherInventoryHasRoom = otherInventory->HasRoomForLoot(lootItem);
   if (!otherInventoryHasRoom && failureMode == ETATInventoryMoveItemFailureMode::DoNothing)
   {
      return false;
   }

   if (!_AuthorityRemoveLootItemByIndex(lootType, lootIndex))
   {
      return false;
   }

   // At this point, the loot item has been removed from our inventory, so either it gets picked up by the other inventory,
   // or we have to drop it on the ground to avoid it getting deleted entirely.
   TArray<FTATLootIdentifier, TInlineAllocator<1>> lootIdsToDropOnGround;
   TArray<FTATLootInstance, TInlineAllocator<1>> lootInstancesToDropOnGround;

   bool pickupSuccess = false;
   if (otherInventoryHasRoom)
   {
      pickupSuccess = otherInventory->AuthorityTryPickupLootItem(lootItem);
   }

   auto getPlayerState = [](UTATLootInventoryComponent* comp) -> APlayerState*
   {
      check(comp != nullptr);
      if (AActor* owner = comp->GetOwner())
      {
         if (APlayerState* ps = Cast<APlayerState>(owner))
         {
            return ps;
         }
         if (ACharacter* ownerChar = Cast<ACharacter>(owner))
         {
            return ownerChar->GetPlayerState();
         }
      }
      return nullptr;
   };

   if (sendPickpocketToasts)
   {
      if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
      {
         const bool droppedOnGround = !pickupSuccess;
         toastBroadcaster->ClientToastBroadcast_PlayerPickpocketedLoot(getPlayerState(otherInventory), getPlayerState(this), lootItem.GetIdentifier(), droppedOnGround);
      }
   }

   if (!pickupSuccess)
   {
      if (lootItem.IsInstance())
      {
         lootInstancesToDropOnGround.Add(lootItem.GetInstanceRef());
      }
      else
      {
         lootIdsToDropOnGround.Add(lootItem.GetIdentifier());
      }

      // Drop the loot item either at our location or the other character's location, depending on the requested failure mode
      if (failureMode == ETATInventoryMoveItemFailureMode::DropOnGroundByDest)
      {
         otherInventory->_AuthoritySpawnLootPinata(lootIdsToDropOnGround, lootInstancesToDropOnGround);
      }
      else
      {
         _AuthoritySpawnLootPinata(lootIdsToDropOnGround, lootInstancesToDropOnGround);
      }

      lootDroppedOnGround = true;
   }

   // Return true if loot was successfully removed from our inventory (even if dropped on the ground)
   return true;
}

bool UTATLootInventoryComponent::GetRandomLootItem(ETATLootType& outLootType, int32& outLootIndex, float minorLootChance, float majorLootChance) const
{
   outLootType = ETATLootType::None;
   outLootIndex = INDEX_NONE;

   if (GetTotalLootCount() == 0)
   {
      return false;
   }

   // For each loot type, if we don't have any loot of that type, zero out the chances we'll pick that type
   if (GetMinorLootCount() == 0)
   {
      minorLootChance = 0.0f;
   }
   if (GetMajorLootCount() == 0)
   {
      majorLootChance = 0.0f;
   }

   const TArray<TTATRandomWeight<ETATLootType>, TInlineAllocator<3>> weights = {
      { ETATLootType::MinorLoot, minorLootChance },
      { ETATLootType::MajorLoot, majorLootChance },
   };
   outLootType = TATMath::WeightedRandom(MakeArrayView(weights)).Get(ETATLootType::None);
   if (outLootType == ETATLootType::None)
   {
      return false;
   }

   const int32 lootCount = GetLootCount(outLootType);

   // we already checked loot counts, so it should not have selected an empty loot type
   if (!ensure(lootCount > 0))
   {
      outLootType = ETATLootType::None;
      return false;
   }

   outLootIndex = FMath::RandRange(0, lootCount - 1);
   check(IsValidLootIndex(outLootType, outLootIndex));
   return true;
}

void UTATLootInventoryComponent::AuthorityStashDataForEscape(FMatchPersistentData& persistentData, bool bWasCaught) const
{
   check(GetOwner()->HasAuthority());

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();

   TConstArrayView<FTATLootInstance> largeCarryLootView = _largeCarriedLoot.Identifier.IsValid() ?
      MakeArrayView(&_largeCarriedLoot, 1) : TConstArrayView<FTATLootInstance>();

   // Determine the value of what we're carrying
   persistentData.OriginalCarriedLootValue += lootSettings.GetLootValue(this, _minorLoot);
   persistentData.OriginalCarriedLootValue += lootSettings.GetLootValue(this, _majorLoot);
   persistentData.OriginalCarriedLootValue += lootSettings.GetLootValue(this, largeCarryLootView);

   if (bWasCaught)
   {
      // If we're captured, we lose everything we carried
      persistentData.KeptCarriedLootValue = 0;
   }
   else
   {
      persistentData.KeptCarriedLootValue = persistentData.OriginalCarriedLootValue;
      // Bundle in carried major/minor loot on successful escape
      for (const FTATLootInstance& majorLoot : _majorLoot)
      {
         persistentData.CarriedLoot.Add(majorLoot.Identifier);
      }

      for (const FTATLootInstance& largeCarryLoot : largeCarryLootView)
      {
         persistentData.CarriedLoot.Add(largeCarryLoot.Identifier);
      }

      // TODO: replace once conversion to money is done at pickup
      for (const FTATLootIdentifier& lootIdentifier : _minorLoot)
      {
         if (lootSettings.DoesAutoConvertToMoney(this, lootIdentifier))
         {
            persistentData.MoneyTaken += lootSettings.GetLootValue(this, lootIdentifier);
         }
         else
         {
            persistentData.CarriedLoot.Add(lootIdentifier);
         }
      }
   }
}

void UTATLootInventoryComponent::AuthorityDropLootBagVoluntarily()
{
   check(GetOwner()->HasAuthority());
   _AuthorityDropLootBag(ETATLootDropReason::Voluntary);
}

void UTATLootInventoryComponent::AuthorityDropLootBagInvoluntarily()
{
   check(GetOwner()->HasAuthority());
   _AuthorityDropLootBag(ETATLootDropReason::Involuntary);
}


void UTATLootInventoryComponent::AuthorityDropRandomMinorLootVoluntarily()
{
   check(GetOwner()->HasAuthority());
   _AuthorityDropRandomLoot(ETATLootType::MinorLoot, ETATLootDropReason::Voluntary);
}

void UTATLootInventoryComponent::AuthorityDropRandomMinorLootInvoluntarily()
{
   check(GetOwner()->HasAuthority());
   _AuthorityDropRandomLoot(ETATLootType::MinorLoot, ETATLootDropReason::Involuntary);
}

void UTATLootInventoryComponent::AuthorityDropAllMajorLootInvoluntarily()
{
   check(GetOwner()->HasAuthority());

   // NB: Make a separate array so we aren't iterating over it while mutating it
   TArray<FTATLootInstanceId, TInlineAllocator<8>> lootIdsToDrop;

   for (const FTATLootInstance& majorLoot : _majorLoot)
   {
      lootIdsToDrop.Add(majorLoot.Id);
   }

   for (const FTATLootInstanceId lootInstanceId : lootIdsToDrop)
   {
      _AuthorityDropLootItem(lootInstanceId, ETATLootDropReason::Involuntary);
   }
}

void UTATLootInventoryComponent::AuthorityDropLargeCarryLootVoluntarily()
{
   check(GetOwner()->HasAuthority());
   if (HasLargeCarryLoot())
   {
      _AuthorityDropLootItem(FLootDropParams(TInPlaceType<FLargeCarryTag>()), ETATLootDropReason::Voluntary);
   }
}

void UTATLootInventoryComponent::AuthorityDropSingleMajorLootVoluntarily(FTATLootInstanceId lootInstanceId)
{
   check(GetOwner()->HasAuthority());
   ensure(lootInstanceId.IsValid());

   _AuthorityDropLootItem(lootInstanceId, ETATLootDropReason::Voluntary);
}

void UTATLootInventoryComponent::AuthorityRemoveSingleMajorLootWithoutDropping(FTATLootInstanceId lootInstanceId)
{
   check(GetOwner()->HasAuthority());
   check(lootInstanceId.IsValid());
   const bool wasDropped = _AuthorityRemoveLootItem(FLootDropParams(TInPlaceType<FTATLootInstanceId>(), lootInstanceId));
   ensureMsgf(wasDropped, TEXT("Loot inventory on '%s' tried to remove major loot but couldn't"), *GetOwner()->GetName());
}

bool UTATLootInventoryComponent::AuthorityDropSingleLootItemVoluntarily(ETATLootType lootType, int32 lootIndex)
{
   if (IsValidLootIndex(lootType, lootIndex))
   {
      _AuthorityDropLootByIndex(lootType, lootIndex, ETATLootDropReason::Voluntary);
      return true;
   }
   return false;
}

void UTATLootInventoryComponent::ServerRequestDropSingleLootItemVoluntarily_Implementation(ETATLootType lootType, int32 lootIndex)
{
   AuthorityDropSingleLootItemVoluntarily(lootType, lootIndex);
}

void UTATLootInventoryComponent::ServerRequestDropAllLootVoluntary_Implementation()
{
   AuthorityDropAllLootVoluntary();
}

void UTATLootInventoryComponent::AuthorityOnHealthChanged(const FOnAttributeChangeData& data)
{
   check(GetOwner()->HasAuthority());

   // Note: GE_HealthRegen produces attribute change callbacks each tick, even when health is full

   const float newHealth = data.NewValue;
   const float damageDealt = data.OldValue - data.NewValue;

   // We only care if damage is dealt
   if (damageDealt <= 0.f)
   {
      return;
   }

   // drop large carry on any damage, per design
   if (HasLargeCarryLoot() && _isAllowedToEverDropItemFromDamage)
   {
      _AuthorityDropLargeCarry(ETATLootDropReason::Involuntary);
   }

   // A knockout event may cause the player to drop certain types of loot based on the match settings
   if (newHealth <= 0.f)
   {
      UE_LOG(LogTATLootInventory, Verbose, TEXT("Player %s knocked out! Dropping loot based on match settings..."), *GetOwner()->GetName());
      AuthorityDropLootOnKO();
      return;
   }

   // Non-KO damage can only cause player to drop minor loot. So don't bother keeping track of damage accumulation if they've got nothing to drop.
   if (_minorLoot.Num() == 0 || _isAllowedToEverDropItemFromDamage == false)
   {
      _authorityLootDropDamageAccumulator = 0.f;
      return;
   }

   _authorityLootDropDamageAccumulator += damageDealt;
   UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | _lootDropDamageAccumulator => %f (change %f)")
      , *GetOwner()->GetName()
      , _authorityLootDropDamageAccumulator
      , damageDealt);

   // If damage accumulator exceeds threshold, drop random loot
   if (_authorityLootDropDamageAccumulator >= _lootDropDamageThreshold)
   {
      UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | Received damage exceeding threshold! Dropping loot..."), *GetOwner()->GetName());

      while (_authorityLootDropDamageAccumulator >= _lootDropDamageThreshold && _minorLoot.Num() > 0)
      {
         _authorityLootDropDamageAccumulator -= _lootDropDamageThreshold;

         AuthorityDropRandomMinorLootInvoluntarily();
      }

      // Reset to avoid damage roll-over between loot drop events
      _authorityLootDropDamageAccumulator = 0.f;
   }

   // Clear timer whenever damage is dealt
   GetOwner()->GetWorldTimerManager().ClearTimer(_authorityLootDamageEventTimer);

   // If damage accumulator is non-zero, start timer again
   if (_authorityLootDropDamageAccumulator > 0)
   {
      const UTATLootSettings& lootSettings = UTATLootSettings::Get();
      GetOwner()->GetWorldTimerManager().SetTimer(
         _authorityLootDamageEventTimer
         , this
         , &UTATLootInventoryComponent::_AuthorityResetLootDamageAccumulator
         , _lootDropDamageThresholdResetDuration
         , false);

      UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | Waiting %f seconds to reset loot damage accumulator...")
         , *GetOwner()->GetName()
         , _lootDropDamageThresholdResetDuration);
   }
}

bool UTATLootInventoryComponent::_CanLootBeStashed(const FTATLootIdentifier& lootIdentifier) const
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(this, lootIdentifier))
   {
      return !lootInfo->IsQuestRelated;
   }
   return false;
}

bool UTATLootInventoryComponent::_CanLootBeStashed(const FTATLootInstance& lootInstance) const
{
   return _CanLootBeStashed(lootInstance.Identifier);
}

int UTATLootInventoryComponent::_GetLastIndexOfStashableMajorLoot() const
{
   for (int i = _majorLoot.Num() - 1; i >=0; --i)
   {
      const FTATLootInstance& lootInstance = _majorLoot[i];
      if (_CanLootBeStashed(lootInstance))
      {
         return i;
      }
   }
   return -1;
}

bool UTATLootInventoryComponent::_AuthorityRemoveLootItemByIndex(ETATLootType lootType, int32 lootIndex, FTATLootItemVariant* outLootItem)
{
   auto onLootDropped = [this](const FTATLootIdentifier& lootIdentifier)
   {
      // Invalid loot info entries should never make it to inventory
      const FTATLootInfo& lootInfo = UTATLootSettings::Get().GetLootInfoChecked(this, lootIdentifier);
      check(lootInfo.LootIdentifier == lootIdentifier);
      UE_LOG(LogTATLootInventory, Verbose, TEXT("Removing loot item %s from inventory..."), *lootInfo.DisplayName.ToString());
   };

   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      if (_minorLoot.IsValidIndex(lootIndex))
      {
         const FTATLootIdentifier minorLoot = _minorLoot[lootIndex];

         _minorLoot.RemoveAt(lootIndex);
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _minorLoot, this);

         if (outLootItem != nullptr)
         {
            outLootItem->Set(minorLoot);
         }
         _AuthorityOnLootRemoved(minorLoot);
         _OnMinorLootChanged(ETATInventoryUpdateEventType::Remove);
         onLootDropped(minorLoot);
         return true;
      }
      break;

   case ETATLootType::MajorLoot:
      if (_majorLoot.IsValidIndex(lootIndex))
      {
         FTATLootInstance majorLoot = _majorLoot[lootIndex];

         _majorLoot.RemoveAt(lootIndex);
         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _majorLoot, this);

         _AuthorityOnMajorLootRemoved(majorLoot);
         _AuthorityOnLootRemoved(majorLoot.Identifier);
         if (outLootItem != nullptr)
         {
            outLootItem->Set(majorLoot);
         }
         onLootDropped(majorLoot.Identifier);
         return true;
      }
      break;

   default:
      checkNoEntry();
   }

   return false;
}

void UTATLootInventoryComponent::_AuthorityRemoveAllLootOfType(ETATLootType lootType)
{
   FTATLootInventoryNotifyScope notifyScope(this);
   switch (lootType)
   {
   case ETATLootType::MinorLoot:
      for (int32 i = _minorLoot.Num() - 1; i >= 0; --i)
      {
         if (!_AuthorityRemoveLootItemByIndex(lootType, i))
         {
            UE_LOG(LogTATLootInventory, Error, TEXT("Failed to remove %s index %i while removing all loot of that type"),
               *StaticEnum<ETATLootType>()->GetNameStringByValue(static_cast<int64>(lootType)), i);
         }
      }
      break;
   case ETATLootType::MajorLoot:
      for (int32 i = _majorLoot.Num() - 1; i >= 0; --i)
      {
         if (!_AuthorityRemoveLootItemByIndex(lootType, i))
         {
            UE_LOG(LogTATLootInventory, Error, TEXT("Failed to remove %s index %i while removing all loot of that type"),
               *StaticEnum<ETATLootType>()->GetNameStringByValue(static_cast<int64>(lootType)), i);
         }
      }
      break;
   default:
      checkNoEntry();
   }
}

bool UTATLootInventoryComponent::_AuthorityRemoveLootItem(const FLootDropParams& dropParams, FTATLootItemVariant* outLootItem)
{
   check(GetOwner()->HasAuthority());

   // Dropping by index is the simplest option
   if (const FLootIndex* lootIndexPtr = dropParams.TryGet<FLootIndex>())
   {
      return _AuthorityRemoveLootItemByIndex(lootIndexPtr->Type, lootIndexPtr->Index, outLootItem);
   }

   // If we're dropping an instance, find the instance by its ID
   if (const FTATLootInstanceId* lootInstanceIdPtr = dropParams.TryGet<FTATLootInstanceId>())
   {
      for (int32 i = 0; i < _majorLoot.Num(); i++)
      {
         if (_majorLoot[i].Id == *lootInstanceIdPtr)
         {
            return _AuthorityRemoveLootItemByIndex(ETATLootType::MajorLoot, i, outLootItem);
         }
      }
      return false;
   }

   // If we're just dropping fungible loot, look it up by identifier
   if (const FTATLootIdentifier* lootIdentifierPtr = dropParams.TryGet<FTATLootIdentifier>())
   {
      const FTATLootIdentifier& lootIdentifier = *lootIdentifierPtr;

      // Invalid loot info entries should never make it to inventory
      const FTATLootInfo& lootInfo = UTATLootSettings::Get().GetLootInfoChecked(this, lootIdentifier);
      check(lootInfo.LootIdentifier == lootIdentifier);

      // Remove from appropriate collection
      switch (lootInfo.LootType)
      {
      case ETATLootType::MinorLoot:
         for (int32 i = 0; i < _minorLoot.Num(); i++)
         {
            if (_minorLoot[i] == lootIdentifier)
            {
               return _AuthorityRemoveLootItemByIndex(ETATLootType::MinorLoot, i, outLootItem);
            }
         }
         break;

      case ETATLootType::MajorLoot:
         checkf(false, TEXT("Major loot must be removed via a loot instance, not an identifier tag"));
         break;

      default:
         checkNoEntry();
      }

      UE_LOG(LogTATLootInventory, Error, TEXT("Failed to remove loot with identifier %s: not in inventory"), *lootIdentifier.ToString());
      return false;
   }

   if (dropParams.IsType<FLargeCarryTag>())
   {
      return _AuthorityRemoveLargeCarryLoot(outLootItem);
   }

   // should never get here because we handled all the possible variant types of FLootDropParams
   checkNoEntry();
   return false;
}

bool UTATLootInventoryComponent::_AuthorityRemoveLargeCarryLoot(FTATLootItemVariant* outLootItem /*= nullptr*/)
{
   if (!_largeCarriedLoot.Identifier.IsValid())
   {
      return false;
   }

   if (outLootItem)
   {
      outLootItem->Set(_largeCarriedLoot);
   }

   FTATLootInstance previousValue = _largeCarriedLoot;

   _largeCarriedLoot.Invalidate();
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _largeCarriedLoot, this);

   _OnLargeCarryLootRemoved(previousValue);
   _AuthorityOnLootRemoved(previousValue.Identifier);

   return true;
}

void UTATLootInventoryComponent::_AuthorityDropLootItem(
   const FLootDropParams& dropParams,
   ETATLootDropReason dropReason)
{
   check(GetOwner()->HasAuthority());
   TGuardValue<bool> droppingGuard(_authorityDropping, true);

   // Remove the item from the inventory first, so that if this fails we can skip spawning the item in the world
   FTATLootItemVariant droppedLootItem;
   if (!_AuthorityRemoveLootItem(dropParams, &droppedLootItem))
   {
      UE_LOG(LogTATLootInventory, Error, TEXT("_AuthorityDropLootItem(%s) _AuthorityRemoveLootItem failed to remove loot from inventory"), *droppedLootItem.ToString());
      return;
   }

   // Invalid loot info entries should never make it to inventory
   const FTATLootInfo& lootInfo = UTATLootSettings::Get().GetLootInfoChecked(this, droppedLootItem.GetIdentifier());

   // Make sure we have a loot actor class to spawn
   if (lootInfo.ActorClass.IsNull())
   {
      UE_LOG(LogTATLootInventory, Error, TEXT("_AuthorityDropLootItem() called with FTATLootIdentifier %s whose FTATLootInfo entry has unassigned ActorClass!"), *droppedLootItem.ToString());
      return;
   }

   UE_LOG(LogTATLootInventory, Verbose, TEXT("Dropping loot item %s..."), *lootInfo.DisplayName.ToString());

   // Spawn instance of loot actor at player's location
   UTATLootUtils::SpawnLootActorAsync(GetWorld(), _GetOwningPawn(), lootInfo.ActorClass, droppedLootItem, _GenerateSuggestedLootDropLocation(_GetOwningPawn(), dropReason));
}

void UTATLootInventoryComponent::_AuthorityDropLootByIndex(ETATLootType lootType, int32 index, ETATLootDropReason dropReason)
{
   check(GetOwner()->HasAuthority());
   if (!IsValidLootIndex(lootType, index))
   {
      return;
   }
   const FLootIndex idx{ lootType, index };
   _AuthorityDropLootItem(FLootDropParams(TInPlaceType<FLootIndex>(), idx), dropReason);
}

void UTATLootInventoryComponent::_AuthorityDropRandomLoot(ETATLootType lootType, ETATLootDropReason dropReason)
{
   check(GetOwner()->HasAuthority());
   const int32 lootCount = GetLootCount(lootType);
   if (lootCount <= 0)
   {
      return;
   }
   _AuthorityDropLootByIndex(lootType, FMath::RandRange(0, lootCount - 1), dropReason);
}

void UTATLootInventoryComponent::_AuthorityDropLootBag(ETATLootDropReason dropReason)
{
   check(GetOwner()->HasAuthority());

   TWeakObjectPtr<UTATLootInventoryComponent> weakThis(this);

   // Spawn loot bag actor
   UAssetManager::GetStreamableManager().RequestAsyncLoad(_lootBagActorClass.ToSoftObjectPath(), [weakThis, dropReason]
      {
         if (UTATLootInventoryComponent* self = weakThis.Get())
         {
            FTATLootInventoryNotifyScope notifyScope(self);
            TGuardValue<bool> droppingGuard(self->_authorityDropping, true);
            self->_AuthoritySpawnLootBagActor(weakThis->_lootBagActorClass.Get(), weakThis->_minorLoot, weakThis->_majorLoot, dropReason);
            self->_AuthorityRemoveAllLootOfType(ETATLootType::MinorLoot);
            self->_AuthorityRemoveAllLootOfType(ETATLootType::MajorLoot);
         }
      });
}

void UTATLootInventoryComponent::AuthorityDropAllLootVoluntary()
{
   check(GetOwner()->HasAuthority());

   const int32 numLootItemsToDrop = _minorLoot.Num() + _majorLoot.Num();
   if (numLootItemsToDrop == 0)
   {
      return;
   }

   // Drop voluntary items as a loot bag (or single item)
   if (numLootItemsToDrop == 1)
   {
      // We only have a single item, avoid wrapping that item in a loot bag and just spawn the item directly
      if (_minorLoot.Num() > 0)
      {
         _AuthorityDropLootByIndex(ETATLootType::MinorLoot, 0, ETATLootDropReason::Voluntary);
      }
      else
      {
         check(_majorLoot.Num() == 1);
         _AuthorityDropLootByIndex(ETATLootType::MajorLoot, 0, ETATLootDropReason::Voluntary);
      }
   }
   else
   {
      AuthorityDropLootBagVoluntarily();
   }

   // Deliberately skip quest loot, it's non-droppable

   // With no remaining loot to drop, might as well clear timer and damage accumulator
   GetOwner()->GetWorldTimerManager().ClearTimer(_authorityLootDamageEventTimer);
   _authorityLootDropDamageAccumulator = 0.f;
}

void UTATLootInventoryComponent::AuthorityDropAllLootInvoluntary()
{
   check(GetOwner()->HasAuthority());

   const int32 numLootItemsToDrop = _minorLoot.Num() + _majorLoot.Num();
   if (numLootItemsToDrop == 0)
   {
      return;
   }

   FTATLootInventoryNotifyScope notifyScope(this);
   TGuardValue<bool> droppingGuard(_authorityDropping, true);

   // Drop involuntary items as a pinata
   _AuthoritySpawnLootPinata(_minorLoot, _majorLoot);
   _AuthorityRemoveAllLootOfType(ETATLootType::MinorLoot);
   _AuthorityRemoveAllLootOfType(ETATLootType::MajorLoot);

   if (HasLargeCarryLoot())
   {
      _AuthorityDropLargeCarry(ETATLootDropReason::Involuntary);
   }

   // Deliberately skip quest loot, it's non-droppable

   // With no remaining loot to drop, might as well clear timer and damage accumulator
   GetOwner()->GetWorldTimerManager().ClearTimer(_authorityLootDamageEventTimer);
   _authorityLootDropDamageAccumulator = 0.f;
}

void UTATLootInventoryComponent::AuthorityDropLootOnKO()
{
   check(GetOwner()->HasAuthority());

   FTATLootInventoryNotifyScope notifyScope(this);

   // just in case
   if (HasLargeCarryLoot())
   {
      _AuthorityDropLargeCarry(ETATLootDropReason::Involuntary);
   }

   if(const UTATMatchSettings* TATMatchSettings = UTATMatchSettingsBase::GetTATMatchSettings<UTATMatchSettings>(GetWorld()))
   {      
      switch(TATMatchSettings->LootDropSettings)
      {
      case ETATLootDropSettings::DropAll:
         AuthorityDropAllLootInvoluntary();
         return;
      case ETATLootDropSettings::DropMajor:
         AuthorityDropAllMajorLootInvoluntarily();
         return;
      case ETATLootDropSettings::KeepAll:
      default:
         return;
      }
   }

   // If we reached here, something went wrong with getting the data from match settings.
   UE_LOG(LogTATLootInventory, Warning, TEXT("AuthorityDropLootOnKO() | failed to grab loot drop settings data from match settings, dropping all loot by default"));
   AuthorityDropAllLootInvoluntary();
}

bool UTATLootInventoryComponent::_AuthoritySpawnLootBagActor(TSubclassOf<ATATLootBagActor> lootBagActorClass, TConstArrayView<FTATLootIdentifier> lootIdentifiers, TConstArrayView<FTATLootInstance> lootInstances, ETATLootDropReason dropReason) const
{
   check(GetOwner()->HasAuthority());

   // Don't bother spawning an empty bag
   if (lootIdentifiers.IsEmpty() && lootInstances.IsEmpty())
   {
      return false;
   }

   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   if (ATATLootBagActor* lootBagActor = GetWorld()->SpawnActorDeferred<ATATLootBagActor>(lootBagActorClass, FTransform(), owner, instigator, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
   {
      // Fill with our minor loot
      lootBagActor->AuthorityPopulateLootItems(lootIdentifiers, lootInstances);

      // Move to spawn location
      const FVector suggestedDropLocation = _GenerateSuggestedLootDropLocation(_GetOwningPawn(), dropReason);
      const FVector spawnLocation = _GenerateDroppedLootSpawnLocation(lootBagActor, suggestedDropLocation, dropReason);

      // TODO: give it a cool trajectory in random direction
      UGameplayStatics::FinishSpawningActor(lootBagActor, FTransform(spawnLocation));

      return true;
   }

   return false;
}

void UTATLootInventoryComponent::_AuthoritySpawnLootPinata(TConstArrayView<FTATLootIdentifier> lootIdentifiers, TConstArrayView<FTATLootInstance> lootInstances) const
{
   const int32 numItemsToDrop = lootIdentifiers.Num() + lootInstances.Num();
   if (numItemsToDrop == 0)
   {
      return;
   }

   APawn* owningPawn = _GetOwningPawn();
   auto findLocation = [owningPawn, this]() -> FVector {
      if (owningPawn)
      {
         return owningPawn->GetActorLocation();
      }

      const APlayerState* owner = GetOwner<APlayerState>();
      if (!ensure(owner))
      {
         return FVector::ZeroVector;
      }

      if (const AActor* controller = owner->GetOwningController())
      {
         // Our controllers have bAttachToPawn set, so should have the last pawn location even if pawn is gone
         // (detach uses keep-world-pos)
         return controller->GetActorLocation();
      }
      else
      {
         checkNoEntry();
         return FVector::ZeroVector;
      }
   };

   const FVector pinataLocation = findLocation();

   auto spawnLootActor = [this, owningPawn](const FTATLootItemVariant& lootItem, const FVector& dropLocation)
   {
      if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootItem.GetIdentifier()))
      {
         UTATLootUtils::SpawnLootActorAsync(GetWorld(), owningPawn, lootInfo->ActorClass, lootItem, dropLocation);
      }
   };

   // If we're only dropping one item, drop it in the middle rather than using a spread algorithm
   if (numItemsToDrop == 1 && lootInstances.Num() == 1)
   {
      check(lootIdentifiers.Num() == 0);
      spawnLootActor(FTATLootItemVariant(lootInstances[0]), pinataLocation);
      return;
   }
   if (numItemsToDrop == 1 && lootIdentifiers.Num() == 1)
   {
      check(lootInstances.Num() == 0);
      spawnLootActor(FTATLootItemVariant(lootIdentifiers[0]), pinataLocation);
      return;
   }

   TArray<FVector> validPinataLocations;
   const int totalLootItems = lootInstances.Num() + lootIdentifiers.Num();
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(SpawnLootPinata));
   if (owningPawn)
   {
      queryParams.AddIgnoredActor(owningPawn);
   }
   for (int i = 0; i < totalLootItems; ++i)
   {
      const FVector targetLocation = UTATLootUtils::CalcLootPinataSpawnLocation(pinataLocation, i, totalLootItems, _pinataRadiusRange);
      if (GetWorld()->LineTraceTestByChannel(pinataLocation, targetLocation, ECC_Visibility, queryParams) == false)
      {
         validPinataLocations.Add(targetLocation);
      }
   }
   
   auto getSpawnLocation = [&validPinataLocations, &pinataLocation](const int index)
   {
      const int totalPositions = validPinataLocations.Num();
      if (totalPositions > 0)
         return validPinataLocations[FMath::WrapExclusive(index, 0, totalPositions)];
      return pinataLocation;
   };
   
   int32 pinataIndex = 0;
   for (const FTATLootInstance& lootInstance : lootInstances)
   {
      spawnLootActor(FTATLootItemVariant(lootInstance), getSpawnLocation(pinataIndex));
      ++pinataIndex;
   }

   for (const FTATLootIdentifier& lootId : lootIdentifiers)
   {
      spawnLootActor(FTATLootItemVariant(lootId), getSpawnLocation(pinataIndex));
      ++pinataIndex;
   }
}

void UTATLootInventoryComponent::_AuthoritySpawnCallingCard(TSubclassOf<ATATCallingCard> callingCardActorClass, const FTransform& spawnTransform, const FTATLootItemVariant& lootItem, const FTATLootInfo& lootInfo)
{
   check(GetOwner()->HasAuthority());

   if (const ATATPlayerState* TATPlayerState = Cast<ATATPlayerState>(GetOwner()))
   {
      AActor* owner = nullptr;
      APawn* instigator = nullptr;
      if (ATATCallingCard* callingCardActor = GetWorld()->SpawnActorDeferred<ATATCallingCard>(callingCardActorClass, spawnTransform, owner, instigator, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
      {
         callingCardActor->SetDataFromPlayerState(*TATPlayerState);
         UGameplayStatics::FinishSpawningActor(callingCardActor, spawnTransform);
      }
   }
}

void UTATLootInventoryComponent::_AuthorityOnLootAdded(const FTATLootIdentifier& lootId)
{
   check(GetOwner()->HasAuthority());

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootId);
   if (lootInfo && !lootInfo->CarriedGameplayEffect.IsNull())
   {
      FLootEffectEntry& effectEntry = _authorityLootEffects.FindOrAdd(lootId);
      effectEntry.Count += 1;
      if (!effectEntry.EffectHandle.IsValid())
      {
         // If effect has not been applied yet, load and apply it
         UAssetManager::GetStreamableManager().RequestAsyncLoad(lootInfo->CarriedGameplayEffect.ToSoftObjectPath(), [weakThis = MakeWeakObjectPtr(this), lootId, softEffect = lootInfo->CarriedGameplayEffect] {
            if (UTATLootInventoryComponent* self = weakThis.Get())
            {
               // Apply effect if still relevant
               FLootEffectEntry* effectEntry = self->_authorityLootEffects.Find(lootId);
               if (effectEntry == nullptr || effectEntry->EffectHandle.IsValid())
               {
                  return;
               }

               AActor* owner = self->GetOwner();
               UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(owner);
               check(asc != nullptr);

               TSubclassOf<UGameplayEffect> effectClass = softEffect.Get();
               check(effectClass);

               FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
               effectContext.AddInstigator(owner, owner);
               effectEntry->EffectHandle = asc->ApplyGameplayEffectToSelf(effectClass.GetDefaultObject(), 0.0f, effectContext);
            }
         });
      }
   }
   
}

void UTATLootInventoryComponent::_AuthorityOnLootRemoved(const FTATLootIdentifier& lootId)
{
   // Don't need to actually look up data when removing effect
   if (FLootEffectEntry* effectEntry = _authorityLootEffects.Find(lootId))
   {
      effectEntry->Count -= 1;
      if (effectEntry->Count <= 0)
      {
         if (effectEntry->EffectHandle.IsValid())
         {
            if (UAbilitySystemComponent* asc = effectEntry->EffectHandle.GetOwningAbilitySystemComponent())
            {
               asc->RemoveActiveGameplayEffect(effectEntry->EffectHandle, 1);
            }
         }
         _authorityLootEffects.Remove(lootId);
      }
   }
}

void UTATLootInventoryComponent::_AuthorityOnMajorLootAdded(FTATLootInstance& addedLoot)
{
   check(GetOwner()->HasAuthority());
   check(addedLoot.IsValid());

   // Ensure this is major loot
   check(UTATLootUtils::GetLootType(this, addedLoot.Identifier) == ETATLootType::MajorLoot);

   // Spawn a glyph actor immediately
   _AuthoritySpawnCarriedMajorLootGlyphActor();

   // Set looping timer for broadcasting position with transient glyph actor
   _AuthoritySetCarriedMajorLootGlyphActorSpawnTimer();

   _UpdateMajorLootGameplayTags(true);

   _OnMajorLootAdded(addedLoot);

   if (_IsQuestLoot(addedLoot.Identifier))
   {
      OnAuthorityQuestLootChanged.Broadcast(ETATInventoryUpdateEventType::Add);
   }
}

void UTATLootInventoryComponent::_AuthorityOnMajorLootRemoved(const FTATLootInstance& removedLoot)
{
   check(GetOwner()->HasAuthority());
   check(removedLoot.IsValid());

   // Ensure this is major loot
   check(UTATLootUtils::GetLootType(this, removedLoot.Identifier) == ETATLootType::MajorLoot);

   GetWorld()->GetTimerManager().ClearTimer(_carriedMajorLootGlyphSpawnTimerHandle);

   _UpdateMajorLootGameplayTags(false);
   _TrySendMajorLootDroppedToast(removedLoot);

   _OnMajorLootRemoved(removedLoot);

   if (_IsQuestLoot(removedLoot.Identifier))
   {
      OnAuthorityQuestLootChanged.Broadcast(ETATInventoryUpdateEventType::Remove);
   }
}

void UTATLootInventoryComponent::_TrySendMajorLootDroppedToast(const FTATLootInstance& removedLoot)
{
   if (_authorityDropping)
   {
      ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this);
      APlayerState* playerState = GetOwner<APlayerState>();
      if (toastBroadcaster && playerState)
      {
         toastBroadcaster->ClientToastBroadcast_PlayerDroppedMajorLoot(playerState, removedLoot.Identifier);
      }
   }
}

void UTATLootInventoryComponent::_OnMajorLootAdded(FTATLootInstance& addedLoot)
{
   // NB: Major loot is replicated to non-owning clients, but we shouldn't refresh loot bag fullness in that case
   // since minor loot will not be
   _RefreshLootBagFullnessIfAuthorityOrLocalPlayer();

   _AddMajorLootGameplayEffect();

   OnMajorLootDataChanged.Broadcast(FTATLootInstance(), addedLoot);

   OnInventoryUpdated.Broadcast(ETATInventoryUpdateEventType::Add, ETATLootType::MajorLoot);
   _NotifyInventoryChanged();
}

void UTATLootInventoryComponent::_OnMajorLootRemoved(const FTATLootInstance& removedLoot)
{
   // NB: Major loot is replicated to non-owning clients, but we shouldn't refresh loot bag fullness in that case
   // since minor loot will not be
   _RefreshLootBagFullnessIfAuthorityOrLocalPlayer();

   _RemoveMajorLootGameplayEffect();

   OnMajorLootDataChanged.Broadcast(removedLoot, FTATLootInstance());

   OnInventoryUpdated.Broadcast(ETATInventoryUpdateEventType::Remove, ETATLootType::MajorLoot);
   _NotifyInventoryChanged();
}

void UTATLootInventoryComponent::_OnLargeCarryLootAdded(const FTATLootInstance& addedLoot)
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, addedLoot.Identifier);
      lootInfo && lootInfo->LootType == ETATLootType::MajorLoot)
   {
      // TODO
      _AddMajorLootGameplayEffect();
      OnMajorLootDataChanged.Broadcast(FTATLootInstance(), addedLoot);
   }
   _AddLargeCarryTool();

   _NotifyInventoryChanged();
}

void UTATLootInventoryComponent::_OnLargeCarryLootRemoved(const FTATLootInstance& removedLoot)
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, removedLoot.Identifier);
       lootInfo && lootInfo->LootType == ETATLootType::MajorLoot)
   {
      _TrySendMajorLootDroppedToast(removedLoot);
      _RemoveMajorLootGameplayEffect();
      OnMajorLootDataChanged.Broadcast(removedLoot, FTATLootInstance());
   }
   _RemoveLargeCarryTool();

   _NotifyInventoryChanged();
}

void UTATLootInventoryComponent::_AddLargeCarryTool()
{
   if (_toolset == nullptr)
   {
      return;
   }

   check(HasLargeCarryLoot());

   const FTATLootIdentifier lootId = _largeCarriedLoot.Identifier;
   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   TSoftClassPtr<UTATLargeCarryLootToolComponent> toolClass = lootSettings.LargeCarryLootToolClass;

   const FTATLootInfo* lootInfo = lootSettings.GetLootInfo(this, lootId);
   check(lootInfo);

   TArray<FSoftObjectPath> pathsToLoad;
   pathsToLoad.Add(toolClass.ToSoftObjectPath());
   pathsToLoad.Add(lootInfo->CarriedLootMeshData.ToolMesh.ToSoftObjectPath());

   // CONSIDER: If there is a loading delay, should enforced side effects happen outside the tool
   //           so there isn't a gap?
   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakThis = MakeWeakObjectPtr(this), lootId, toolClass] {
      if (UTATLootInventoryComponent* self = weakThis.Get())
      {
         self->_AuthorityFinishAddingLargeCarryTool(toolClass, lootId);
      }
   });
}

void UTATLootInventoryComponent::_AuthorityFinishAddingLargeCarryTool(TSoftClassPtr<UTATLargeCarryLootToolComponent> toolClass, FTATLootIdentifier lootId)
{
   if (_toolset == nullptr || _largeCarriedLoot.Identifier != lootId)
   {
      return;
   }

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootId);
   if (!ensure(lootInfo))
   {
      return;
   }

   TSubclassOf<UTATLargeCarryLootToolComponent> loadedToolClass = toolClass.Get();
   if (!ensure(loadedToolClass))
   {
      return;
   }

   // could be firing multiple load callbacks
   if (_toolset->HasToolClass(loadedToolClass))
   {
      return;
   }

   UTATLargeCarryLootToolComponent* newTool = _toolset->AuthorityCreateToolClass(loadedToolClass);
   check(newTool);
   newTool->AuthoritySetLootParams(FTATLargeCarryLootToolParams{
      .LootId = lootId,
      .Mesh = lootInfo->CarriedLootMeshData.ToolMesh.Get()
   });

   _toolset->AuthorityAddTool(newTool);
}

void UTATLootInventoryComponent::_RemoveLargeCarryTool()
{
   if (_toolset == nullptr)
   {
      return;
   }

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   TSubclassOf<UTATLargeCarryLootToolComponent> toolClass = lootSettings.LargeCarryLootToolClass.Get();

   _toolset->AuthorityRemoveToolsOfClass(toolClass);
}

void UTATLootInventoryComponent::_NotifyInventoryChanged()
{
   if (!_batchNotifies)
   {
      _FireCoarseEvents();
   }
   else
   {
      _hasBatchedChanges = true;
   }
}

void UTATLootInventoryComponent::_FlushNotifies()
{
   check(_batchNotifies);

   if(_hasBatchedChanges)
   {
      _FireCoarseEvents();
      _hasBatchedChanges = false;
   }
   _batchNotifies = false;
}

void UTATLootInventoryComponent::_FireCoarseEvents()
{
   if (GetOwner()->HasAuthority())
   {
      OnAuthorityInventoryOrStashChanged.Broadcast();
   }

   // TODO: only do for local/authority
   _RefreshHeldLootValue();
}

void UTATLootInventoryComponent::_UpdateMajorLootGameplayTags(bool majorLootAdded)
{
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   const FGameplayTagContainer& majorLootTags = UTATLootSettings::Get().CharacterGrantedTagsWhenHoldingMajorLoot;
   if (asc == nullptr || majorLootTags.Num() == 0)
   {
      return;
   }

   if (majorLootAdded)
   {
      asc->AddLooseGameplayTags(majorLootTags);
   }
   else
   {
      asc->RemoveLooseGameplayTags(majorLootTags);
   }
}

bool UTATLootInventoryComponent::_IncrementPredictedLoot(const FTATLootIdentifier& lootId, int32 lootCount)
{
   if (const TOptional<int32> slotSize = UTATLootSettings::Get().GetSlotSize(this, lootId))
   {
      _predictedAdditionalUsedSlots = FMath::Max(0, _predictedAdditionalUsedSlots + (*slotSize * lootCount));
      return true;
   }
   return false;
}

void UTATLootInventoryComponent::_OnPredictedLootCatchup(FTATLootIdentifier lootId, int32 lootCount)
{
   _IncrementPredictedLoot(lootId, -lootCount);
}

bool UTATLootInventoryComponent::_IsQuestLoot(const FTATLootIdentifier& lootId) const
{
   if (const ATATPlayerState* TATPlayerState = Cast<ATATPlayerState>(GetOwner()))
   {
      return TATPlayerState->HasQuestRelatedLoot(lootId);
   }

   UE_LOG(LogTATLootInventory, Warning, TEXT("%s | _IsQuestLoot() failed due to not being able to get the TATPlayerState."), *GetOwner()->GetName());
   return false;
}

void UTATLootInventoryComponent::_AuthoritySetCarriedMajorLootGlyphActorSpawnTimer()
{
   if (!_carriedMajorLootGlyphIndicatorType.IsValid())
   {
      UE_LOG(LogTATLootInventory, Warning, TEXT("%s | _AuthoritySetCarriedMajorLootGlyphActorSpawnTimer() failed due to unassigned _carriedMajorLootGlyphIndicatorType!"), *GetOwner()->GetName());
      return;
   }

   // Set a looping timer that spawns a new instance of the transient glyph actor as soon as the previous instance's InitialLifeSpan has elapsed
   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   check(thiefVisionSubsystem != nullptr);
   const float glyphLifespan = thiefVisionSubsystem->GetThiefVisionIndicatorLifeSpan(_carriedMajorLootGlyphIndicatorType);
   if (glyphLifespan > 0.f)
   {
      const float delay = glyphLifespan + _glyphSpawnDelaySeconds;
      const bool looping = true;
      GetWorld()->GetTimerManager().SetTimer(_carriedMajorLootGlyphSpawnTimerHandle, this, &UTATLootInventoryComponent::_AuthoritySpawnCarriedMajorLootGlyphActor, delay, looping);
   }
   else
   {
      UE_LOG(LogTATLootInventory, Warning, TEXT("_AuthoritySetCarriedMajorLootGlyphActorSpawnTimer() | failed to set timer for spawning _carriedMajorLootGlyphIndicatorType, ")
         TEXT("due to %s having an IndicatorLifeSpan of %f! Assign it a positive value"), *_carriedMajorLootGlyphIndicatorType.ToString(), glyphLifespan);
   }
}

void UTATLootInventoryComponent::_AuthoritySpawnCarriedMajorLootGlyphActor()
{
   checkf(_carriedMajorLootGlyphIndicatorType.IsValid(), TEXT("_AuthoritySpawnCarriedMajorLootGlyphActor should never be called without a valid indicator type"));

   check(GetOwner()->HasAuthority());
   if (!HasMajorLoot())
   {
      UE_LOG(LogTATLootInventory, Warning, TEXT("%s | _AuthoritySpawnCarriedMajorLootGlyphActor() called with no major loot held!"), *GetOwner()->GetName());
      GetWorld()->GetTimerManager().ClearTimer(_carriedMajorLootGlyphSpawnTimerHandle);
      return;
   }

   const APawn* ownerPawn = _GetOwningPawn();
   if (!ownerPawn)
   {
      // Abort and unbind callback if pawn missing
      UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | _AuthoritySpawnCarriedMajorLootGlyphActor() called with no major loot held! Unbinding timer..."), *GetOwner()->GetName());
      GetWorld()->GetTimerManager().ClearTimer(_carriedMajorLootGlyphSpawnTimerHandle);
      return;
   }

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   if (thiefVisionSubsystem)
   {
      UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | _AuthoritySpawnCarriedMajorLootGlyphActor() spawning a transient glyph..."), *GetOwner()->GetName());

      const FTransform spawnTransform = ownerPawn->GetTransform();
      thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(_carriedMajorLootGlyphIndicatorType, spawnTransform);
   }
}

void UTATLootInventoryComponent::_AuthorityResetLootDamageAccumulator()
{
   check(GetOwner()->HasAuthority());

   _authorityLootDropDamageAccumulator = 0.f;
   UE_LOG(LogTATLootInventory, Verbose, TEXT("%s | reset loot drop damage accumulator!"), *GetOwner()->GetName());

   GetOwner()->GetWorldTimerManager().ClearTimer(_authorityLootDamageEventTimer);
}

void UTATLootInventoryComponent::_OnMinorLootChanged(ETATInventoryUpdateEventType eventType)
{
   _RefreshLootBagFullness();
   OnMinorLootChanged.Broadcast();

   OnInventoryUpdated.Broadcast(eventType, ETATLootType::MinorLoot);
   _NotifyInventoryChanged();
}

void UTATLootInventoryComponent::_RefreshLootBagFullness()
{
   // This component may be owned by guards for returning loot to origin
   if (const AOSEPlayerState* osePs = Cast<AOSEPlayerState>(GetOwner()))
   {
      // Should only be called for the local player, or the session host
      check(GetOwner()->HasAuthority() || osePs->IsLocalPlayerState());

      const float minorLootSlots = static_cast<float>(UTATLootSettings::Get().GetSlotSizeCombined(this, _minorLoot));
      const float majorLootSlots = static_cast<float>(UTATLootSettings::Get().GetSlotSizeCombined(this, _majorLoot));
      const float totalFullness = minorLootSlots + (majorLootSlots * MajorLootSlotVolumeCoefficient);

      // We require a "max slot count" to compute fullness, but the inventory doesn't have to have a slot limit, so add a fake limit here if needed for VFX/SFX purposes.
      const float maxFullness = static_cast<float>(FMath::Clamp(GetInventorySlotMaxCapacity(), 1, 64));
      const float maxFullnessNormalized = totalFullness / maxFullness;

      _lootBagFullness = FMath::Clamp(maxFullnessNormalized, 0.f, 1.f);
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _lootBagFullness, this);

      UE_LOG(LogTATLootInventory, Verbose, TEXT("Loot bag fullness => %f"), _lootBagFullness);
      OnLootBagFullnessChanged.Broadcast(_lootBagFullness);
   }
}

void UTATLootInventoryComponent::_RefreshLootBagFullnessIfAuthorityOrLocalPlayer()
{
   if (const AOSEPlayerState* osePs = Cast<AOSEPlayerState>(GetOwner()))
   {
      if (GetOwner()->HasAuthority() || osePs->IsLocalPlayerState())
      {
         _RefreshLootBagFullness();
      }
   }
}

void UTATLootInventoryComponent::_RefreshHeldLootValue()
{
   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   _heldLootTotalValue = lootSettings.GetLootValue(this, _minorLoot)
      + lootSettings.GetLootValue(this, _majorLoot)
      + lootSettings.GetLootValue(this, _largeCarriedLoot);

   if (GetOwner()->HasAuthority())
   {
      OnAuthorityLootValueChanged.Broadcast();
   }

   OnLootValueChanged.Broadcast();
}

void UTATLootInventoryComponent::_OnRep_MinorLoot(const TArray<FTATLootIdentifier>& oldValues)
{
   UE_LOG(LogTATLootInventory, Verbose, TEXT("_OnRep_MinorLoot() | %d -> %d items"), oldValues.Num(), _minorLoot.Num());

   ETATInventoryUpdateEventType updateType = ETATInventoryUpdateEventType::Update;
   if (_minorLoot.Num() > oldValues.Num())
   {
      updateType = ETATInventoryUpdateEventType::Add;
   }
   else if (_minorLoot.Num() < oldValues.Num())
   {
      updateType = ETATInventoryUpdateEventType::Remove;
   }

   _OnMinorLootChanged(updateType);
}

void UTATLootInventoryComponent::_OnRep_MajorLoot(const TArray<FTATLootInstance>& oldValue)
{
   if (!GetOwner()->HasAuthority())
   {
      const bool prevHeldMajorLoot = oldValue.Num() > 0;
      const bool curHeldMajorLoot = _majorLoot.Num() > 0;
      if (!prevHeldMajorLoot && curHeldMajorLoot)
      {
         // just picked up major loot
         _UpdateMajorLootGameplayTags(true);
      }
      else if (prevHeldMajorLoot && !curHeldMajorLoot)
      {
         // just dropped major loot
         _UpdateMajorLootGameplayTags(false);
      }
   }

   for (const FTATLootInstance& oldMajorLoot : oldValue)
   {
      const bool stillInInventory = _majorLoot.ContainsByPredicate([&](const FTATLootInstance& majorLootSlot)
      {
         return majorLootSlot.Id == oldMajorLoot.Id;
      });

      // Major loot removed
      if (!stillInInventory)
      {
         _OnMajorLootRemoved(oldMajorLoot);
      }
   }

   bool majorLootUpdated = false;

   for (FTATLootInstance& currentMajorLoot : _majorLoot)
   {
      const FTATLootInstance* previouslyExistingSlot = oldValue.FindByPredicate([&](const FTATLootInstance& majorLootSlot)
      {
         return majorLootSlot.Id == currentMajorLoot.Id;
      });

      // Major loot instance data changed
      if (previouslyExistingSlot != nullptr)
      {
         OnMajorLootDataChanged.Broadcast(*previouslyExistingSlot, currentMajorLoot);
         majorLootUpdated = true;
      }
      else
      {
         // New major loot added
         _OnMajorLootAdded(currentMajorLoot);
      }
   }

   if (majorLootUpdated)
   {
      OnInventoryUpdated.Broadcast(ETATInventoryUpdateEventType::Update, ETATLootType::MajorLoot);
      _NotifyInventoryChanged();
   }
}

void UTATLootInventoryComponent::_OnRep_LargeCarriedLoot(const FTATLootInstance& oldValue)
{
   if (oldValue.Identifier != _largeCarriedLoot.Identifier)
   {
      if (oldValue.Identifier.IsValid())
      {
         _OnLargeCarryLootRemoved(oldValue);
      }
      if (_largeCarriedLoot.Identifier.IsValid())
      {
         _OnLargeCarryLootAdded(_largeCarriedLoot);
      }
   }
}

void UTATLootInventoryComponent::_OnRep_LootBagFullness()
{
   OnLootBagFullnessChanged.Broadcast(_lootBagFullness);
}

FVector UTATLootInventoryComponent::_GenerateSuggestedLootDropLocation(const APawn* ownerPawn, ETATLootDropReason dropReason) const
{
   check(ownerPawn);

   FVector suggestedDropStart = FVector::ZeroVector;
   switch (dropReason)
   {
   case ETATLootDropReason::Voluntary:
      suggestedDropStart = UTATItemFunctionLibrary::FindSuggestedDropStartFromActorEyes(ownerPawn, _voluntaryDroppedLootSpawnRange, UTATProjectSettings::Get().ItemDropTraceProfile);
      break;

   case ETATLootDropReason::Involuntary:
   {
      // can be fancier later, but start on edge of a circle
      const float randomAngle = FMath::RandRange(0.f, 360.f);
      suggestedDropStart = ownerPawn->GetPawnViewLocation() + FVector(FVector2D(_involuntaryDroppedLootSpawnRadius, 0).GetRotated(randomAngle), 0);
      break;
   }

   default:
      checkNoEntry();
   }
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(AuraPerceiverTrace));
   queryParams.AddIgnoredActor(ownerPawn);
   if (GetWorld()->LineTraceTestByChannel(ownerPawn->GetActorLocation(), suggestedDropStart, ECC_Visibility, queryParams) == false)
   {
      return suggestedDropStart;
   }
   return ownerPawn->GetActorLocation();
}

FVector UTATLootInventoryComponent::_GenerateDroppedLootSpawnLocation(const AActor* lootActor, const FVector& suggestedDropStart, ETATLootDropReason dropReason) const
{
   // NB: May be null if the pawn has being destroyed before the loot actor loaded.
   // That is okay, we can still finish spawning the loot
   const APawn* owningPawn = _GetOwningPawn();

   FVector dropLocation = FVector::ZeroVector;
   UTATItemFunctionLibrary::FindDropLocationFromSuggestedStart(lootActor, owningPawn, UTATProjectSettings::Get().ItemDropTraceProfile, suggestedDropStart, dropLocation);
   return dropLocation;
}

void UTATLootInventoryComponent::_AddMajorLootGameplayEffect()
{
   if (!_gameplayEffectToApplyWhenHoldingMajorLoot || !GetOwner() || !GetOwner()->HasAuthority())
   {
      return;
   }

   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   check(asc != nullptr);

   // Add a stack of the major loot gameplay effect
   FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
   effectContext.AddInstigator(GetOwner(), GetOwner());
   asc->ApplyGameplayEffectToSelf(_gameplayEffectToApplyWhenHoldingMajorLoot.GetDefaultObject(), 0.0f, effectContext);
}

void UTATLootInventoryComponent::_RemoveMajorLootGameplayEffect()
{
   if (!_gameplayEffectToApplyWhenHoldingMajorLoot || !GetOwner() || !GetOwner()->HasAuthority())
   {
      return;
   }

   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   check(asc != nullptr);

   // Remove one stack of the first valid handle we find
   TArray<FActiveGameplayEffectHandle> gameplayEffectHandles = UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(asc, _gameplayEffectToApplyWhenHoldingMajorLoot);
   constexpr int32 stacksToRemove = 1;
   for (const FActiveGameplayEffectHandle& handle : gameplayEffectHandles)
   {
      if (asc->RemoveActiveGameplayEffect(handle, stacksToRemove))
      {
         break;
      }
   }
}

APawn* UTATLootInventoryComponent::_GetOwningPawn() const
{
   // AI will have their loot inventory directly on the pawn,
   // but players will have it on their player state. This helper handles either case
   return UOSECommon::GetPawn<APawn>(GetOwner());
}

void UTATLootInventoryComponent::_UpdateTeamStashedValue(int32 newStashedValue)
{
   if (_stashedLootTotalValue != newStashedValue)
   {
      const int previousValue = _stashedLootTotalValue;
      _stashedLootTotalValue = newStashedValue;
      OnStashedLootChanged.Broadcast(_stashedLootTotalValue, previousValue);
   }
}

FTATLootInventoryNotifyScope::FTATLootInventoryNotifyScope(UTATLootInventoryComponent* inventory)
   :_inventory(inventory)
{
   check(inventory);
   if (!inventory->_batchNotifies)
   {
      inventory->_batchNotifies = true;
      _ownsBatch = true;
   }
}

FTATLootInventoryNotifyScope::~FTATLootInventoryNotifyScope()
{
   if (_ownsBatch)
   {
      _inventory->_FlushNotifies();
   }
}
