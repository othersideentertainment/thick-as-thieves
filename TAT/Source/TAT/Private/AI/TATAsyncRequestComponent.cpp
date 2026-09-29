// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/TATAsyncRequestComponent.h"

// ose
#include "OSECoreCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAsyncRequestComponent)

UTATAsyncRequestComponent::UTATAsyncRequestComponent()
{
   // Ticking is used in pruning request data.
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.TickInterval = _requestPruneInterval;
}

void UTATAsyncRequestComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   const float now = GetWorld()->GetTimeSeconds();
   for (auto it = _asyncLineTraces.CreateIterator(); it; ++it)
   {
      // Only prune data if it has been some time since it was last asked for
      // rather than checking the age of the data itself. The requester can
      // determine staleness with FTATAsyncRequestContext::Interval.
      const FAsyncRequestDataCache& data = it.Value();
      const float timeSinceLastRequest = (now - data.LastRequestTime);
      const bool shouldRemove = !data.IsInFlight() && (timeSinceLastRequest > _requestPruneThreshold);
      if (shouldRemove)
      {
         it.RemoveCurrent();
      }
   }

   if (_asyncLineTraces.IsEmpty())
   {
      // No entries left to check, no need to tick anymore.
      SetComponentTickEnabled(false);
   }
}

const UTATAsyncRequestComponent::FAsyncRequestDataCache& UTATAsyncRequestComponent::RequestAsyncLineTraceData(const FTATAsyncTraceRequestContext& context)
{
   check(context.IsValid());

   // Create new or find existing data for a given trace request.
   const FAsyncRequestHandle handle = context.GetHandle();
   FAsyncRequestDataCache* dataCache = _asyncLineTraces.Find(handle);
   if (dataCache == nullptr)
   {
      dataCache = &_asyncLineTraces.Add(handle);

      // Enable ticking when _asyncLineTraces has entries to check for pruning.
      SetComponentTickEnabled(true);
   }
   check(dataCache != nullptr);

   // Log the last time this information was requested (use in pruning checks).
   dataCache->LastRequestTime = GetWorld()->GetTimeSeconds();

   // Check if we should start a new async trace.
   const float now = GetWorld()->GetTimeSeconds();
   const float timeSinceLastUpdated = (now - dataCache->LastUpdatedTime);
   if (!dataCache->IsInFlight() && (timeSinceLastUpdated >= context.Interval))
   {
      _TriggerAsyncLineTrace(context, *dataCache);
   }

   return *dataCache;
}

void UTATAsyncRequestComponent::_TriggerAsyncLineTrace(const FTATAsyncTraceRequestContext& context, FAsyncRequestDataCache& data)
{
   auto findLocation = [](const AActor* actor, ETATAsyncRequestTraceLocationType type, FVector& outLocation) -> bool
   {
      switch (type)
      {
         case ETATAsyncRequestTraceLocationType::EyesViewPoint:
         {
            if (const APawn* ownerPawn = Cast<APawn>(actor))
            {
               FRotator eyesRotation;
               ownerPawn->GetActorEyesViewPoint(outLocation, eyesRotation);
               return true;
            }
            break;
         }
         case ETATAsyncRequestTraceLocationType::ActorLocation:
         {
            outLocation = actor->GetActorLocation();
            return true;
         }
      }

      return false;
   };

   FVector traceStart = FVector::ZeroVector;
   if (!ensure(findLocation(context.Source, context.SourceLocation, traceStart)))
   {
      return;
   }

   FVector traceEnd = FVector::ZeroVector;
   if (!ensure(findLocation(context.Target, context.TargetLocation, traceEnd)))
   {
      return;
   }

   FCollisionQueryParams queryParams(FName("TATAsyncRequestComponent"), SCENE_QUERY_STAT_ONLY(TATAsyncRequestComponent));
   queryParams.AddIgnoredActor(context.Source);
   queryParams.AddIgnoredActor(context.Target);

   FTraceDelegate delegate = FTraceDelegate::CreateUObject(this, &UTATAsyncRequestComponent::_HandleAsyncTraceComplete, context.GetHandle());
   data.TraceHandle = GetWorld()->AsyncLineTraceByProfile(EAsyncTraceType::Single, traceStart, traceEnd, context.CollisionProfile.Name, queryParams, &delegate);
}

void UTATAsyncRequestComponent::_HandleAsyncTraceComplete(const FTraceHandle& traceHandle, FTraceDatum& traceDatum, const FAsyncRequestHandle requestHandle)
{
   FAsyncRequestDataCache* dataCache = _asyncLineTraces.Find(requestHandle);
   if (dataCache == nullptr)
   {
      return;
   }

   // Trace finished, clear the handle.
   dataCache->TraceHandle.Invalidate();
   dataCache->LastUpdatedTime = GetWorld()->GetTimeSeconds();

   if (traceDatum.OutHits.IsEmpty())
   {
      dataCache->HitResult.Reset();
   }
   else
   {
      dataCache->HitResult = traceDatum.OutHits[0];
   }
}
