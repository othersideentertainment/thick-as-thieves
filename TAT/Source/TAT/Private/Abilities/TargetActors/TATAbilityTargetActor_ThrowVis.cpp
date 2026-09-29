// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/TATAbilityTargetActor_ThrowVis.h"

// ose
#include "Projectiles/OSEProjectileFunctionLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityTargetActor_ThrowVis)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityTargetActor_ThrowVis, Log, All)

ATATAbilityTargetActor_ThrowVis::ATATAbilityTargetActor_ThrowVis()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
}

void ATATAbilityTargetActor_ThrowVis::BeginPlay()
{
   Super::BeginPlay();

   if (VisualisationDelay <= 0.0f)
   {
      _StartShowingVisualisation();
   }
   else
   {
      FTimerHandle handle;
      GetWorld()->GetTimerManager().SetTimer(handle, this, &ATATAbilityTargetActor_ThrowVis::_StartShowingVisualisation, VisualisationDelay);
   }
}

void ATATAbilityTargetActor_ThrowVis::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);
   TickVisualization();
}

void ATATAbilityTargetActor_ThrowVis::TickVisualization()
{
   const FVector startPos = GetStartLocation();
   const FVector endPos = GetActorLocation();

   const FVector suggestedVelocity = SuggestLaunchVelocity(startPos, endPos);

   TArray<AActor*> sourceActorArray;
   sourceActorArray.Add(SourceActor);

   const bool tracePath = true;
   const bool traceComplex = false;
   const float drawDebugTime = 0.0f;

   FHitResult traceHit;
   TArray<FVector> pathPositions;
   FVector lastTraceDestination;
   bool successfulThrow = UOSEProjectileFunctionLibrary::Blueprint_PredictProjectilePath_ByTraceProfile(
      this,
      traceHit,
      pathPositions,
      lastTraceDestination,
      startPos,
      suggestedVelocity,
      tracePath,
      PredictedProjectileRadius,
      TraceProfile.Name,
      traceComplex,
      sourceActorArray,
      EDrawDebugTrace::None,
      drawDebugTime,
      SimFrequency,
      MaxSimTime,
      OverrideGravityZ);

   if (successfulThrow && pathPositions.Num() == 0)
   {
      UE_LOG(LogTATAbilityTargetActor_ThrowVis, Error,
         TEXT("ATATAbilityTargetActor_ThrowVis '%s' obtained 0 points in its predicted path, likely a misconfiguration"),
         *GetName());
   }

   // Now that we've computed our throw path, update any predicted visuals
   bool hasPath = successfulThrow && pathPositions.Num() > 0;
   UpdateVisualsForPredictedThrow(hasPath, pathPositions, traceHit);
}

FVector ATATAbilityTargetActor_ThrowVis::GetStartLocation()
{
   return GetEyeRelativePosition(SourceActor, ThrowStartOffset, bViewRelative);
}

FVector ATATAbilityTargetActor_ThrowVis::GetEyeRelativePosition(AActor* source, const FVector& offset, bool viewRelative)
{
   FVector eyeLocation;
   FRotator eyeRotation;
   source->GetActorEyesViewPoint(eyeLocation, eyeRotation);
   if (viewRelative)
   {
      return eyeLocation + eyeRotation.RotateVector(offset);
   }
   else
   {
      return eyeLocation + source->GetActorRotation().RotateVector(offset);
   }
}

void ATATAbilityTargetActor_ThrowVis::_StartShowingVisualisation()
{
   // Call into the blueprint so it can make any visualisation visible
   ShowVisualisation();

   SetActorTickEnabled(true);
}

