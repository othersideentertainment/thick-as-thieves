// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "TATActionNodeComponent.h"

// self
#include "TATActionNodeComponent_MajorLootReturn.generated.h"

class UTATLootInventoryComponent;
class ATATLootActor;
struct FTATLootInstance;

USTRUCT()
struct FLootReturnSlot
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category = "Slot", meta = (MakeEditWidget = true))
   FVector Location { FVector::ZeroVector };

   void Reserve() { _bIsReservationPending = true; }
   bool CanUseSlot() const { return _itemInSlot == nullptr && _bIsReservationPending == false; }

   bool IsSlotTakenByActor(const AActor* actor) const  { return _itemInSlot == actor; }
   
   void TakeSlot(const AActor* itemTakingSlot);
   void ReleaseSlot();

private:
   UPROPERTY(Transient)
   const AActor* _itemInSlot { nullptr };

   // As the actor spawning is async, we want to reserve the slot ahead of time
   bool _bIsReservationPending { false };
};

UCLASS(Blueprintable, BlueprintType, ClassGroup = Gameplay, meta = (BlueprintSpawnableComponent), config = Game,
   HideCategories = (Activation, AssetUserData, Collision, Cooking, HLOD, Lighting, LOD, Mobile, Mobility, Navigation,
      Physics, RayTracing, Rendering, Tags, TextureStreaming), AutoExpandCategories = ("AI|TAT|ActionNode"))
class TAT_API UTATActionNodeComponent_MajorLootReturn : public UTATActionNodeComponent 
   , public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()

public:
   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;

   UFUNCTION(BlueprintCallable)
   void PlaceLoot(UTATLootInventoryComponent* lootInventoryComponent);

protected:
   UPROPERTY(EditDefaultsOnly, Category="TAT|Slots")
   TArray<FLootReturnSlot> _lootSlots;

   UFUNCTION()
   void _OnLootDestroyed(AActor* destroyedActor);
   void _SpawnMajorLootActor(TSubclassOf<ATATLootActor> lootClass, int32 slotIndex, const FTATLootInstance& lootInstance);

   bool _isFull { false };
};
