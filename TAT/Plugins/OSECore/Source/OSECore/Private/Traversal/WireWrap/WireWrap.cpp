// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/WireWrap/WireWrap.h"

// ue4
#include "Net/UnrealNetwork.h"
#include "Algo/Accumulate.h"
#include "Algo/IsSorted.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WireWrap)

// Sets default values
AWireWrap::AWireWrap()
{
   // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true;
   bReplicates = true;
}

// Called when the game starts or when spawned
void AWireWrap::BeginPlay()
{
   Super::BeginPlay();

   SetActorTickEnabled(Endpoint.Actor != nullptr);
}

void AWireWrap::PostInitializeComponents()
{
   Super::PostInitializeComponents();
   Segments.Owner = this;
}

void AWireWrap::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(AWireWrap, Segments);
   DOREPLIFETIME(AWireWrap, Endpoint);
}

FVector AWireWrap::GetEndPosition() const
{
   return Endpoint.Actor->GetActorTransform().TransformPosition(Endpoint.Offset);
}

AActor* AWireWrap::GetEndActor() const
{
   return Endpoint.Actor;
}

float AWireWrap::GetTotalSegmentLength() const
{
   return TotalSegmentLength;
}

FVector AWireWrap::GetWorldPosition(const FWireWrapSegment& Segment)
{
   return Segment.GetWorldPosition();
}

bool AWireWrap::IsValidSegment(const FWireWrapSegment& Segment)
{
   return Segment.Point.IsValid();
}

FVector AWireWrap::GetWorldPositionForEndpoint(const FWrapEndpoint& Point)
{
   return Point.ToWorldPosition();
}

bool AWireWrap::IsValidEndpoint(const FWrapEndpoint& Point)
{
   return Point.IsValid();
}

FWrapEndpoint AWireWrap::MakeActorRelativeEndpoint(AActor* Actor, FVector Offset)
{
   FWrapEndpoint Result;
   Result.Actor = Actor;
   Result.Offset = Offset;
   Result.Type = EWrapEndpointType::ActorRelative;
   return Result;
}

void AWireWrap::FireWrapChanged()
{
   RecalculateSegmentLength();
   HandleWrapChanged();
}

void AWireWrap::OnRep_Segments()
{
   // When entries are removed, the order is not preserved, so sort in that case
   Algo::SortBy(Segments.Items, [](const auto& s) { return s.ReplicationID; });

   int32 FirstAddedIndex = 1 + Segments.Items.FindLastByPredicate([](const auto& s) { return !s.bJustAdded; });
   if (Segments.Items.IsValidIndex(FirstAddedIndex))
   {
      // TODO: handle out of order replication? (e.g. sort/merged added ones)
      const int SegmentCount = Segments.Items.Num();
      for (int i = FMath::Max(1, FirstAddedIndex); i < SegmentCount; ++i)
      {
         FWireWrapSegment& Segment = Segments.Items[i];
         const FWireWrapSegment& PreviousSegment = Segments.Items[i - 1];

         // TODO: check if previous matches segment id
         OnSegmentAdded(Segment.ReplicationID, PreviousSegment.Point, Segment.Point);
         Segment.bJustAdded = false;
      }
   }

   FireWrapChanged();
}

// Called every frame
void AWireWrap::Tick(float DeltaTime)
{
   Super::Tick(DeltaTime);

   if(Endpoint.Actor == nullptr || !HasAuthority() || _bStopWrapping)
   {
      return;
   }

   FVector CurrentEnd = GetEndPosition();
   bool bHasChanged = false;
   bHasChanged |= CheckForBreak();
   bHasChanged |= CheckForWrap(CurrentEnd);
   bHasChanged |= CheckForUnwrap(CurrentEnd);

   PreviousEndpoint = CurrentEnd;

   if(bHasChanged)
   {
      FireWrapChanged();
   }
}

static bool ShouldBreakWithDistance(const FWrapSegmentSettings& Settings, float InitialLength, float CurrentLengthSqr)
{
   const float Tolerance = FMath::Min(Settings.MaxAbsoluteLengthChange, InitialLength * Settings.MaxPercentLengthChange);
   const float MinSqr = FMath::Square(InitialLength - Tolerance);
   const float MaxSqr = FMath::Square(InitialLength + Tolerance);
   return !FMath::IsWithinInclusive(CurrentLengthSqr, MinSqr, MaxSqr);
}

bool AWireWrap::ShouldBreakWithTime(const FWireWrapSegment& Segment, float CurrentTime) const
{
   return SegmentSettings.SegmentLifespan > 0 && (Segment.CreateServerWorldTime + SegmentSettings.SegmentLifespan) < CurrentTime;
}

bool AWireWrap::CheckForBreak()
{
   const float CurrentTime = GetWorld()->GetTimeSeconds();
   bool dirty = false;

   for (int i = Segments.Items.Num()-1; i > 0; --i)
   {
      FWireWrapSegment& Segment = Segments.Items[i];
      if (!Segment.Point.IsValid())
      {
         // if not the last segment, just break them
         if (Segments.Items.IsValidIndex(i + 1))
         {
            BreakSegmentsUpTo(i + 1);
            return true;
         }
         else if(i > 0) // not the first segment, just unwrap?
         {
            // TODO: try to re-wrap based on delta between old point on new one? (would have to account for consecutive unwraps properly)
            check(i == Segments.Items.Num() - 1);
            DoUnwrap();
            dirty = true;
            continue;
         }
         // TODO: the rest of it?
      }

      const FVector PreviousSegmentEnd = Segments.Items[i-1].GetWorldPosition(); // Possibly redundant, but simpler
      const FVector SegmentEnd = Segment.GetWorldPosition();
      float CurrentLengthSqr = FVector::DistSquared(SegmentEnd, PreviousSegmentEnd);
      if (ShouldBreakWithDistance(SegmentSettings, Segment.Length, CurrentLengthSqr) || ShouldBreakWithTime(Segment, CurrentTime))
      {
         // "break" all segments prior to this one, making this the new root
         BreakSegmentsUpTo(i);
         return true;
      }
   }

   return dirty;
}

void AWireWrap::BreakSegmentsUpTo(int segmentIndex)
{
   FWireWrapSegment& Segment = Segments.Items[segmentIndex];
   TArrayView<FWireWrapSegment> SliceToRemove(Segments.Items.GetData(), segmentIndex);
   Segment.Length += Algo::TransformAccumulate(SliceToRemove, [](const auto& s) { return s.Length; }, 0.f);
   Segment.PreviousSegmentId = 0;
   Segments.MarkItemDirty(Segment);
   OnSegmentsDestroyed(Segment.ReplicationID);

   Segments.Items.RemoveAt(0, segmentIndex, EAllowShrinking::No);
}

bool AWireWrap::CheckForWrap(const FVector& CurrentEnd)
{
   TArray<FWrapActorAndPosition> Results;
   if (!WireWrapQuery::ComputeWireWrap(this,
      WrapSettings, ActorsToIgnore, Segments.Items.Last().GetWorldPosition(),
      CurrentEnd, PreviousEndpoint, Results))
   {
      return false;
   }

   // TODO: other stuff
   if (SegmentSettings.bCreateSegments)
   {
      const float CurrentTime = GetWorld()->GetTimeSeconds();
      for (const FWrapActorAndPosition& NewPoint : Results)
      {
         auto Segment = new(Segments.Items) FWireWrapSegment;
         const FWireWrapSegment& PreviousSegment = Segments.Items.Last(1);
         Segment->Point = NewPoint.ToEndpoint();
         Segment->Length = FVector::Dist(NewPoint.WorldPosition, PreviousSegment.GetWorldPosition());
         Segment->PreviousSegmentId = PreviousSegment.ReplicationID;
         Segment->CreateServerWorldTime = CurrentTime;
         Segments.MarkItemDirty(*Segment);
         OnSegmentAdded(Segment->ReplicationID, PreviousSegment.Point, Segment->Point);
      }
   }
   else
   {
      // If not creating segments, update last segment and ratchet length
      // TODO: most of the logic in this class is book-keeping for segments. If there is a
      //       is a use-case for no segments at all, should this just be a separate class that
      //       uses the same wrap-query, but has to do less work?
      check(FMath::IsNearlyEqual(Segments.Items.Last().Length, TotalSegmentLength));

      const FWrapActorAndPosition& NewPoint = Results.Last();
      auto& Segment = Segments.Items.Last();
      const FVector PreviousSegmentPosition = Segment.GetWorldPosition();
      Segment.Point = NewPoint.ToEndpoint();
      Segment.Length = TotalSegmentLength + FVector::Dist(NewPoint.WorldPosition, PreviousSegmentPosition);
      Segment.PreviousSegmentId = 0;
      Segments.MarkItemDirty(Segment);
   }
   return true;
}

bool AWireWrap::CheckForUnwrap(FVector CurrentEnd)
{
   if (!SegmentSettings.bUnwrapSegments || Segments.Items.Num() < 2) return false;

   if (WireWrapQuery::CanUnwrap(this, WrapSettings, ActorsToIgnore, Segments.Items.Last(1).GetWorldPosition(), Segments.Items.Last().GetWorldPosition(), CurrentEnd))
   {
      DoUnwrap();
      return true;
   }

   return false;
}

void AWireWrap::DoUnwrap()
{
   OnSegmentUnwrapped(Segments.Items.Last().ReplicationID);
   Segments.Items.Pop(EAllowShrinking::No);
   Segments.MarkArrayDirty();
}

void AWireWrap::StartWrapping(AActor* StartActor, const FVector& StartPosition, const FWrapEndpoint& InEndpoint)
{
   Segments.Items.Reset();
   auto Segment = new(Segments.Items) FWireWrapSegment;
   Segment->Point = FWrapEndpoint::FromWorldPosition(StartActor, StartPosition);
   Segments.MarkItemDirty(*Segment);

   ActorsToIgnore.Reset();
   ActorsToIgnore.Add(this);
   ActorsToIgnore.Add(InEndpoint.Actor);
   
   Endpoint = InEndpoint;
   SetActorTickEnabled(true);
}

void AWireWrap::PauseWrapping()
{
   check(HasAuthority());

   SetActorTickEnabled(false);
}

void AWireWrap::ResumeWrapping(bool forgetPreviousEndpoint)
{
   check(HasAuthority());

   // skip if stopped for other reasons
   if(_bStopWrapping) return;

   SetActorTickEnabled(true);

   if (forgetPreviousEndpoint)
   {
      PreviousEndpoint = GetEndPosition();
   }
}

FVector AWireWrap::GetLastSegmentPoint() const
{
   check(Segments.Items.Num() > 0);
   return Segments.Items.Last().GetWorldPosition();
}

AActor* AWireWrap::GetLastSegmentActor() const
{
   check(Segments.Items.Num() > 0);
   return Segments.Items.Last().Point.Actor;
}

const TArray<FWireWrapSegment>& AWireWrap::GetSegments() const
{
   return Segments.Items;
}

void AWireWrap::RecalculateSegmentLength()
{
   float Total = 0;
   for(const FWireWrapSegment& Segment : Segments.Items)
   {
      Total += Segment.Length;
   }
   TotalSegmentLength = Total;
}

void AWireWrap::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices)
{
   check(Algo::IsSorted(RemovedIndices));

   int32 LastContiguousIndex = -1;
   for (int32 RemovedIndex : RemovedIndices)
   {
      if (LastContiguousIndex + 1 == RemovedIndex)
      {
         LastContiguousIndex = RemovedIndex;
      }
      else
      {
         // Assume non-contiguous removal must have been an unwrap
         OnSegmentUnwrapped(Segments.Items[RemovedIndex].ReplicationID);
      }
   }

   // Destroy contiguous ones in one go
   if (LastContiguousIndex >= 0 && LastContiguousIndex + 1 < Segments.Items.Num())
   {
      OnSegmentsDestroyed(Segments.Items[LastContiguousIndex + 1].ReplicationID);
   }
}

void FWireWrapSegmentArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
   check(Owner);
   if (Owner)
   {
      Owner->PreReplicatedRemove(RemovedIndices);
   }
}

void FWireWrapSegment::PostReplicatedAdd(const FFastArraySerializer& Serializer)
{
   bJustAdded = true;
}

