// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Traversal/WireWrap/WireWrapQuery.h"

//UE includes
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"


#include "WireWrap.generated.h"

USTRUCT(BlueprintType)
struct FWireWrapSegment : public FFastArraySerializerItem
{
   GENERATED_USTRUCT_BODY()

   // The end position of this segment
   UPROPERTY(BlueprintReadOnly)
   FWrapEndpoint Point;

   // The initial length of the segment
   UPROPERTY()
   float Length = 0;

   // The replication id of the previousSegment, in case things are out of order
   UPROPERTY()
   int32 PreviousSegmentId = 0;

   // The replication id of the previousSegment, in case things are out of order
   UPROPERTY()
   float CreateServerWorldTime = 0;

   bool bJustAdded;

   FVector GetWorldPosition() const { return Point.ToWorldPosition(); }

   void PostReplicatedAdd(const FFastArraySerializer& Serializer);
};

USTRUCT()
struct FWireWrapSegmentArray : public FFastArraySerializer
{
   GENERATED_USTRUCT_BODY()

   UPROPERTY()
   TArray<FWireWrapSegment> Items;

   AWireWrap* Owner;

   bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
   {
      return FFastArraySerializer::FastArrayDeltaSerialize<FWireWrapSegment, FWireWrapSegmentArray>(Items, DeltaParms, *this);
   }

   void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits< FWireWrapSegmentArray > : public TStructOpsTypeTraitsBase2< FWireWrapSegmentArray >
{
   enum
   {
      WithNetDeltaSerializer = true,
   };
};

// Configuration settings about segments
USTRUCT(BlueprintType)
struct OSECORE_API FWrapSegmentSettings
{
   GENERATED_USTRUCT_BODY()

   // The maximum allowed relative difference in length between the current and initial lengths of a segment
   UPROPERTY(EditDefaultsOnly)
   float MaxPercentLengthChange = 0.25;

   // The maximum allowed absolute difference in length between the current and initial lengths of a segment
   UPROPERTY(EditDefaultsOnly)
   float MaxAbsoluteLengthChange = 100;

   // Lifespan of segments in seconds. 0 means an infinite lifespan
   UPROPERTY(EditDefaultsOnly)
   float SegmentLifespan = 0;

   // Whether to create segments at all, or just move the endpoint
   UPROPERTY(EditDefaultsOnly)
   bool bCreateSegments = true;

   // Whether to allow unwrapping of segments
   // TODO: Is this the best place for this?
   UPROPERTY(EditDefaultsOnly)
   bool bUnwrapSegments = true;
};

UCLASS(Blueprintable)
class OSECORE_API AWireWrap : public AActor
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   AWireWrap();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   virtual void PostInitializeComponents() override;

public:   
   // Called every frame
   virtual void Tick(float DeltaTime) override;

   UFUNCTION(BlueprintCallable, Category = Wrapping)
   void StartWrapping(AActor* StartActor, const FVector& StartPosition, const FWrapEndpoint& Endpoint);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Wrapping)
   void PauseWrapping();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Wrapping)
   void ResumeWrapping(bool forgetPreviousEndpoint);

   // Gets the last fixed wrap point in the wire
   UFUNCTION(BlueprintCallable, Category=Wrapping)
   FVector GetLastSegmentPoint() const;

   // Gets the actor the fixed wrap point in the wire is attached to (if any)
   UFUNCTION(BlueprintCallable, Category = Wrapping)
   AActor* GetLastSegmentActor() const;

   // Gets the wrapped length in the wire, in cm
   // Does not change if attached to moving actors
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   float GetTotalSegmentLength() const;

   // Gets the current world position of the segment
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   static FVector GetWorldPosition(const FWireWrapSegment& Segment);

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   static bool IsValidSegment(const FWireWrapSegment& Segment);

   // Gets the current world position of the segment
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   static FVector GetWorldPositionForEndpoint(const FWrapEndpoint& Point);

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   static bool IsValidEndpoint(const FWrapEndpoint& Point);

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = Wrapping)
   static FWrapEndpoint MakeActorRelativeEndpoint(AActor* Actor, FVector Offset);

protected:
   UFUNCTION(BlueprintCallable, Category = Wrapping, Meta = (BlueprintProtected))
   const TArray<FWireWrapSegment>& GetSegments() const;

   // The current position of the movable end-point
   UFUNCTION(BlueprintCallable, Category = Wrapping, Meta = (BlueprintProtected))
   FVector GetEndPosition() const;

   UFUNCTION(BlueprintCallable, Category = Wrapping, Meta = (BlueprintProtected))
   AActor* GetEndActor() const;

   UFUNCTION(BlueprintImplementableEvent)
   void HandleWrapChanged();

   virtual void OnSegmentAdded(int32 SegmentId, const FWrapEndpoint& Start, const FWrapEndpoint& End) {};
   virtual void OnSegmentsDestroyed(int32 HighestSegmentId) {}
   virtual void OnSegmentUnwrapped(int32 UnwrappedSegmentId) {}

private:
   bool CheckForWrap(const FVector& CurrentEnD);
   bool CheckForUnwrap(FVector CurrentEnd);
   bool CheckForBreak();
   void BreakSegmentsUpTo(int segmentIndex);
   void DoUnwrap();
   void FireWrapChanged();

   void RecalculateSegmentLength();
   bool ShouldBreakWithTime(const FWireWrapSegment& Segment, float CurrentTime) const;

   UFUNCTION()
   void OnRep_Segments();

   friend struct FWireWrapSegmentArray;
   void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices);

private:
   UPROPERTY(Transient, Replicated)
   FWrapEndpoint Endpoint;

   UPROPERTY(Transient)
   FVector PreviousEndpoint;

   UPROPERTY(Transient)
   float TotalSegmentLength;
   
   UPROPERTY(Transient, ReplicatedUsing=OnRep_Segments)
   FWireWrapSegmentArray Segments;

   UPROPERTY(Transient)
   TArray<AActor*> ActorsToIgnore;

protected:
   UPROPERTY(EditDefaultsOnly, Category=Wrapping)
   FWrapSettings WrapSettings;

   UPROPERTY(EditDefaultsOnly, Category=Wrapping)
   FWrapSegmentSettings SegmentSettings;

   bool _bStopWrapping;
};
