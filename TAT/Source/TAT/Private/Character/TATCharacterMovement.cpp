// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/TATCharacterMovement.h"

// tat
#include "Developer/TATProjectSettings.h"

// ose
#include "Character/OSECharacterBase.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterMovement)
DEFINE_LOG_CATEGORY_STATIC(LogTATCharacterMovement, Log, All);

namespace MovementCVars
{
   static int32 MantleIgnoreCorrectionMultiplier = 2;
   FAutoConsoleVariableRef CVarMantleIgnoreCorrectionMultiplier(
      TEXT("TAT.Mantle.IgnoreCorrectionMultiplier"),
      MantleIgnoreCorrectionMultiplier,
      TEXT("Ignores corrects during mantling if delta is less than this * NetworkLargeClientCorrectionDistance (0 = correct as normal)"),
      ECVF_Default);

   static float ClientTrustDistance = 50;
   FAutoConsoleVariableRef CVarClientTrustDistance(
      TEXT("TAT.Movement.ClientTrustDistance"),
      ClientTrustDistance,
      TEXT("Distance that the server trusts client positions when the TrustClientMovement tag is active"),
      ECVF_Default);

   static int32 MantleAllowClientAuthority = 1;
   FAutoConsoleVariableRef CVarMantleAllowClientAuthority(
      TEXT("TAT.Mantle.AllowClientAuthority"),
      MantleAllowClientAuthority,
      TEXT("Server will use client moves during a mantle *if* they don't cause a correction (does not give unlimited license)"),
      ECVF_Default);

   static int32 PuddleMovementDebug = 0;
   FAutoConsoleVariableRef CVarPuddleMovementDebug(
      TEXT("TAT.Movement.PuddleDebug"),
      PuddleMovementDebug,
      TEXT("Enable debug drawing for puddle sliding movement"),
      ECVF_Default);
}

namespace MovementHelpers
{
   static void DrawDebugDirection(const AActor* owner, FVector direction, float vertOffset, float arrowLength, FColor color, const TOptional<FString>& extraInfo = NullOpt)
   {
      check(owner != nullptr);
      const UWorld* world = owner->GetWorld();
      check(world != nullptr);
      const FVector origin = owner->GetActorLocation();
      const FVector offset = FVector::UpVector * vertOffset;
      if (direction.Normalize())
      {
         DrawDebugDirectionalArrow(world, origin + offset, origin + offset + (direction * arrowLength), 25.0f, color);
      }
      else
      {
         DrawDebugSphere(world, origin + offset, FMath::Clamp(arrowLength * 0.2f, 10.0f, 50.0f), 12, color);
      }
      if (extraInfo)
      {
         DrawDebugString(world, origin + offset, *extraInfo, nullptr, color, world->GetDeltaSeconds() * 1.05f, true);
      }
   }
}

float FTATCharacterMovementModifierStack::Eval(float baseValue, ETATCharacterMovementModifierStat stat) const
{
   if (stat == ETATCharacterMovementModifierStat::None)
   {
      return baseValue;
   }
   for (const FTATCharacterMovementModifier& mod : Mods)
   {
      // apply the modifier as long as at least one bit in the stat param matches
      if ((mod.Stat & stat) == ETATCharacterMovementModifierStat::None)
      {
         continue;
      }
      switch (mod.Op)
      {
      case ETATCharacterMovementModifierOp::ClampMax:
         if (baseValue > mod.Value)
         {
            baseValue = mod.Value;
         }
         break;
      default:
         checkNoEntry();
         break;
      }
   }
   return baseValue;
}

UTATCharacterMovement::UTATCharacterMovement()
{
   // Levitate Passenger
   {
      MaxLevitatePassengerSpeed = 200;
      BrakingDecelerationLevitatePassenger = 700;
   }

   bWantsInitializeComponent = true;
}

#if WITH_EDITOR
EDataValidationResult UTATCharacterMovement::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult result = Super::IsDataValid(context);

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : result;
}
#endif // WITH_EDITOR

void UTATCharacterMovement::InitializeComponent()
{
   Super::InitializeComponent();

   AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner());
   check(ownerCharacter);
   ownerCharacter->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::_OnOwnerAbilitiesInitialized));
}

void UTATCharacterMovement::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
   {
      // Compute the character's estimated acceleration by just using the change in velocity over time (and without using the Acceleration property).
      _simulatedProxyAcceleration = (Velocity - _velocityLastFrame) / deltaTime;

      if (MovementCVars::PuddleMovementDebug > 0)
      {
         MovementHelpers::DrawDebugDirection(GetOwner(), Velocity, 45.0f, 65.0f, FColor::Orange, FString::Printf(TEXT("%.2f : velocity"), Velocity.Length()));
         MovementHelpers::DrawDebugDirection(GetOwner(), _simulatedProxyAcceleration, 25.0f, 65.0f, FColor::Cyan, FString::Printf(TEXT("%.2f : acceleration"), _simulatedProxyAcceleration.Length()));
      }
   }

   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   _velocityLastFrame = Velocity;
}

float UTATCharacterMovement::GetMaxSpeed() const
{
   ETATCharacterMovementModifierStat stat = ETATCharacterMovementModifierStat::None;
   if (IsSprinting())
      stat = ETATCharacterMovementModifierStat::SprintSpeed;
   else if (IsCrouching())
      stat = ETATCharacterMovementModifierStat::CrouchSpeed;
   else if (IsWalking())
      stat = ETATCharacterMovementModifierStat::WalkSpeed;

   return _movementModifierStack.Eval(Super::GetMaxSpeed(), stat);
}

float UTATCharacterMovement::GetMaxAcceleration() const
{
   return _movementModifierStack.Eval(Super::GetMaxAcceleration(), ETATCharacterMovementModifierStat::Acceleration);
}

void UTATCharacterMovement::UpdateProxyAcceleration()
{
   Super::UpdateProxyAcceleration();

   ensure(GetOwner()->GetLocalRole() == ROLE_SimulatedProxy);

   // 10/24/2025: Disabled for now, as this seems to be causing prediction issues
#if 0
   // For simulated proxies, roughly compute their input acceleration based on their velocity and current surface properties.
   // Only do this when friction or braking deceleration values have been modified.

   // 10/10/2025: This is WIP and will need some additional R&D in the future.
   // It works pretty well when acceleration is zero (because friction is zero, so characters don't slow down on puddles).
   // It does _not_ work particularly well when friction is greater than zero because it struggles to subtract deceleration from friction.
   // Some possible improvements:
   //   - Replicate input vectors (ideally only when sliding). Not cheap on bandwidth.
   //   - Pass sliding state to anim blueprints and have them compensate.
   //     This may or may not be effective because the anim blueprints also rely on having the correct acceleration.
   //   - Improve friction calculation to match exactly how the CMC computes friction normally.
   //     This feels very error-prone to me, and likely to break if we (or Epic) ever change how friction is normally computed.

   // Ideally we'd be using the actual friction and braking deceleration, but that's not computed for simulated proxies, so we have to use an approximation here.
   const float maxBrakingDeceleration = GetMaxBrakingDeceleration();
   float friction = GroundFriction;
   float brakingDeceleration = maxBrakingDeceleration;
   _ComputeFrictionAndBrakingDeceleration(friction, brakingDeceleration);
   constexpr float slipperyThreshold = 0.5f;
   const bool isSlipperySurface = friction <= GroundFriction * slipperyThreshold || brakingDeceleration <= maxBrakingDeceleration * slipperyThreshold;
   if (isSlipperySurface)
   {
      auto getSimulatedProxyEstimatedInputDirection = [this](float normalizedFriction) -> FVector
      {
         if (Velocity.SizeSquared() <= UE_KINDA_SMALL_NUMBER)
         {
            return FVector::ZeroVector;
         }

         if (_simulatedProxyAcceleration.SizeSquared() <= UE_KINDA_SMALL_NUMBER)
         {
            return FVector::ZeroVector;
         }

         // If the velocity and observed net acceleration are similar, assume non-intentional movement (eg. sliding)
         constexpr float directionalAlignmentDotProductThreshold = 0.95f;
         if (FVector::DotProduct(Velocity.GetSafeNormal(), _simulatedProxyAcceleration.GetSafeNormal()) > directionalAlignmentDotProductThreshold)
         {
            return FVector::ZeroVector;
         }

         // Subtract any passive deceleration (eg. sliding) from our current acceleration to get the estimated input direction.
         const FVector passiveDecelerationVector = -Velocity.GetSafeNormal() * normalizedFriction;
         const FVector estimatedInputAcceleration = _simulatedProxyAcceleration - passiveDecelerationVector;
         if (estimatedInputAcceleration.SizeSquared() <= UE_KINDA_SMALL_NUMBER)
         {
            return FVector::ZeroVector;
         }

         return estimatedInputAcceleration.GetSafeNormal();
      };

      const float deltaSeconds = GetWorld()->GetDeltaSeconds();
      const float baseFriction = FMath::Max(0.1f, GroundFriction);
      const float normalizedFriction = FMath::Clamp(FMath::Max(friction, brakingDeceleration), 0.0f, baseFriction) / baseFriction;
      const FVector curEstimatedInput = getSimulatedProxyEstimatedInputDirection(normalizedFriction);
      if (_simulatedProxyInputDirEstimate.IsNearlyZero())
      {
         _simulatedProxyInputDirEstimate = curEstimatedInput;
      }
      else if (curEstimatedInput.IsNearlyZero())
      {
         _simulatedProxyInputDirEstimate = FMath::VInterpConstantTo(_simulatedProxyInputDirEstimate, curEstimatedInput, deltaSeconds, 15.0f);
      }
      else
      {
         _simulatedProxyInputDirEstimate = FMath::VInterpNormalRotationTo(_simulatedProxyInputDirEstimate.GetSafeNormal(), curEstimatedInput, deltaSeconds, 10.0f);
      }

      if (MovementCVars::PuddleMovementDebug > 0)
      {
         MovementHelpers::DrawDebugDirection(GetOwner(), curEstimatedInput, 0.0f, 65.0f, FColor::Emerald, FString::Printf(TEXT("%.2f : input dir (estimate)"), curEstimatedInput.Length()));
         MovementHelpers::DrawDebugDirection(GetOwner(), _simulatedProxyInputDirEstimate, -25.0f, 65.0f, FColor::White, FString::Printf(TEXT("%.2f : input dir (estimate, smoothed)"), _simulatedProxyInputDirEstimate.Length()));
      }

      if (_simulatedProxyInputDirEstimate.SizeSquared() > FMath::Square(0.25))
      {
         Acceleration = _simulatedProxyInputDirEstimate.GetSafeNormal() * GetMaxAcceleration();
      }
      else
      {
         Acceleration = FVector::ZeroVector;
      }
   }
#endif
}

FString UTATCharacterMovement::GetMovementName() const
{
   switch (GetTATCustomMovementType())
   {
   case ETATCustomMovementType::LevitatePassenger:
      return TEXT("LevitatePassenger");

   default:
      return Super::GetMovementName();
   }

}

float UTATCharacterMovement::GetBaseMaxSpeed() const
{
   if (IsLevitatePassenger())
   {
      return MaxLevitatePassengerSpeed;
   }
   else
   {
      return Super::GetBaseMaxSpeed();
   }
}

// Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner.
void UTATCharacterMovement::OnMovementModeChanged(EMovementMode previousMovementMode, uint8 previousCustomMode)
{
   if (!HasValidData())
   {
      return;
   }

   UPrimitiveComponent* oldMovementBase = GetMovementBase();

   Super::OnMovementModeChanged(previousMovementMode, previousCustomMode);

   if (IsLevitatePassenger())
   {
      // the base class OnMovementModeChanged explicitly clears the base if the movement
      // mode changes to anything other than Walking. This restores the base after it has
      // been clobbered
      // Don't bother copying the socket for now
      SetBase(oldMovementBase, NAME_None, false);
   }
}

void UTATCharacterMovement::PhysCustom(float deltaTime, int32 iterations)
{
   switch (GetTATCustomMovementType())
   {
   case ETATCustomMovementType::LevitatePassenger:
      PhysCustomLevitatePassenger(deltaTime, iterations);
      break;

   default:
      Super::PhysCustom(deltaTime, iterations);
      break;
   }
}

void UTATCharacterMovement::PhysCustomLevitatePassenger(float deltaTime, int32 iterations)
{
   if (deltaTime < MIN_TICK_TIME)
   {
      return;
   }

   iterations++;

   RestorePreAdditiveRootMotionVelocity();

   const FVector oldVelocity = Velocity;

   // Apply input
   if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
   {
      // Compute Velocity
      // Acceleration = airControlAcceleration for CalcVelocity(), but we restore it after using it.
      const FVector airControlAcceleration = FVector(Acceleration.X, Acceleration.Y, 0.f);
      TGuardValue<FVector> RestoreAcceleration(Acceleration, airControlAcceleration);
      const float friction = 0.f;
      CalcVelocity(deltaTime, friction, false, BrakingDecelerationLevitatePassenger);

      // Just zero vertical velocity?
      Velocity.Z = 0;
   }

   ApplyRootMotionToVelocity(deltaTime);

   // Default delta position is the velocity change (using midpoint integration method)
   FVector deltaPosition = 0.5f * (oldVelocity + Velocity) * deltaTime;

   // Move; this is combined root motion and the delta position
   FHitResult moveHit(1.0f);
   SafeMoveUpdatedComponent(deltaPosition, UpdatedComponent->GetComponentQuat(), true, moveHit);

   if (moveHit.Time < 1.0f)
   {
      // We hit something; adjust and move again
      HandleImpact(moveHit, deltaTime, deltaPosition);
      SlideAlongSurface(deltaPosition, (1.f - moveHit.Time), moveHit.Normal, moveHit, true);
   }
}

bool UTATCharacterMovement::ServerExceedsAllowablePositionError(float clientTimeStamp, float deltaTime, const FVector& accel,
   const FVector& clientWorldLocation, const FVector& relativeClientLocation, UPrimitiveComponent* clientMovementBase, FName clientBaseBoneName,
   uint8 clientMovementMode)
{
   // Allow tag to temporarily opt-in to ignoring client errors while active. Something to use with care, but
   // the tag is only read by the server, so the client cannot cause this directly.
   if (OSECharOwner->HasMatchingGameplayTag(UTATProjectSettings::Get().TrustClientMovementTag))
   {
      const FVector locDiff = UpdatedComponent->GetComponentLocation() - clientWorldLocation;
      if (locDiff.SizeSquared() <= FMath::Square(MovementCVars::ClientTrustDistance))
      {
#if !UE_BUILD_SHIPPING
         static const IConsoleVariable* CVarNetShowCorrections = IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetShowCorrections"));
         if (CVarNetShowCorrections && CVarNetShowCorrections->GetInt() != 0)
         {
            UE_LOG(LogNetPlayerMovement, Warning, TEXT("*** Server: %s is set to ignore error checks and corrections via tag."), *GetNameSafe(CharacterOwner));
         }
#endif
         return false;
      }
   }


   // Increase threshold for corrections during mantling to avoid micro-corrections where the montage
   // play position is just slightly off. These tend to resolve themselves, but may not fully.
   // Certainly does not absolve from digging
   const uint8 currentPackedMovementMode = PackNetworkMovementMode();
   if(currentPackedMovementMode == clientMovementMode && CustomMovementMode == static_cast<uint8>(ECustomMovementType::Mantle))
   {
      const FVector locDiff = UpdatedComponent->GetComponentLocation() - clientWorldLocation;
      if(locDiff.SizeSquared() <= FMath::Square(NetworkLargeClientCorrectionDistance * MovementCVars::MantleIgnoreCorrectionMultiplier))
      {
         return false;
      }
   }
   
   return Super::ServerExceedsAllowablePositionError(clientTimeStamp, deltaTime, accel, clientWorldLocation, relativeClientLocation,
                                                     clientMovementBase,
                                                     clientBaseBoneName, clientMovementMode);
}

bool UTATCharacterMovement::ServerShouldUseAuthoritativePosition(float clientTimeStamp, float deltaTime, const FVector& accel,
   const FVector& clientWorldLocation, const FVector& relativeClientLocation, UPrimitiveComponent* clientMovementBase, FName clientBaseBoneName,
   uint8 clientMovementMode)
{
   // Server will use client moves during a mantle *if* they don't cause a correction (does not give unlimited license)
   if(CustomMovementMode == static_cast<uint8>(ECustomMovementType::Mantle) )
   {
      // deliberately only including server view of movement mode to give some additional fudge when exiting mantle (if client exists first)
      return true;
   }

   // Allow tag to temporarily opt-in to ignoring client errors while active. Something to use with care, but
   // the tag is only read by the server, so the client cannot cause this directly.
   // Server will use client moves during a mantle *if* they don't cause a correction (does not give unlimited license)
   if (OSECharOwner->HasMatchingGameplayTag(UTATProjectSettings::Get().TrustClientMovementTag))
   {
      return true;
   }
   
   return Super::ServerShouldUseAuthoritativePosition(clientTimeStamp, deltaTime, accel, clientWorldLocation, relativeClientLocation,
                                                      clientMovementBase,
                                                      clientBaseBoneName, clientMovementMode);
}

void UTATCharacterMovement::OnUnableToFollowBaseMove(const FVector& deltaPosition, const FVector& oldLocation, const FHitResult& moveOnBaseHit)
{
   Super::OnUnableToFollowBaseMove(deltaPosition, oldLocation, moveOnBaseHit);

   if (IsLevitatePassenger())
   {
      // Try sliding along surface if the base move fails while in "LevitatePassenger"
      // This may have some risks, but without it, characters against a wall can get stuck and fail to levitate
      FHitResult hitCopy = moveOnBaseHit;
      SlideAlongSurface(deltaPosition, (1.f - moveOnBaseHit.Time), moveOnBaseHit.Normal, hitCopy, /*handleImpact*/ false);
   }
}

bool UTATCharacterMovement::GetWantsToMantle() const
{
   if (Super::GetWantsToMantle())
   {
      return true;
   }

   if (TryToAutoMantleDuringScramble && IsScrambling())
   {
      return true;
   }

   return false;
}

bool UTATCharacterMovement::CanScrambleInCurrentState() const
{
   // Can't be levitate passenger
   if (IsLevitatePassenger())
      return false;

   return Super::CanScrambleInCurrentState();
}

bool UTATCharacterMovement::CanMantleInCurrentState() const
{
   // Can't be levitate passenger
   if (IsLevitatePassenger())
      return false;

   return Super::CanMantleInCurrentState();
}

void UTATCharacterMovement::PopulatePhysicalMaterialContext(FOSEMovementMaterialContext& newContext) const
{
   Super::PopulatePhysicalMaterialContext(newContext);

   // TODO: is this useful?
   newContext.IsInAir |= IsLevitatePassenger();
}

void UTATCharacterMovement::_ComputeFrictionAndBrakingDeceleration(float& inOutFriction, float& inOutBrakingDeceleration) const
{
   Super::_ComputeFrictionAndBrakingDeceleration(inOutFriction, inOutBrakingDeceleration);

   if (IsMovingOnGround())
   {
      // Allow the movement modifier stack to adjust ground fiction and braking deceleration
      inOutFriction = _movementModifierStack.Eval(inOutFriction, ETATCharacterMovementModifierStat::Friction);
      inOutBrakingDeceleration = _movementModifierStack.Eval(inOutBrakingDeceleration, ETATCharacterMovementModifierStat::BrakingDeceleration);
   }
}

void UTATCharacterMovement::_OnOwnerAbilitiesInitialized()
{
   // Cache ASC for momentum evaluation
   _ownerAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   check(_ownerAbilitySystemComponent.IsValid());
   UE_LOG(LogTATCharacterMovement, VeryVerbose, TEXT("_OnOwnerAbilitiesInitialized() | cached ASC for owner %s"), *GetOwner()->GetName());
}

