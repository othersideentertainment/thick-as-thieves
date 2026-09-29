// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
//#include "CoreMinimal.h"
#include "WorldCollision.h"
#include "Components/ActorComponent.h"
#include "Containers/Map.h"
#include "Engine/HitResult.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "TATAsyncRequestComponent.generated.h"

class AActor;

UENUM()
enum class ETATAsyncRequestTraceLocationType : uint8
{
   EyesViewPoint,
   ActorLocation
};

struct FTATAsyncTraceRequestContext
{
public:
   // Unique identifier for this 'Source'->'Target' combination.
   using FHandle = uint32;

   // Used to determine the starting location of the line trace.
   TObjectPtr<const AActor> Source = nullptr;
   ETATAsyncRequestTraceLocationType SourceLocation = ETATAsyncRequestTraceLocationType::EyesViewPoint;

   // Used to determine the ending location of the line trace.
   TObjectPtr<const AActor> Target = nullptr;
   ETATAsyncRequestTraceLocationType TargetLocation = ETATAsyncRequestTraceLocationType::ActorLocation;

   // Collision profile to use in the line trace.
   FCollisionProfileName CollisionProfile;

   // How often should we attempt to start a new line trace to ensure the results stay fresh?
   float Interval = 1.0f;

   FORCEINLINE bool IsValid() const { return Source != nullptr && Target != nullptr && CollisionProfile.Name.IsValid() && Interval >= 0.0f; }

   FORCEINLINE FHandle GetHandle() const { check(IsValid()); return HashCombine(GetTypeHash(Source), GetTypeHash(Target)); }

};

UCLASS(HideCategories = (AssetUserData, Activation, ComponentReplication, ComponentTick, Cooking, Navigation, Replication, Tags))
class TAT_API UTATAsyncRequestComponent : public UActorComponent
{
	GENERATED_BODY()

   using FAsyncRequestHandle = FTATAsyncTraceRequestContext::FHandle;

public:	
   // Could be templated to swap 'Handle' and 'HitResult' for other types of async requests (pathfinding?)
   struct FAsyncRequestDataCache
   {
      // Only valid while a trace is in flight.
      FTraceHandle TraceHandle;

      // Results from the line trace. If unset, there was no collision found.
      TOptional<FHitResult> HitResult;

      // The last time this request was requested.
      float LastRequestTime = -FLT_MAX;

      // The last time this request finished an async trace.
      float LastUpdatedTime = -FLT_MAX;

      FORCEINLINE bool HasData() const { return (LastUpdatedTime >= 0.0f); }
      FORCEINLINE bool IsInFlight() const { return TraceHandle.IsValid(); }
   };

   UTATAsyncRequestComponent();

   // from UActorComponent
   virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   // Request the most recently available data cached from the last line trace.
   // Note that only one trace per target actor can currently be processed.
   const FAsyncRequestDataCache& RequestAsyncLineTraceData(const FTATAsyncTraceRequestContext& context);

private:
   // How long after a trace was last requested should it be removed (if not re-requested)?
   UPROPERTY(EditDefaultsOnly, meta = (Units = "seconds"))
   float _requestPruneThreshold = 1.0f;

   // The interval at which we check for requests that need pruning.
   // Will be used as tick interval for this component when prune checks need to occur.
   UPROPERTY(EditDefaultsOnly, meta = (Units = "seconds"))
   float _requestPruneInterval = 1.0f;

   // Local cache of all the currently processing trace requests.
   // Entries will be removed after some time they are no longer 
   // requested (configurable with '_requestPruneThreshold').
   TMap<FAsyncRequestHandle, FAsyncRequestDataCache> _asyncLineTraces;

   void _TriggerAsyncLineTrace(const FTATAsyncTraceRequestContext& context, FAsyncRequestDataCache& data);
   void _HandleAsyncTraceComplete(const FTraceHandle& traceHandle, FTraceDatum& traceDatum, const FAsyncRequestHandle requestHandle);

};
