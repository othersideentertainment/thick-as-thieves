// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/WireWrap/WireWrapSegmentVisuals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WireWrapSegmentVisuals)

// Add default functionality here for any IWireWrapSegmentVisuals functions that are not pure virtual.

void FWireWrapVisualWrapper::AddSegment(int32 segmentId, const FWrapEndpoint& start, const FWrapEndpoint& end, TArrayView<UObject*> visuals)
{
   // assume that the interface can buffer and add these in order
   check(Entries.Num() == 0 || Entries.Last().SegmentId < segmentId);

   EntryType* entry = new(Entries) EntryType;
   entry->SegmentId = segmentId;
   entry->Start = start;
   entry->End = end;
   entry->Visuals.Append(visuals.GetData(), visuals.Num());

   for (UObject* segmentVisual : visuals)
   {
      IWireWrapSegmentVisuals::Execute_InitSegment(segmentVisual, segmentId, start, end);
   }

   if (Entries.Num() >= 2)
   {
      FWireWrapSegmentVisualEntry& previous = Entries.Last(1);
      previous.SetNext(end);
      entry->SetPrevious(previous.Start);
   }
}

void FWireWrapVisualWrapper::DestroyUpTo(int32 SegmentId)
{
   int index = 0;
   const int max = Entries.Num();
   for (; index < max; ++index)
   {
      EntryType& entry = Entries[index];
      if (entry.SegmentId > SegmentId)
      {
         break;
      }

      for (UObject* segmentVisual : entry.Visuals)
      {
         IWireWrapSegmentVisuals::Execute_DestroySegment(segmentVisual);
      }
   }

   Entries.RemoveAt(0, index);

   if (Entries.Num() > 0)
   {
      Entries[0].SetPrevious(FWrapEndpoint::Empty());
   }
}

void FWireWrapVisualWrapper::Unwrap(int32 segmentId)
{
   int32 foundIndex = Entries.FindLastByPredicate([segmentId](const auto& s) { return s.SegmentId == segmentId; });
   if(foundIndex >= 0)
   {
      EntryType& entry = Entries[foundIndex];
      for (UObject* segmentVisual : entry.Visuals)
      {
         IWireWrapSegmentVisuals::Execute_UnwrapSegment(segmentVisual);
      }
      Entries.RemoveAt(foundIndex);

      if (foundIndex - 1 >= 0)
      {
         Entries[foundIndex - 1].SetNext(FWrapEndpoint::Empty());
      }
   }
}

void FWireWrapSegmentVisualEntry::SetPrevious(const FWrapEndpoint& previous)
{
   for (UObject* segmentVisual : Visuals)
   {
      IWireWrapSegmentVisuals::Execute_SetPreviousPoint(segmentVisual, previous);
   }
}

void FWireWrapSegmentVisualEntry::SetNext(const FWrapEndpoint& next)
{
   for (UObject* segmentVisual : Visuals)
   {
      IWireWrapSegmentVisuals::Execute_SetNextPoint(segmentVisual, next);
   }
}

