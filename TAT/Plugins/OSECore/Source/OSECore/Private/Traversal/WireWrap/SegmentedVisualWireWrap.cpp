// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/WireWrap/SegmentedVisualWireWrap.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SegmentedVisualWireWrap)

ASegmentedVisualWireWrap::ASegmentedVisualWireWrap()
{
   _lifespanWhenDestroyingVisuals = 1;
}

void ASegmentedVisualWireWrap::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ASegmentedVisualWireWrap, _bDestroyingVisuals);
}

void ASegmentedVisualWireWrap::DestroyVisuals()
{
   this->SetLifeSpan(_lifespanWhenDestroyingVisuals);
   _bDestroyingVisuals = true;
   OnRep_DestroyingVisuals();
}

void ASegmentedVisualWireWrap::OnSegmentAdded(int32 segmentId, const FWrapEndpoint& start, const FWrapEndpoint& end)
{
   TArray<UObject*> visualsForSegment;
   CreateVisualsForSegment(visualsForSegment);
   _segmentVisuals.AddSegment(segmentId, start, end, visualsForSegment);
}

void ASegmentedVisualWireWrap::OnSegmentsDestroyed(int32 highestSegmentId)
{
   _segmentVisuals.DestroyUpTo(highestSegmentId);
   BP_OnSegmentsBroken();
}

void ASegmentedVisualWireWrap::OnSegmentUnwrapped(int32 unwrappedSegmentId)
{
   _segmentVisuals.Unwrap(unwrappedSegmentId);
}

void ASegmentedVisualWireWrap::OnRep_DestroyingVisuals()
{
   if (_bDestroyingVisuals)
   {
      _bStopWrapping = true;
      _segmentVisuals.DestroyUpTo(MAX_int32);
      BP_OnDestroyingVisuals();
   }
}

