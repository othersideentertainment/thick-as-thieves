// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "TATActionNodeComponent.h"

// self
#include "TATActionNodeComponent_Ambient.generated.h"


UCLASS(Blueprintable, BlueprintType, ClassGroup = Gameplay, meta = (BlueprintSpawnableComponent), config = Game,
   HideCategories = (Activation, AssetUserData, Collision, Cooking, HLOD, Lighting, LOD, Mobile, Mobility, Navigation,
      Physics, RayTracing, Rendering, Tags, TextureStreaming), AutoExpandCategories = ("AI|TAT|ActionNode"))
class TAT_API UTATActionNodeComponent_Ambient : public UTATActionNodeComponent 
   , public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATActionNodeComponent_Ambient(const FObjectInitializer& objectInitializer);
   
   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;

   UFUNCTION(BlueprintCallable)
   bool HasMoreThanOneSlot() const;
   
   UFUNCTION(BlueprintCallable)
   bool HasMoreThanOneSlotBeenOccupied() const;
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotChanged, ESmartObjectChangeReason, Reason, FSmartObjectSlotHandle, SlotHandle);
   UPROPERTY(BlueprintAssignable)
   FOnSlotChanged OnSlotChanged; 
protected:
   virtual void _HandleSlotEvent(const FSmartObjectEventData& smartObjectEventData) override;

   TArray<FSmartObjectSlotHandle> _occupiedSlots;
};
