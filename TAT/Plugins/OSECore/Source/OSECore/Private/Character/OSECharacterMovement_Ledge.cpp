// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "Character/OSECharacterMovement.h"
#include "Character/OSECharacterBase.h"
#include "Traversal/Mantle/OSEMantleQuery.h"
#include "Traversal/Mantle/OSELedgeQuery.h"
#include "Traversal/Mantle/OSELedgeState.h"
#include "Traversal/Mantle/OSELedgeAnimSet.h"
#include "OSECommon.h"

// UE4
#include "Components/CapsuleComponent.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSECharacterMovement_Ledge, Log, All);


void UOSECharacterMovement::StartLedgeState(bool bClientSimulation)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      bIgnoreClientMovementErrorChecksAndCorrection = true;

      // Simulated proxies get this via replication
      OSECharOwner->SetIsLedgeMode(true);

      //we always enter the mount entering into ledge state
      StartLedgeMount(bClientSimulation);
   }
   
   SetCustomMovementType(ECustomMovementType::Mantle);

   OSECharOwner->OnStartLedgeState();
}


void UOSECharacterMovement::StopLedgeState(bool bClientSimulation)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      bIgnoreClientMovementErrorChecksAndCorrection = false;

      if (IsLedgeMounting())
      {
         StopLedgeMount(bClientSimulation);
      }
      // Simulated proxies get this via replication
      OSECharOwner->SetIsLedgeMode(false);

   }

   // Only change movement mode if we're still in mantle mode. External logic could
   // have explicitly set a new movement mode which would trigger the StopMantle(),
   // and we don't want to mess with what they set in that situation.
   if (GetCustomMovementType() == ECustomMovementType::Mantle)
      SetMovementMode(MOVE_Falling);

   // Require the request to be submitted again
   // @TODO: Make this an option (or cvar)
   SetWantsToMantle(false);
   SetWantsToScramble(false);

   OSECharOwner->OnStopLedgeState();
}

void UOSECharacterMovement::StartLedgeMount(bool bClientSimulation)
{
   check(IsInLedgeState());

   const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   // Dump logging info when mantle state is not valid
   if (!ensure(ledgeState.MountTarget.IsValid))
   {
      const FString mantleText = FString::Printf(TEXT("[%s %s]\t\tledgeState"), *GetReadableName(), *UEnum::GetValueAsString(GetOwnerRole()));
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s IsValid        : %d"), *mantleText, ledgeState.MountTarget.IsValid);
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s IsMantling     : %d"), *mantleText, ledgeState.IsLedgeStateActive);
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s Sim Proxy      : %d"), *mantleText, bClientSimulation);
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s Capsule        : %s"), *mantleText, *ledgeState.MountTarget.CapsuleExtents.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s Montage        : %s"), *mantleText, ledgeState.MountTarget.Montage.IsValid() ? *(ledgeState.MountTarget.Montage.Get()->GetName()) : TEXT("none"));
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s QueryLocation  : %s"), *mantleText, *ledgeState.MountTarget.QueryLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s QueryDirection : %s"), *mantleText, *ledgeState.MountTarget.QueryDirection.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s StartLocation  : %s"), *mantleText, *ledgeState.MountTarget.StartLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s StartDirection : %s"), *mantleText, *ledgeState.MountTarget.StartDirection.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s FinalLocation  : %s"), *mantleText, *ledgeState.MountTarget.MantleLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement_Ledge, Warning, TEXT("%s FinalDirection : %s"), *mantleText, *ledgeState.MountTarget.MantleDirection.ToCompactString());
   }
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   if (!bClientSimulation)
   {
      // Simulated proxies get this via replication
      OSECharOwner->SetIsLedgeMounting(true);
   }

   // Change the start location if we're planning to scale the root motion to reach our goal
   const FVector startLocationResolved = ledgeState.MountTarget.StartLocation;
   const FVector startLocationActual = FVector(
      ledgeState.MountTarget.StartLocation.X,
      ledgeState.MountTarget.StartLocation.Y,
      ledgeState.MountTarget.QueryLocation.Z);

   const FVector startLocationToUse = startLocationActual;

   // Clamp our start velocity to our current max speed
   const FVector startVelocity = GetLastUpdateVelocity().GetClampedToMaxSize(GetMaxSpeed());

   //set up motion warping
   UMotionWarpingComponent* warp = GetOwner()->FindComponentByClass<UMotionWarpingComponent>();
   if (warp)
   {
      const FName warpTargetName(TEXT("LedgeTarget"));
      const FRotator ledgeRotation = FRotationMatrix::MakeFromX(ledgeState.MountTarget.AnchorDirection).Rotator();
      FTransform anchorTransform(ledgeRotation, ledgeState.MountTarget.AnchorLocation);
      warp->AddOrUpdateWarpTargetFromTransform(warpTargetName, anchorTransform);
   }

   // Set transient Mount state
   SetLedgeMountMontage(ledgeState.MountTarget.Montage.Get());
   SetLedgeMountTimeElapsed(0.0f);
   SetLedgeMountTimeRemaining(0.0f);
   SetLedgeMountStartOffset(startLocationToUse - ledgeState.MountTarget.QueryLocation);
   SetLedgeMountStartVelocity(startVelocity);

   // Store an approximate Mount duration; we use this to ensure the Mount ends eventually
   if (UAnimMontage* mountMontage = GetLedgeMountMontage())
   {
      const float animDuration = CharacterOwner->PlayAnimMontage(mountMontage);
      const float finalDuration = FMath::Max(0.0f, animDuration - mountMontage->GetDefaultBlendOutTime());
      SetLedgeMountTimeRemaining(finalDuration);
   }
}

void UOSECharacterMovement::StopLedgeMount(bool bClientSimulation)
{
   if (!bClientSimulation)
   {
      // Simulated proxies get this via replication
      OSECharOwner->SetIsLedgeMounting(false);
   }
   // Stop the animation montage
   if (UAnimMontage* mountMontage = GetLedgeMountMontage())
   {
      CharacterOwner->StopAnimMontage(mountMontage);
   }
   //set up motion warping
   UMotionWarpingComponent* warp = GetOwner()->FindComponentByClass<UMotionWarpingComponent>();
   if (warp)
   {
      const FName warpTargetName(TEXT("LedgeTarget"));
      warp->RemoveWarpTarget(warpTargetName);
   }

   // Reset the internal mantle state as well
   SetLedgeMountMontage(nullptr);
   SetLedgeMountTimeElapsed(0.0f);
   SetLedgeMountTimeRemaining(0.0f);
   SetLedgeMountStartOffset(FVector::ZeroVector);
   SetLedgeMountStartVelocity(FVector::ZeroVector);
}

bool UOSECharacterMovement::IsInLedgeState() const
{
   return OSECharOwner && OSECharOwner->IsOnLedge();
}

bool UOSECharacterMovement::IsLedgeMounting() const
{
   return OSECharOwner && OSECharOwner->IsLedgeMounting();
}

bool UOSECharacterMovement::CanEverUseLedges() const
{
   return GetLedgeStateEnabled();
}

bool UOSECharacterMovement::CanUseLedgesInCurrentState() const
{
   if (OSECharOwner == nullptr)
      return false;

   //// Must be able to mantle
   //if (!CanEverEnterLedgeState())
   //   return false;

   // Owner must be able to mantle
   if (!OSECharOwner->CanUseLedges())
      return false;

   // Can't be sliding or crouching
   if (IsSliding() || IsCrouching())
      return false;

   // Must have valid ledge data
   const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();
   if (!ledgeState.MountTarget.IsValid)
      return false;
   const FVector startDirection = ledgeState.MountTarget.StartDirection;
   

   // Determine if we're moving into or away from the wall
   const float radianFaceAngle = 2.0f * FMath::DegreesToRadians(GetLedgeSettings().MaxAngleFacing);
   const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
   const float cosineWallAngle = (Acceleration.GetSafeNormal2D() | startDirection);
   const bool isMovingAway = (cosineWallAngle <= -cosineFaceAngle);
   const bool isMovingInto = (cosineWallAngle >= +cosineFaceAngle);

   if (!IsInLedgeState())
   {
      // We're not mantling yet. We can mantle if we're accelerating into the wall.
      return !isMovingAway;
   }
   else
   {

      const bool isMantleEnded = GetCustomMovementType() != ECustomMovementType::Mantle;

      // Keep mantling until we should stop
      return !isMantleEnded;
   }
}

// Helper for saved moves. Allows us to keep the actual transient state protected
// while modified explicitly and with clear purpose.
void UOSECharacterMovement::SetLedgeTransients(const FOSESavedMove_Character& savedMove)
{
   SetLedgeMountMontage(savedMove.SavedLedgeMontage.Get());
   SetLedgeMountTimeElapsed(savedMove.SavedLedgeMountTimeElapsed);
   SetLedgeMountTimeRemaining(savedMove.SavedLedgeMountTimeRemaining);
   SetLedgeMountStartOffset(savedMove.SavedLedgeMountStartOffset);
   SetLedgeMountStartVelocity(savedMove.SavedLedgeMountStartVelocity);
}

bool UOSECharacterMovement::ResolveLedgeQuery(FOSELedgeMountTarget& outState, const FOSELedgeQueryResult& inQuery) const
{
   outState = FOSELedgeMountTarget();

   if (!inQuery.IsValid())
      return false;

   // Check if the final position is a valid landing spot
   const bool bValidLanding = IsValidLandingSpot(inQuery.FinalHitResult.Location, inQuery.FinalHitResult);

   if (!bValidLanding && !inQuery.IsValidLedge())
      return false;

   if (!IsClimbableHit(inQuery.LedgeHitResult) || !IsClimbableHit(inQuery.FinalHitResult))
      return false;

   // The height delta for this query
   const float heightDelta = inQuery.LedgeHitResult.ImpactPoint.Z - (inQuery.StartHitResult.Location.Z - inQuery.CapsuleHalfHeight);

   //Copy over the state info reguardless of the outcome, for debugging.
   outState.CapsuleExtents = FVector(inQuery.CapsuleRadius, inQuery.CapsuleRadius, inQuery.CapsuleHalfHeight);
   outState.QueryLocation = inQuery.QueryLocation;
   outState.QueryDirection = inQuery.QueryRotation.Vector();
   outState.StartLocation = inQuery.StartHitResult.Location;
   outState.StartDirection = (-inQuery.StartHitResult.Normal).GetSafeNormal2D();
   outState.StartPhysicalMaterial = UOSECommon::GetPhysicalMaterialFromHitResult(inQuery.StartHitResult);
   outState.AnchorLocation = inQuery.LedgeHitResult.ImpactPoint;
   outState.AnchorDirection = outState.StartDirection;
   outState.MantleLocation = inQuery.FinalHitResult.Location;
   outState.MantleDirection = outState.StartDirection;

   // Search for a matching animation
   const TArray<UOSELedgeAnimSet*>& mountAnims = GetMountAnimations();
   for (const auto animSet : mountAnims)
   {
      if (animSet == nullptr)
         continue;

      FOSELedgeAnim matchedAnim;
      if (animSet->FindAnimation(matchedAnim, MovementMode, GetCustomMovementType(), Velocity.Size2D(), heightDelta, bValidLanding))
      {
         outState.IsValid = inQuery.IsValid();
         outState.AutoEject = matchedAnim.AutoEject;
         outState.Montage = matchedAnim.Montage;

         // Offset the start to be the exact delta we require
         outState.StartLocation.Z = outState.MantleLocation.Z - matchedAnim.HeightOffset;

         // Matched
         return true;
      }
   }

   return false;
}

bool UOSECharacterMovement::ResolveLedgeShimmyQuery(FOSELedgeMountTarget& outState, const FOSELedgeQueryResult& inQuery) const
{
   outState = FOSELedgeMountTarget();

   if (!inQuery.IsValidLedge())
      return false;

   if (!IsClimbableHit(inQuery.LedgeHitResult) || !IsClimbableHit(inQuery.FinalHitResult))
      return false;

   // The height delta for this query
   const float heightDelta = inQuery.FinalHitResult.Location.Z - inQuery.StartHitResult.Location.Z;


   // TODO: probably want to rebuild this loop for shimmy animations, but right now I'm only supporting the two directions
   //const TArray<UOSELedgeAnimSet*>& mountAnims = GetMountAnimations();
   //for (const auto animSet : mountAnims)
   {
      //if (animSet == nullptr)
      //   continue;
      UAnimMontage* montage = nullptr;

      //I feel like I do this a lot, might be worth formalizing
      const FVector currentLocation = OSECharOwner->GetLedgeState().MountTarget.MantleLocation;
      const FVector currentForward = OSECharOwner->GetLedgeState().MountTarget.MantleDirection;
      const FVector test = currentForward ^ (inQuery.FinalHitResult.Location - currentLocation);
      //our two vectors should be near to the XY plane, so we taking the shortcut of just testing Z rather than dot with the up
      if (test.Z > 0)
      {
         montage = _rightLedgeShimmy;
      }
      else
      {
         montage = _leftLedgeShimmy;
      }

      //UPGRADETODO: Why is this commented out?
      //FOSELedgeAnim matchedAnim;
      //if (animSet->FindAnimation(matchedAnim, MovementMode, GetCustomMovementType(), Velocity.Size2D(), heightDelta))
      {
         outState.IsValid = inQuery.IsValid();
         outState.AutoEject = false;
         outState.CapsuleExtents = FVector(inQuery.CapsuleRadius, inQuery.CapsuleRadius, inQuery.CapsuleHalfHeight);
         outState.Montage = montage;
         outState.QueryLocation = inQuery.QueryLocation;
         outState.QueryDirection = inQuery.QueryRotation.Vector();
         outState.StartLocation = inQuery.StartHitResult.Location;
         outState.StartDirection = (-inQuery.StartHitResult.Normal).GetSafeNormal2D();
         outState.StartPhysicalMaterial = UOSECommon::GetPhysicalMaterialFromHitResult(inQuery.StartHitResult);
         outState.AnchorLocation = inQuery.LedgeHitResult.ImpactPoint;
         outState.AnchorDirection = outState.StartDirection;
         outState.MantleLocation = inQuery.FinalHitResult.Location;
         outState.MantleDirection = outState.StartDirection;

         // Matched
         return true;
      }
   }

   //return false;
}

bool UOSECharacterMovement::GetShouldMantle() const
{
   if (!_ledgeHangEnabled)
   {
      return true;
   }

   FVector localAcceleration = FRotationMatrix::MakeFromX(OSECharOwner->GetLedgeState().MountTarget.MantleDirection).InverseTransformVector(Acceleration);
   return localAcceleration.X > 100.f && localAcceleration.X > FMath::Abs(localAcceleration.Y);
}

bool UOSECharacterMovement::GetShouldShimmy() const
{
   if (!_shimmyEnabled)
   {
      return false;
   }

   FVector localAcceleration = FRotationMatrix::MakeFromX(OSECharOwner->GetLedgeState().MountTarget.MantleDirection).InverseTransformVector(Acceleration);
   return FMath::Abs(localAcceleration.Y) > 100.f && FMath::Abs(localAcceleration.Y) > localAcceleration.X;
}

void UOSECharacterMovement::UpdateLedgeState()
{
   if (!OSECharOwner->CanUseLedges())
   {
      return;
   }

   // Proxies get replicated state
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      // @TODO: Optimization: Try adding a check for GetWantsToMantle() here too... thus avoids the query

      // If we're not mantling but we can mantle, then update our query
      if (!IsInLedgeState())
      {
         // Query the raw mantle data
         const FOSELedgeQueryResult mantleQuery = UOSELedgeQuery::TraceMantleCharacterWithVelocity(
            GetLedgeSettings(), CharacterOwner, GetLastUpdateVelocity());

         // Resolve it into the state we need for our conditions
         FOSELedgeState ledgeState = OSECharOwner->GetLedgeState();
         ResolveLedgeQuery(ledgeState.MountTarget, mantleQuery);
         OSECharOwner->SetLedgeState(ledgeState);

         // Transition in and out of mantle mode
         // If we're not allowing ledge hang then we only allow grabbing ledges where mantle is possible
         if (GetWantsToMantle() && CanUseLedgesInCurrentState()
            && (_ledgeHangEnabled || mantleQuery.IsValidMantle()))
         {
            StartLedgeState(false);
         }
      }
      else //In Ledge State
      {
         if (!CanUseLedgesInCurrentState())
         {
            StopLedgeState(false);
         }
         else
         {
            const FOSELedgeState& constLedgeState = OSECharOwner->GetLedgeState();
            // Mantle can end normally, or from external changes to movement mode
            const bool isMountOver = constLedgeState.IsMounting && GetLedgeMountTimeRemaining() < MIN_TICK_TIME;
            if (isMountOver)
            {
               if (constLedgeState.MountTarget.AutoEject)
               {
                  StopLedgeState(false);
               }
               else
               {
                  StopLedgeMount(false);
               }
            }

            //Currently at rest
            if (!OSECharOwner->GetLedgeState().IsMounting)
            {
               bool bShouldMantle = GetShouldMantle();
               bool bShouldShimmy = GetShouldShimmy();

               if (GetWantsToReleaseLedge())
               {
                  StopLedgeState(false);
               }
               //check if we should mantle 
               else if(GetWantsToMantle() || bShouldMantle)
               {
                  FOSELedgeState ledgeState = OSECharOwner->GetLedgeState();
                  const FOSELedgeQueryResult mantleQuery = UOSELedgeQuery::TraceFromLedgeCharacter(
                     GetLedgeSettings(), CharacterOwner, ledgeState);

                  // Resolve it into the state we need for our conditions
                  //unlike the initial ledge grab, we don't want to just exit if there isn't a valid mantle
                  // just do nothing if there isn't a mantle in the direction we want.
                  if(ResolveLedgeQuery(ledgeState.MountTarget, mantleQuery))
                  {
                     OSECharOwner->SetLedgeState(ledgeState);

                     if (ledgeState.MountTarget.IsValid)
                     {
                        StartLedgeMount();
                     }
                     _DebugLedgeState.MountTarget.IsValid = false;
                  }
                  else
                  {
                     _DebugLedgeState = ledgeState;
                     _DebugLedgeState.MountTarget.IsValid = true;
                  }
                  
               }
               else if(bShouldShimmy)
               {
                  //get the normal along the ledge in the direction of the shimmy
                  FVector SearchNormal = OSECharOwner->GetLedgeState().MountTarget.MantleDirection ^ Acceleration.GetSafeNormal() ^ OSECharOwner->GetLedgeState().MountTarget.MantleDirection;
                  FRotator SearchDirection = FRotationMatrix::MakeFromX(SearchNormal).Rotator();

                  const FOSELedgeQueryResult mantleQuery = UOSELedgeQuery::TraceShimmyCharacterWithVelocity(
                     GetLedgeSettings(), CharacterOwner, SearchDirection, GetLastUpdateVelocity());

                  // Resolve it into the state we need for our conditions
                  //unlike the initial ledge grab, we don't want to just exit if there isn't a valid shimmy
                  // just do nothing if there isn't a shimmy in the direction we want.

                  FOSELedgeState ledgeState = OSECharOwner->GetLedgeState();
                  if(ResolveLedgeShimmyQuery(ledgeState.MountTarget, mantleQuery))
                  {
                     OSECharOwner->SetLedgeState(ledgeState);

                     if (ledgeState.MountTarget.IsValid)
                     {
                        StartLedgeMount();
                     }
                  }
               }
            } 
         }
      }
   }
}

void UOSECharacterMovement::PhysCustomLedge(float deltaTime, int32 iterations)
{
   if (deltaTime < MIN_TICK_TIME)
   {
      return;
   }

   const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();
   if (!ledgeState.MountTarget.IsValid)
   {
      return;
   }

   //If we're currently mounting a target position, run the logic for that
   if(ledgeState.IsMounting)
   {
      PhysCustomLedge_Mount(deltaTime, iterations);
   }
}

void UOSECharacterMovement::PhysCustomLedge_Mount(float deltaTime, int32 iterations)
{
   const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();

   // Perform the move using substeps, as this will help with prediction and replays
   float remainingTime = deltaTime;
   while ((remainingTime >= MIN_TICK_TIME) && (iterations < MaxSimulationIterations))
   {
      ++iterations;

      const float timeTick = GetSimulationTimeStep(remainingTime, iterations);
      remainingTime -= timeTick;

      RestorePreAdditiveRootMotionVelocity();

      const FVector oldLocation = UpdatedComponent->GetComponentLocation();
      const FVector oldVelocity = Velocity;

      ApplyRootMotionToVelocity(timeTick);

      // Scrambling is blocked while mantling
      _scrambleIsBlocked = true;

      // Update the remaining time; the value is used to swap us out of mount mode
      const float mountTimeElapse = GetLedgeMountTimeElapsed();
      const float mountTimeRemain = GetLedgeMountTimeRemaining();
      SetLedgeMountTimeElapsed(mountTimeElapse + timeTick);
      SetLedgeMountTimeRemaining(FMath::Max(0.0f, mountTimeRemain - timeTick));

      // MOMENTUM
      if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
      {
         const bool scaleRootMotionZ = false ; //MovementCVars::MountScaleRootMotionZ > 0;
         if (scaleRootMotionZ)
         {
            // During the first half of the mount, we'll use the height 
            // differences to compute a velocity to add every tick. This
            // velocity will be enough to compensate for the potential
            // difference in heights between our animation and our destination.
            if (mountTimeElapse < mountTimeRemain)
            {
               float heightResolved = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.StartLocation.Z;
               float heightActual = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.QueryLocation.Z;

               if(ledgeState.MountTarget.AutoEject)
               {
                  heightResolved = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.StartLocation.Z;
                  heightActual = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.QueryLocation.Z;
               }
               else
               {
                  heightResolved = ledgeState.MountTarget.AnchorLocation.Z - ledgeState.MountTarget.StartLocation.Z;
                  heightActual = ledgeState.MountTarget.AnchorLocation.Z - ledgeState.MountTarget.QueryLocation.Z;
               }


               const float mountTimeTotal = mountTimeElapse + mountTimeRemain;
               const float mountTimeRootZ = mountTimeTotal * 0.5f;
               if (mountTimeRootZ > 0.0f)
               {
                  const float mountHeightDiff = FMath::Max(0.0f, heightActual - heightResolved);
                  const float mountHeightSpeed = mountHeightDiff / mountTimeRootZ;
                  Velocity += FVector::UpVector * mountHeightSpeed;
               }
            }
         }

         const bool keepMomentum = false;//MovementCVars::MountKeepMomentum > 0;
         if (keepMomentum)
         {
            // If we're applying input during the second half of the mount, accumulate
            // extra velocity such that we approximately match our speed when we
            // initiated the mount. This allows us to continue momentum even if the
            // animation we're using would normally bring us to a full stop.
            if (mountTimeRemain < mountTimeElapse)
            {
               // We apply the speed in our expected final direction, and use it
               // to project our acceleration (input) direction
               const FVector finalVector = ledgeState.MountTarget.MantleDirection;
               const float accelBias = finalVector | Acceleration.GetSafeNormal();

               // Our initial forward speed when we queried the mount. Ensure we'll
               // move at least at walking speed (it is filtered later)
               const FVector queryVector = ledgeState.MountTarget.QueryDirection;
               const float querySpeed = queryVector | GetLedgeMountStartVelocity();
               const float speedNext = FMath::Max(MaxWalkSpeed, querySpeed);

               // We don't want to exceed the speed if our animation would bring us there anyways
               const float speedCurr = Velocity.Size2D();
               const float speedDiff = FMath::Max(0.0f, speedNext - speedCurr);

               // Progressively ramp up the amount of speed as our animation ends
               const float ratio = 1.0f - (mountTimeRemain / mountTimeElapse);
               const FVector extraVelocity = finalVector * speedDiff * accelBias * ratio;
               Velocity += extraVelocity;
            }
         }
      }

      // BLEND IN; BLEND OUT
      if (UAnimMontage* mountMontage = GetLedgeMountMontage())
      {
         FVector deltaPosition = FVector::ZeroVector;

         // Use the montage blend in time to move towards the start
         {
            const float defaultBlendInTime = mountMontage->BlendIn.GetBlendTime();
            const float blendInRemains = defaultBlendInTime - mountTimeElapse;
            const FVector mountStartOffset = GetLedgeMountStartOffset();

            if (blendInRemains < MIN_TICK_TIME)
            {
               // Blend in is done; make sure we apply the rest of the offset and clear it
               deltaPosition += mountStartOffset;
               SetLedgeMountStartOffset(FVector::ZeroVector);
            }
            else
            {
               // Compute how fast we need to finish the offset based on the blend in time,
               // apply it and clamp the distance, subtracting the result from the offset.
               const float offsetDistRemain = mountStartOffset.Size();
               const float offsetSpeed = offsetDistRemain / blendInRemains;
               const float offsetDistClamped = FMath::Min(offsetDistRemain, offsetSpeed * timeTick);
               const FVector offsetAmount = mountStartOffset.GetSafeNormal() * offsetDistClamped;

               deltaPosition += offsetAmount;
               SetLedgeMountStartOffset(mountStartOffset - offsetAmount);
            }
         }

         // Use the montage blend out time to move towards the floor we are exiting the ledge state
         if (ledgeState.MountTarget.AutoEject)
         {
            const float defaultBlendOutTime = mountMontage->GetDefaultBlendOutTime();
            const float blendOutFactor = 1.0f - (mountTimeRemain / defaultBlendOutTime);
            if (blendOutFactor > 0.0f)
            {
               // Search for a walkable floor beneath the spot we'll end next tick
               const FVector deltaSoFar = deltaPosition + (Velocity * timeTick);
               const FVector capsuleLocation = deltaSoFar + UpdatedComponent->GetComponentLocation();

               FFindFloorResult floorResult;
               FindFloor(capsuleLocation, floorResult, false);

               if (floorResult.IsWalkableFloor())
               {
                  // It's walkable; if our floor is close enough just transition to walking
                  const FVector deltaToFloor = (floorResult.HitResult.Location - capsuleLocation);
                  if (floorResult.FloorDist <= UCharacterMovementComponent::MAX_FLOOR_DIST)//MovementCVars::MountFloorCheckDistance)
                  {
                     // Switch to walking
                     SetMovementMode(GetGroundMovementMode());
                     StartNewPhysics(remainingTime, iterations);
                     return;
                  }

                  // Nudge towards the floor with increasing ratio as we blend out
                  deltaPosition += (deltaToFloor * blendOutFactor);
               }
            }
         }

         Velocity += (deltaPosition / timeTick);
      }

      // APPLY THE MOVE
      {
         const FVector deltaPosition = Velocity * timeTick;

         // Move; this is combined root motion and the initial offset
         FHitResult moveHit(1.0f);
         SafeMoveUpdatedComponent(deltaPosition, UpdatedComponent->GetComponentQuat(), true, moveHit);

         if (moveHit.bBlockingHit)
         {
            // We hit something; adjust and move again
            HandleImpact(moveHit, timeTick, deltaPosition);
            SlideAlongSurface(deltaPosition, (1.f - moveHit.Time), moveHit.Normal, moveHit, true);
         }

         _RelaxDistanceConstraintIfNeeded();
      }

      // Update velocity to reflect actual move
      if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
      {
         Velocity = (UpdatedComponent->GetComponentLocation() - oldLocation) / timeTick;
      }

      bJustTeleported = false;
   }
}

float UOSECharacterMovement::VisualizeLedgeState(float HeightOffset) const
{

   const float OffsetPerElement = 10.0f;
   const FVector TopOfCapsule = GetActorLocation() + FVector(0.f, 0.f, CharacterOwner->GetSimpleCollisionHalfHeight());

   HeightOffset += OffsetPerElement;
   FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);
   const FColor DebugColor = FColor::Purple;
   FString DebugText = FString::Printf(TEXT("WantsToMantle: %d IsInLedgeState: %d IsLedgeMounting: %d"), GetWantsToMantle(), IsInLedgeState(), IsLedgeMounting());
   DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, DebugColor, 0.f, true);

   const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();
   if (ledgeState.MountTarget.IsValid)
   {
      HeightOffset = DrawLedgeState(ledgeState, HeightOffset, DebugColor, OffsetPerElement, DebugLocation, TopOfCapsule);
   }
   if (_DebugLedgeState.MountTarget.IsValid)
   {
      const FColor FailDebugColor = FColor::Red;
      HeightOffset = DrawLedgeState(_DebugLedgeState, HeightOffset, FailDebugColor, OffsetPerElement, DebugLocation, TopOfCapsule);
   }
   return HeightOffset;
}

float UOSECharacterMovement::DrawLedgeState(const FOSELedgeState& ledgeState, float HeightOffset, const FColor DebugColor, const float OffsetPerElement, FVector DebugLocation, const FVector TopOfCapsule) const
{
   //draw capsule transforms
   DrawDebugCapsule(GetWorld(), ledgeState.MountTarget.StartLocation, ledgeState.MountTarget.CapsuleExtents.Z, ledgeState.MountTarget.CapsuleExtents.X, ledgeState.MountTarget.StartDirection.ToOrientationQuat(), DebugColor, false, -1.0f, (uint8)'\000', 0.5f);
   DrawDebugCapsule(GetWorld(), ledgeState.MountTarget.MantleLocation, ledgeState.MountTarget.CapsuleExtents.Z, ledgeState.MountTarget.CapsuleExtents.X, ledgeState.MountTarget.MantleDirection.ToOrientationQuat(), DebugColor, false, -1.0f, (uint8)'\000', 1.0f);

   //draw edge normal
   DrawDebugDirectionalArrow(GetWorld(), ledgeState.MountTarget.MantleLocation, ledgeState.MountTarget.MantleLocation - (ledgeState.MountTarget.MantleDirection * ledgeState.MountTarget.CapsuleExtents.X), 10, DebugColor, false, -1.0f, (uint8)'\000', 0.5f);
   DrawDebugDirectionalArrow(GetWorld(), ledgeState.MountTarget.AnchorLocation, ledgeState.MountTarget.AnchorLocation - (ledgeState.MountTarget.AnchorDirection * ledgeState.MountTarget.CapsuleExtents.X), 10, DebugColor, false, -1.0f, (uint8)'\000', 0.5f);
   DrawDebugSphere(GetWorld(), ledgeState.MountTarget.AnchorLocation, 10, 16, DebugColor, false, -1.0f, (uint8)'\000', 1.0f);


   // Show the height deltas and animations
   {
      HeightOffset += OffsetPerElement;
      DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);

      const float heightResolved = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.StartLocation.Z;
      const float heightActual = ledgeState.MountTarget.MantleLocation.Z - ledgeState.MountTarget.QueryLocation.Z;
      const UAnimMontage* montageAnim = ledgeState.MountTarget.Montage.Get();
      const FString montageText = (montageAnim != nullptr) ? montageAnim->GetName() : TEXT("none");
      const FString mantleText = FString::Printf(TEXT("[%s] Height: %0.2f (Actual: %0.2f)"), *montageText, heightResolved, heightActual);
      DrawDebugString(GetWorld(), DebugLocation, mantleText, nullptr, DebugColor, 0.f, true);
   }
   return HeightOffset;
}

