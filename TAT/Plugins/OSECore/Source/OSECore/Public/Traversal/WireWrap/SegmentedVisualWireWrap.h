// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Traversal/WireWrap/WireWrap.h"
#include "Traversal/WireWrap/WireWrapSegmentVisuals.h"

// ue4
#include "CoreMinimal.h"

#include "SegmentedVisualWireWrap.generated.h"

/**
 * A subclass of WireWrap that manages objects for visuals per segment
 * 
 * I was going to just let the blueprint talk to the FWireWrapVisualWrapper itself, but :shrug:
 * TODO: better name?
 */
UCLASS(Blueprintable)
class OSECORE_API ASegmentedVisualWireWrap : public AWireWrap
{
   GENERATED_BODY()

public:
   ASegmentedVisualWireWrap();

   UFUNCTION(BlueprintCallable, Category=Wrapping)
   void DestroyVisuals();
   
protected:
   void OnSegmentAdded(int32 segmentId, const FWrapEndpoint& start, const FWrapEndpoint& end) override;
   void OnSegmentsDestroyed(int32 highestSegmentId) override;
   void OnSegmentUnwrapped(int32 unwrappedSegmentId) override;

   UFUNCTION(BlueprintImplementableEvent)
   void CreateVisualsForSegment(TArray<UObject*>& arrayToFill);

   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnSegmentsBroken();

   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnDestroyingVisuals();

private:
   UPROPERTY(Transient)
   FWireWrapVisualWrapper _segmentVisuals;

   UPROPERTY(EditDefaultsOnly, Category="Wrapping|Visuals")
   float _lifespanWhenDestroyingVisuals;

   UPROPERTY(Transient, ReplicatedUsing=OnRep_DestroyingVisuals);
   bool _bDestroyingVisuals;

   UFUNCTION()
   void OnRep_DestroyingVisuals();
};
