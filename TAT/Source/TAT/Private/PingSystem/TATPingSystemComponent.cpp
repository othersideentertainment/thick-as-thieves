// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "PingSystem/TATPingSystemComponent.h"

// OSE
#include "KismetTraceUtils.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "PingSystem/OSEPingActor.h"
#include "Player/OSEPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPingSystemComponent)

namespace TATPingSystemComponentCVars
{
   static int DrawPingTraceDebug = 0;
   FAutoConsoleVariableRef CVarDebugDrawPingTraceDebug(
      TEXT("TAT.Ping.Trace.DrawDebug"),
      DrawPingTraceDebug,
      TEXT("Draw debug for the ping system"),
      ECVF_Default);
   static int DrawPingActualLocationDebug = 0;
   FAutoConsoleVariableRef CVarDrawPingActualLocationDebug(
      TEXT("TAT.Ping.Location.DrawDebug"),
      DrawPingActualLocationDebug,
      TEXT("Draw debug for the ping system"),
      ECVF_Default);
}

bool CheckIsPingable(const AActor* actor)
{
   if(actor && actor->Implements<UOSEPingableInterface>())
   {
      return IOSEPingableInterface::Execute_IsPingable(actor);
   }
   return false;
}
FHitResult AttemptToGatherPingableTargetsFromTrace(const TArray<FHitResult>& results)
{
   for (const FHitResult& result : results)
   {
      if(CheckIsPingable(result.GetActor()))
      {
         return result;
      }
   }
   return FHitResult();
}

void UTATPingSystemComponent::_AttemptSonarPingFromHitActor(const AActor* sourceActor, AActor* hitResultActor, TArray<FHitResult>& outTraces) const
{
   FVector overlappingActorLocation = hitResultActor->GetActorLocation();
   FCollisionQueryParams overlapParams(SCENE_QUERY_STAT(UOSEPingSystemComponent_PingOverlap), false);
   overlapParams.AddIgnoredActor(sourceActor);
   overlapParams.AddIgnoredActor(hitResultActor);
         
   TArray<FOverlapResult> overlapResults;
   GetWorld()->OverlapMultiByProfile(overlapResults, overlappingActorLocation, FQuat::Identity, _traceProfileForGather.Name, FCollisionShape::MakeSphere(_sonarPingSize), overlapParams);

#if ENABLE_DRAW_DEBUG
   if(TATPingSystemComponentCVars::DrawPingTraceDebug != 0)
   {
      DrawDebugSphere(GetWorld(), overlappingActorLocation, _sonarPingSize, 8, FColor::Yellow, false, 2.f);
   }
#endif
   
   for (const FOverlapResult& overlapResult : overlapResults)
   {
      AActor* overlappingActor = overlapResult.GetActor();
      if(CheckIsPingable(overlappingActor))
      {
         FCollisionQueryParams overlapVisibilityParams(SCENE_QUERY_STAT(UOSEPingSystemComponent_PingOverlap_Visibility), false);
         overlapVisibilityParams.AddIgnoredActor(sourceActor);
         overlapVisibilityParams.AddIgnoredActor(hitResultActor);
         overlapVisibilityParams.AddIgnoredActor(overlappingActor);

         // Get the center of both the original hit actor and the new overlapping actor
         FVector hitResultActorCenterPoint, hitResultActorBounds;
         hitResultActor->GetActorBounds(true, hitResultActorCenterPoint, hitResultActorBounds);
         FVector overlappingActorCenterPoint, overlappingActorBounds;
         overlappingActor->GetActorBounds(true, overlappingActorCenterPoint, overlappingActorBounds);

         const bool canSeeTarget = GetWorld()->LineTraceTestByChannel(
            hitResultActorCenterPoint,
            overlappingActorCenterPoint,
            ECC_Visibility,
            overlapVisibilityParams
         ) == false;
         
#if ENABLE_DRAW_DEBUG
         if(TATPingSystemComponentCVars::DrawPingTraceDebug != 0)
         {
            DrawDebugLine(
               GetWorld(),
               hitResultActorCenterPoint,
               overlappingActorCenterPoint,
               canSeeTarget ? FColor::Green : FColor::Red,
               false,
               2.f
            );
         }
#endif
         if(canSeeTarget)
         {
            if(outTraces.ContainsByPredicate([overlappingActor](const FHitResult& otherTrace)
            {
               return otherTrace.GetActor() == overlappingActor;
            }) == false)
            {
               outTraces.Add(FHitResult{
                  overlappingActor,
                  nullptr,
                  overlappingActorCenterPoint,
                  FVector::ForwardVector
               });
            }
         }
      }
   }
}

void UTATPingSystemComponent::_DrawDebugSweepTrace(const TArray<FHitResult>& hitResults, const FVector& traceStartLocation, const FVector& traceEndLocation) const
{
#if ENABLE_DRAW_DEBUG
   if(TATPingSystemComponentCVars::DrawPingTraceDebug != 0)
   {
      const bool hit = hitResults.Num() > 0;
      const bool isBlocking = hit ? hitResults[hitResults.Num() - 1].bBlockingHit : false;
      const FVector hitEndLocation = isBlocking ? hitResults[hitResults.Num() - 1].ImpactPoint : traceEndLocation;
      DrawDebugSphereTraceMulti(
         GetWorld(),
         traceStartLocation,
         hitEndLocation,
         _traceWidth,
         EDrawDebugTrace::ForDuration,
         isBlocking,
         hitResults,
         FColor::Yellow,
         FColor::Red,
         2.f
      );
   }
#endif
}

bool UTATPingSystemComponent::_ShouldUseLocallyFocusedPing() const
{
   if(const AOSEPingActor* const focusedActor = GetLocalFocusedPingActor())
   {
      if(focusedActor->GetHitResult().GetActor() == _gatheredBlockHitResult.GetActor())
         return true;
      const float distance = focusedActor->GetDistanceTo(_GetOwnerController().GetPawn());
      if(_gatheredBlockHitResult.bBlockingHit && _gatheredBlockHitResult.Distance < distance)
      {
         return false;
      }
      if(_gatheredPingableTargets.Num() > 0)
      {
         for (const FGatheredPingTarget& pingableTarget : _gatheredPingableTargets)
         {
            if(pingableTarget.Actor.IsValid())
            {
               const float distanceToPingTarget = pingableTarget.Actor->GetDistanceTo(_GetOwnerController().GetPawn());
               if(distanceToPingTarget < distance)
               {
                  return false;
               }
            }
         }
      }
      return true;
   }
   return false;
}

bool UTATPingSystemComponent::_LineTraceForPing(const AActor* sourceActor,
                                                const FCollisionQueryParams& params,
                                                const UWorld* world,
                                                const FVector& traceStartLocation,
                                                const FVector& traceEndLocation,
                                                FHitResult& blockingHitResult,
                                                TArray<FHitResult>& finalizedHitResults) const
{
   TArray<FHitResult> hitResults;
   world->LineTraceMultiByProfile(
      hitResults,
      traceStartLocation,
      traceEndLocation,
      _traceProfileForGather.Name,
      params);
   
#if ENABLE_DRAW_DEBUG
   if(TATPingSystemComponentCVars::DrawPingTraceDebug != 0)
   {
      DrawDebugLine(
         GetWorld(),
         traceStartLocation,
         traceEndLocation,
         FColor::Red,
         false,
         2.f
      );
   }
#endif
   if(hitResults.Num() > 0)
   {
      const FHitResult& lastHitResult = hitResults[hitResults.Num() - 1];
      if(lastHitResult.bBlockingHit)
      {
         blockingHitResult = lastHitResult;
      }
   }
   return _HandleHitResultsForPing(sourceActor, hitResults, finalizedHitResults);
}

bool UTATPingSystemComponent::_SweepTraceForPing(const AActor* sourceActor,
                                                 const FCollisionQueryParams& params,
                                                 const UWorld* world,
                                                 const FCollisionShape traceShape,
                                                 const FVector& traceStartLocation,
                                                 const FVector& traceEndLocation,
                                                 TArray<FHitResult>& finalizedHitResults) const
{
   TArray<FHitResult> hitResults;
   world->SweepMultiByProfile(
      hitResults,
      traceStartLocation,
      traceEndLocation,
      FQuat::Identity,
      _traceProfileForGather.Name,
      traceShape,
      params
   );
   _DrawDebugSweepTrace(hitResults, traceStartLocation, traceEndLocation);
   return _HandleHitResultsForPing(sourceActor, hitResults, finalizedHitResults);
}

bool UTATPingSystemComponent::_HandleHitResultsForPing(const AActor* sourceActor, const TArray<FHitResult>& hitResults, TArray<FHitResult>& finalizedHitResults) const
{
   const FHitResult hitResult = AttemptToGatherPingableTargetsFromTrace(hitResults);
   if(AActor* hitResultActor = hitResult.GetActor())
   {
      finalizedHitResults.Add(hitResult);
      _AttemptSonarPingFromHitActor(sourceActor, hitResultActor, finalizedHitResults);
      return true;
   }
   return false;
}

void UTATPingSystemComponent::GatherPingableTargets()
{
   _gatheredPingableTargets.Empty();
   _gatheredBlockHitResult.Reset(0.f, false);

   // First we check for cooldown to avoid spamming.
   const float now = GetWorld()->GetTimeSeconds();
   if (now < _cooldownExpiringTime)
      return;
      
   _cooldownExpiringTime = now + _cooldownDuration;
   
   const AActor* sourceActor = _GetOwnerController().GetPawn();
   FVector traceStartMiddle;
   FVector traceEndMiddle;
   if (UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(
      sourceActor,
      FGameplayAbilityTargetingLocationInfo(),
      _maxGatherTraceRange,
      traceStartMiddle,
      traceEndMiddle
   ) == false)
   {
      return;
   }
   FCollisionQueryParams params(SCENE_QUERY_STAT(UOSEPingSystemComponent), false);
   params.AddIgnoredActor(sourceActor);

   const FVector forwardDirection = (traceEndMiddle - traceStartMiddle).GetSafeNormal();
   
   const UWorld* world = GetWorld();
   const FCollisionShape traceShape = FCollisionShape::MakeSphere(_traceWidth);

   TArray<FHitResult> finalizedHitResults;

   // Step 1: Line trace to center of screen - note this returns the blocking hit (if any)
   if(_LineTraceForPing(sourceActor, params, world, traceStartMiddle, traceEndMiddle, _gatheredBlockHitResult, finalizedHitResults) == false)
   {
      // Step 2: If we don't hit anything with the line trace, sweep along the middle
      if (_SweepTraceForPing(sourceActor, params, world, traceShape, traceStartMiddle, traceEndMiddle, finalizedHitResults) == false)
      {
         const float maxModifiedTraceRange = _gatheredBlockHitResult.bBlockingHit ? _gatheredBlockHitResult.Distance : _maxGatherTraceRange;
         const float traceWidthTimesTwo = _traceWidth * 2.f;
         for(int ringIndex = 1; ringIndex <= _numberOfTraceRings; ++ringIndex)
         {
            // Step 3: If neither of those sweeps or line traces hit, now we're going to start tracing in a circle, offset based on the
            // trace radius, increasing in circumference based on the trace radius and the number of ring traces we're doing
            // if any of them hit and return a valid target for a sweep, break out
            constexpr float angleDegrees = 360.f;
            const float radius = traceWidthTimesTwo * ringIndex;
            const float circumference = 2.f * PI * radius;
            const int stepCount = FMath::CeilToInt(circumference / traceWidthTimesTwo) + 1;
            const float angleStep = angleDegrees / (stepCount-1);
            const FVector traceStartDirection = forwardDirection.RotateAngleAxis(90.f, FVector::UpVector) * radius;
            bool shouldFinish = false;
            for(int step = 0; step < stepCount; ++step)
            {
               FVector traceStartLocation = (traceStartMiddle + traceStartDirection.RotateAngleAxis(angleStep * step, forwardDirection));
               FVector traceEndLocation = traceStartLocation + (forwardDirection * maxModifiedTraceRange);
         
               shouldFinish = _SweepTraceForPing(sourceActor, params, world, traceShape, traceStartLocation, traceEndLocation, finalizedHitResults);
               if(shouldFinish)
                  break;
            }
            if(shouldFinish)
               break;
         }
      }
   }
   // Step 4: Now we have the results (which should all be valid pingable targets), add them to the gathered pingable targets list
   for (const FHitResult& result : finalizedHitResults)
   {
      AActor* overlappingActor = result.GetActor();
      // Quick sanity check to ensure we don't accidentally add something that isn't pingable
      if(CheckIsPingable(overlappingActor))
      {
         _gatheredPingableTargets.Add(FGatheredPingTarget(
            overlappingActor,
            result
         ));
      }
   }
}

FGameplayTagContainer UTATPingSystemComponent::TagsFromGatheredTargets() const
{
   if(_ShouldUseLocallyFocusedPing() && _ShouldTriggerResponses())
   {
      if(const AOSEPingActor* focusedPingActor = GetLocalFocusedPingActor())
      {
         const FGameplayTag& pingTag = focusedPingActor->GetPingTag();
         check(pingTag.IsValid());
         if(const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag))
         {
            return FGameplayTagContainer::CreateFromArray(pingInfo->Responses);
         }
      }
   }
   
   //TODO: We currently pull the first tag from the container when we are using the "quick ping" functionality.
   //We need to be able to data drive the _best_ ping to select.
   FGameplayTagContainer tagContainer;
   for (const FGatheredPingTarget& pingableTarget : _gatheredPingableTargets)
   {
      if(pingableTarget.Actor.IsValid() && pingableTarget.Actor->Implements<UOSEPingableInterface>())
      {
         if(IOSEPingableInterface::Execute_IsPingable(pingableTarget.Actor.Get()))
         {
            tagContainer.AppendTags(IOSEPingableInterface::Execute_GetDefaultSingleInputPingTag(pingableTarget.Actor.Get()));
         }
      }
   }
   if(tagContainer.Num() == 0)
   {
      tagContainer = _fallbackPingsForGather;
   }
   return tagContainer;
}

void UTATPingSystemComponent::TriggerDefaultPingOnGatheredTargets()
{
   if(_ShouldUseLocallyFocusedPing() && _ShouldTriggerResponses())
   {
      if(const AOSEPingActor* focusedPingActor = GetLocalFocusedPingActor())
      {
         const FGameplayTag& pingTag = focusedPingActor->GetPingTag();
         check(pingTag.IsValid());
         if(const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag))
         {
            _AttemptTriggeringPingOnFocusedActor(pingInfo->DefaultResponse);
         }
      }
      return;
   }
   int currentPriority = INDEX_NONE;
   FGameplayTag currentlySelectedTag;
   for (FGatheredPingTarget& pingableTarget : _gatheredPingableTargets)
   {
      if(pingableTarget.Actor.IsValid() && pingableTarget.Actor->Implements<UOSEPingableInterface>())
      {
         if(IOSEPingableInterface::Execute_IsPingable(pingableTarget.Actor.Get()) == false)
         {
            continue;
         }
         FGameplayTagContainer pingTags = IOSEPingableInterface::Execute_GetDefaultSingleInputPingTag(pingableTarget.Actor.Get());
         for (FGameplayTag pingTag : pingTags)
         {
            if(const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag))
            {
               if(pingInfo->Priority > currentPriority)
               {
                  currentPriority = pingInfo->Priority;
                  currentlySelectedTag = pingTag;
               }
            }
         }
      }
   }
   if(currentlySelectedTag.IsValid())
   {
      TriggerPingOnGatheredTargets(currentlySelectedTag);
   }
}

bool UTATPingSystemComponent::_AttemptTriggeringPingOnFocusedActor(const FGameplayTag pingTagToTrigger)
{
   if(AOSEPingActor* focusedPingActor = GetLocalFocusedPingActor())
   {
      if(focusedPingActor->GetPingedBy() == Cast<AOSEPlayerState>(_GetOwnerController().PlayerState.Get()))
      {
         ServerCancelPing(focusedPingActor, true);      
      }
      else
      {
         ServerRespondToPing(focusedPingActor, pingTagToTrigger);      
      }
      return true;
   }
   return false;
}

void UTATPingSystemComponent::TriggerPingOnGatheredTargets(const FGameplayTag pingTagToTrigger)
{
   if(_ShouldUseLocallyFocusedPing() && _ShouldTriggerResponses())
   {
      if (_AttemptTriggeringPingOnFocusedActor(pingTagToTrigger)) return;
   }
   FVector outLocation;
   FRotator outRotation;
   _GetOwnerController().GetActorEyesViewPoint(outLocation, outRotation);

   const FTransform spawnerViewTransform = FTransform(outRotation, outLocation);
   const FTransform spawnerWorldTransform = _GetOwnerController().GetPawn()->GetTransform();
   for (FGatheredPingTarget& pingableTarget : _gatheredPingableTargets)
   {
      if(pingableTarget.Actor.IsValid() && pingableTarget.Actor->Implements<UOSEPingableInterface>())
      {
         if(IOSEPingableInterface::Execute_IsPingable(pingableTarget.Actor.Get()))
         {
            if(IOSEPingableInterface::Execute_GetDefaultSingleInputPingTag(pingableTarget.Actor.Get()).HasTag(pingTagToTrigger))
            {
               FOSEPingSpawnInfo spawnInfo;
               IOSEPingableInterface::Execute_GetPingSpawnInfo(pingableTarget.Actor.Get(), spawnInfo);
               ServerSpawnPing(
                  spawnInfo,
                  pingableTarget.HitResult,
                  spawnerViewTransform,
                  spawnerWorldTransform,
                  pingTagToTrigger,
                  pingableTarget.Actor.Get()
                  );
#if ENABLE_DRAW_DEBUG
               if(TATPingSystemComponentCVars::DrawPingActualLocationDebug)
               {
                  DrawDebugSphere(GetWorld(), pingableTarget.HitResult.Location, 32.f , 8, FColor::Red , false, 2.f);
               }
#endif
            }
         }
      }
   }
   if(_gatheredPingableTargets.Num() == 0 && _gatheredBlockHitResult.bBlockingHit)
   {
      FOSEPingSpawnInfo spawnInfo;
      spawnInfo.Location = _gatheredBlockHitResult.ImpactPoint;
      spawnInfo.SpawnType = EOSEPingSpawnType::AtSpecificWorldLocation;
#if ENABLE_DRAW_DEBUG
      if(TATPingSystemComponentCVars::DrawPingActualLocationDebug)
      {
         DrawDebugSphere(GetWorld(), _gatheredBlockHitResult.ImpactPoint, 32.f , 8, FColor::Red , false, 2.f);
      }
#endif
      ServerSpawnPing(
         spawnInfo,
         _gatheredBlockHitResult,
         spawnerViewTransform,
         spawnerWorldTransform,
         pingTagToTrigger,
         nullptr
         );
   }
}
