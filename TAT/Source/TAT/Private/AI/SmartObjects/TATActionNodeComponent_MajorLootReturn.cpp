// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/SmartObjects/TATActionNodeComponent_MajorLootReturn.h"
#include "Engine/AssetManager.h"

// tat
#include "AI/Target/TATTargetingGroups.h"
#include "Developer/TATLootSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/TATLootTypes.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootActor.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent_MajorLootReturn)

void FLootReturnSlot::TakeSlot(const AActor* itemTakingSlot)
{
   check(CanUseSlot());
   _itemInSlot = itemTakingSlot;
   _bIsReservationPending = false;
}

void FLootReturnSlot::ReleaseSlot()
{
   _itemInSlot = nullptr;
   _bIsReservationPending = false;
}

FGameplayTag UTATActionNodeComponent_MajorLootReturn::GetUtilityAITargetingGroup() const
{
  return _isFull ? TAG_AI_TargetingGroup_SmartObject_MajorLootReturn_Full : TAG_AI_TargetingGroup_SmartObject_MajorLootReturn_HasSpace;
}

void UTATActionNodeComponent_MajorLootReturn::PlaceLoot(UTATLootInventoryComponent* lootInventoryComponent)
{
   check(lootInventoryComponent);
   checkf(lootInventoryComponent->GetMajorLootCount() >= 1, TEXT("Expected at least one piece of major loot, got %d"), lootInventoryComponent->GetMajorLootCount());
   const FTATLootInstance majorLootInstance = lootInventoryComponent->GetMajorLootRefByIndex(0);
   check(majorLootInstance.IsValid());

   const int32 slotIndex = _lootSlots.IndexOfByPredicate([](const FLootReturnSlot& slot){ return slot.CanUseSlot(); });
   if(slotIndex == INDEX_NONE)
      return;

   lootInventoryComponent->AuthorityRemoveSingleMajorLootWithoutDropping(majorLootInstance.Id);

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   const FTATLootInfo* lootInfo = lootSettings.GetLootInfo(this, majorLootInstance.Identifier);
   check(lootInfo);

   // Make sure we have a loot actor class to spawn
   if (lootInfo->ActorClass.IsNull())
   {
      UE_LOG(LogTemp, Error, TEXT("ATATMajorLootReturnLocation::PlaceLoot() called with %s whose FTATLootInfo entry has unassigned ActorClass!"), *majorLootInstance.ToString());
      return;
   }

   UE_LOG(LogTemp, Verbose, TEXT("Placing item into Major Loot Return location - item %s..."), *lootInfo->DisplayName.ToString());

   // Spawn instance of loot actor at player's location
   TWeakObjectPtr<UTATActionNodeComponent_MajorLootReturn> weakThis(this);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(lootInfo->ActorClass.ToSoftObjectPath(), [weakThis, lootInfo, slotIndex, majorLootInstance]
      {
         if (weakThis.IsValid())
         {
            weakThis->_SpawnMajorLootActor(lootInfo->ActorClass.Get(), slotIndex, majorLootInstance);
         }
      });
}

void UTATActionNodeComponent_MajorLootReturn::_OnLootDestroyed(AActor* destroyedActor)
{
   const int32 slotIndex = _lootSlots.IndexOfByPredicate([destroyedActor](const FLootReturnSlot& slot){ return slot.IsSlotTakenByActor(destroyedActor); });
   if(slotIndex == INDEX_NONE)
   {
      UE_LOG(LogTemp, Error, TEXT("Loot was destroyed but the slot wasn't by the actor"));
      return;
   }
   _lootSlots[slotIndex].ReleaseSlot();
   _isFull = false; 
}

void UTATActionNodeComponent_MajorLootReturn::_SpawnMajorLootActor(TSubclassOf<ATATLootActor> lootClass, int32 slotIndex, const FTATLootInstance& lootInstance)
{
   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   FTransform spawnTransform;
   if (ATATLootActor* lootActor = GetWorld()->SpawnActorDeferred<ATATLootActor>(lootClass, spawnTransform, owner, instigator, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
   {
      // Move to spawn location
      const FVector spawnLocation = GetOwner()->GetActorTransform().TransformPosition(_lootSlots[slotIndex].Location);
      spawnTransform = FTransform(spawnLocation);

      lootActor->LootInstance = lootInstance;
      lootActor->PlacementState = ETATLootPlacementState::Returned;
      
      UGameplayStatics::FinishSpawningActor(lootActor, spawnTransform);

      lootActor->OnDestroyed.AddDynamic(this, &ThisClass::_OnLootDestroyed);
      _lootSlots[slotIndex].TakeSlot(lootActor);
      
      if(_lootSlots.ContainsByPredicate([](const FLootReturnSlot& slot){ return slot.CanUseSlot(); }))
      {
         // If there are no remaining slots, stop AI from trying to return here
         _isFull = true;    
      }
   }
   else
   {
      _lootSlots[slotIndex].ReleaseSlot();
   }
}
