// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Traversal/WireWrap/WireWrapQuery.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "WireWrapSegmentVisuals.generated.h"


USTRUCT()
struct FWireWrapSegmentVisualEntry
{
   GENERATED_BODY()

   UPROPERTY()
   TArray<UObject*> Visuals;

   int32 SegmentId;

   UPROPERTY()
   FWrapEndpoint Start;

   UPROPERTY()
   FWrapEndpoint End;

   void SetPrevious(const FWrapEndpoint& Previous);
   void SetNext(const FWrapEndpoint& Next);
};

USTRUCT()
struct FWireWrapVisualWrapper
{
   GENERATED_BODY()

   void AddSegment(int32 SegmentId, const FWrapEndpoint& Start, const FWrapEndpoint& End, TArrayView<UObject*> Visuals);

   void DestroyUpTo(int32 SegmentId);
   void Unwrap(int32 SegmentId);
   
private:
   using EntryType = FWireWrapSegmentVisualEntry;

   UPROPERTY()
   TArray<FWireWrapSegmentVisualEntry> Entries;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWireWrapSegmentVisuals : public UInterface
{
   GENERATED_BODY()
};

/**
 * 
 */
class OSECORE_API IWireWrapSegmentVisuals
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   // Called to initialize a segment
   UFUNCTION(BlueprintNativeEvent, Category = "Wrapping|Visuals|Segment")
   void InitSegment(int SegmentId, const FWrapEndpoint& Start, const FWrapEndpoint& End);

   // Called when segment is destroyed
   UFUNCTION(BlueprintNativeEvent, Category = "Wrapping|Visuals|Segment")
   void DestroySegment();

   // Called when segment is unwrapped
   UFUNCTION(BlueprintNativeEvent, Category = "Wrapping|Visuals|Segment")
   void UnwrapSegment();

   UFUNCTION(BlueprintNativeEvent, Category = "Wrapping|Visuals|Segment")
   void SetPreviousPoint(const FWrapEndpoint& Previous);

   UFUNCTION(BlueprintNativeEvent, Category = "Wrapping|Visuals|Segment")
   void SetNextPoint(const FWrapEndpoint& Next);
};
