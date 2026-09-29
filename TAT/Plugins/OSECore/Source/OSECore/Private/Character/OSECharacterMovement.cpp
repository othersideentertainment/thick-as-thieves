// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "Character/OSECharacterMovement.h"
#include "Character/OSECharacterBase.h"
#include "Traversal/Mantle/OSEMantleQuery.h"
#include "Traversal/Mantle/OSELedgeState.h"
#include "Traversal/Mantle/OSEMantleAnimSet.h"
#include "OSECommon.h"
#include "GameFramework/PlayerController.h"

// UE
#include "CommonInputSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Misc/DataValidation.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECharacterMovement)


DEFINE_LOG_CATEGORY_STATIC(LogOSECharacterMovement, Log, All);

namespace MovementCVars
{
   static int32 LedgeEnableLedgeMantleCheck = 1;
   FAutoConsoleVariableRef CVarEnableLedgeMantle(
      TEXT("OSE.Ledge.EnableLedgeMantle"),
      LedgeEnableLedgeMantleCheck,
      TEXT("Enables the ledge mantle rather than the original mantle system"),
      ECVF_Default);

   static int32 JumpEnableClearanceCheck = 1;
   FAutoConsoleVariableRef CVarEnableClearanceCheck(
      TEXT("OSE.Jump.EnableClearanceCheck"),
      JumpEnableClearanceCheck,
      TEXT("Enables the clearance check for jumping on slopes, which will modify the horizontal and vertical speed in order to clear the apex"),
      ECVF_Default);

   static int32 JumpAlwaysProjectVelocity = 0;
   FAutoConsoleVariableRef CVarAlwaysProjectVelocity(
      TEXT("OSE.Jump.AlwaysProjectVelocity"),
      JumpAlwaysProjectVelocity,
      TEXT("Jumping on slopes only projects velocity when moving upwards. This will always project it, regardless if velocity is upwards or downwards. Requires clearance check to be enabled."),
      ECVF_Default);

   static int32 ClimbRemoveLateralInput = 1;
   FAutoConsoleVariableRef CVarClimbRemoveLateralInput(
      TEXT("OSE.Climb.RemoveLateralInput"),
      ClimbRemoveLateralInput,
      TEXT("Entirely removes all lateral input while climbing"),
      ECVF_Default);

   static float ClimbLateralDamping = 5;
   FAutoConsoleVariableRef CVarClimbLateralDamping(
      TEXT("OSE.Climb.LateralDamping"),
      ClimbLateralDamping,
      TEXT("Damping applied to lateral velocity when climbing"),
      ECVF_Default);

   static int32 ClimbCylinderCheck = 0;
   FAutoConsoleVariableRef CVarClimbCylinderCheck(
      TEXT("OSE.Climb.CylinderCheck"),
      ClimbCylinderCheck,
      TEXT("Compares the impact locations against the cylinder"),
      ECVF_Default);

   static float ClimbCylinderBuffer = 5;
   FAutoConsoleVariableRef CVarClimbCylinderBuffer(
      TEXT("OSE.Climb.CylinderBuffer"),
      ClimbCylinderBuffer,
      TEXT("Buffer distance when comparing climb impacts against the cylinder"),
      ECVF_Default);

   static float ClimbRadiusBuffer = 10;
   FAutoConsoleVariableRef CVarClimbRadiusBuffer(
      TEXT("OSE.Climb.RadiusBuffer"),
      ClimbRadiusBuffer,
      TEXT("Capsule radius buffer (shrink amount) when checking head clearance, or when performing forward casts"),
      ECVF_Default);

   static float ClimbWallGravityScale = 2;
   FAutoConsoleVariableRef CVarClimbWallGravityScale(
      TEXT("OSE.Climb.WallGravityScale"),
      ClimbWallGravityScale,
      TEXT("Scale factor for an additional gravity pulling towards the wall while climbing"),
      ECVF_Default);

   static int32 ClimbCancelWhenMovingAway = 1;
   FAutoConsoleVariableRef CVarClimbCancelWhenMovingAway(
      TEXT("OSE.Climb.CancelWhenMovingAway"),
      ClimbCancelWhenMovingAway,
      TEXT("Cancels climbing when the user moves away from the wall"),
      ECVF_Default);

   static int32 ClimbForwardCastEnabled = 0;
   FAutoConsoleVariableRef CVarClimbForwardCastEnabled(
      TEXT("OSE.Climb.ForwardCast.Enabled"),
      ClimbForwardCastEnabled,
      TEXT("Enables the forward clearance cast"),
      ECVF_Default);

   static float ClimbForwardCastDistance = 15;
   FAutoConsoleVariableRef CVarClimbForwardCastDistance(
      TEXT("OSE.Climb.ForwardCast.Distance"),
      ClimbForwardCastDistance,
      TEXT("Distance to cast forward when checking forward clearance"),
      ECVF_Default);

   static int32 ClimbBlocksSprint = 1;
   FAutoConsoleVariableRef CVarClimbBlocksSprint(
      TEXT("OSE.Climb.BlocksSprint"),
      ClimbBlocksSprint,
      TEXT("Whether sprint is prevented while climbing (default 1)"),
      ECVF_Default);

   static float TurnInPlaceCenteredThreshold = 0.1f;
   FAutoConsoleVariableRef CVarTurnInPlaceCenteredThreshold(
      TEXT("OSE.TurnInPlace.CenteredThreshold"),
      TurnInPlaceCenteredThreshold,
      TEXT("Tolerance in degrees for determining if we've centered for turn-in-place"),
      ECVF_Default);

   static float TurnInPlaceRateWalkSpeedScalar = 0.5f;
   FAutoConsoleVariableRef CVarTurnInPlaceRateWalkSpeedScalar(
      TEXT("OSE.TurnInPlace.Rate.WalkSpeedScalar"),
      TurnInPlaceRateWalkSpeedScalar,
      TEXT("Considers the walk speed times this value when computing rotation rate"),
      ECVF_Default);

   static float TurnInPlaceRateCrouchSpeedScalar = 1.0f;
   FAutoConsoleVariableRef CVarTurnInPlaceRateCrouchSpeedScalar(
      TEXT("OSE.TurnInPlace.Rate.CrouchSpeedScalar"),
      TurnInPlaceRateCrouchSpeedScalar,
      TEXT("Considers the crouch speed times this value when computing rotation rate"),
      ECVF_Default);

   static float TurnInPlaceRateQuantization = 15.0f;
   FAutoConsoleVariableRef CVarTurnInPlaceRateQuantization(
      TEXT("OSE.TurnInPlace.Rate.Quantization"),
      TurnInPlaceRateQuantization,
      TEXT("Quantizes the rotation rate (degrees/sec) by this value"),
      ECVF_Default);

   static int32 TurnInPlaceStopOnDirectionChange = 1;
   FAutoConsoleVariableRef CVarTurnInPlaceStopOnDirectionChange(
      TEXT("OSE.TurnInPlace.StopOnDirectionChange"),
      TurnInPlaceStopOnDirectionChange,
      TEXT("Will stop turn-in-place if the direction changes"),
      ECVF_Default);

   static float TurnInPlaceTimeDecayScalar = 0.7f;
   FAutoConsoleVariableRef CVarTurnInPlaceTimeDecayScalar(
      TEXT("OSE.TurnInPlace.TimeDecayScalar"),
      TurnInPlaceTimeDecayScalar,
      TEXT("Modifies the rate that the timer decays when within normal angle thresholds"),
      ECVF_Default);

   static int32 MantleKeepMomentum = 0;
   FAutoConsoleVariableRef CVarMantleKeepMomentum(
      TEXT("OSE.Mantle.KeepMomentum"),
      MantleKeepMomentum,
      TEXT("Attempts to maintain initial speed as the mantle completes"),
      ECVF_Default);

   static int32 MantleScaleRootMotionZ = 1;
   FAutoConsoleVariableRef CVarMantleScaleRootMotionZ(
      TEXT("OSE.Mantle.ScaleRootMotionZ"),
      MantleScaleRootMotionZ,
      TEXT("Scales the root motion Z value to reach the target height"),
      ECVF_Default);

   static float MantleFloorCheckDistance = UCharacterMovementComponent::MAX_FLOOR_DIST;
   FAutoConsoleVariableRef CVarMantleFloorCheckDistance(
      TEXT("OSE.Mantle.FloorCheckDistance"),
      MantleFloorCheckDistance,
      TEXT("Transition to ground movement this close to the floor at the end of the mantle"),
      ECVF_Default);

   static int32 MantleBlocksSprint = 1;
   FAutoConsoleVariableRef CVarMantleBlocksSprint(
      TEXT("OSE.Mantle.BlocksSprint"),
      MantleBlocksSprint,
      TEXT("Whether sprint is prevented while mantling (default 1)"),
      ECVF_Default);

   static bool MantleCancelIfMontageEnded = true;
   FAutoConsoleVariableRef CVarMantleCancelIfMontageEnded(
      TEXT("OSE.Mantle.CancelIfMontageEnded"),
      MantleCancelIfMontageEnded,
      TEXT("Cancel Mantling if montage ends prematurely"),
      ECVF_Default);

   static float MantleMaxVerticalExitSpeed = -1;
   FAutoConsoleVariableRef CVarMaxVerticalExitSpeed(
      TEXT("OSE.Mantle.MaxVerticalExitSpeed"),
      MantleMaxVerticalExitSpeed,
      TEXT("Max vertical speed to allow when exiting mantle (-1 = none)"),
      ECVF_Default);

   static float MantleMaxLateralExitSpeed = -1;
   FAutoConsoleVariableRef CVarMaxLateralExitSpeed(
      TEXT("OSE.Mantle.MaxLateralExitSpeed"),
      MantleMaxVerticalExitSpeed,
      TEXT("Max lateral speed to allow when exiting mantle (-1 = none)"),
      ECVF_Default);

   static int32 DistanceConstraintClientForcePartialUpdates = 0;
   FAutoConsoleVariableRef CVarDistanceConstraintClientForcePartialUpdates(
      TEXT("OSE.DistanceConstraint.ClientForcePartialUpdates"),
      DistanceConstraintClientForcePartialUpdates,
      TEXT("Force partial updates of distance constraint to be sent when it changes from extension/climbing"),
      ECVF_Default);

   static float MaxSpeedMultiplierCap = 2;
   FAutoConsoleVariableRef CVarMaxSpeedMultiplierCap(
      TEXT("OSE.MaxSpeedMultiplier.Cap"),
      MaxSpeedMultiplierCap,
      TEXT("The largest MaxSpeedMultiplier allowed"),
      ECVF_Default);

   static float EnableSlopesAffectSpeed = 1;
   FAutoConsoleVariableRef CVarEnableSlopesAffectSpeed(
      TEXT("OSE.EnableSlopesAffectSpeed"),
      EnableSlopesAffectSpeed,
      TEXT("Enables slopes affecting the max speed. Requires Character movement component to enable SlopesAffectMaxSpeed"),
      ECVF_Default);

   static float NegativeCollisionTraceDistance = 250.0f;
   FAutoConsoleVariableRef CVarNegativeCollisionTraceDistance(
      TEXT("OSE.Climb.NegativeCollisionTraceDistance"),
      NegativeCollisionTraceDistance,
      TEXT("Distance we trace out from the character to detect negative collisions"),
      ECVF_Default);

   static int32 DebugNegativeCollisions = 0;
   FAutoConsoleVariableRef CVarDebugNegativeCollisions(
      TEXT("OSE.WallClimb.DebugNegativeCollisions"),
      DebugNegativeCollisions,
      TEXT("Shows the ray casts involved in detecting negative collisions"),
      ECVF_Default);

   static int32 UseCrouchCapsuleForWallClimb = 0;
   FAutoConsoleVariableRef CVarUseCrouchCapsuleForWallClimb(
      TEXT("OSE.WallClimb.UseCrouchCapsuleForWallClimb"),
      UseCrouchCapsuleForWallClimb,
      TEXT("This allows the use of the crouch capsule size for wall climbing"),
      ECVF_Default);

   static int32 DebugLedgeDetection = 0;
   FAutoConsoleVariableRef CVarDebugLedgeDetection(
      TEXT("OSE.Vault.DebugLedgeDetection"),
      DebugLedgeDetection,
      TEXT("Shows the line traces involved in detecting vaulting ledges"),
      ECVF_Default);
}


//---------------------------------------------------------------------------------------
// Saved move for client side prediction, server validation, and replays
//---------------------------------------------------------------------------------------

FOSESavedMove_Character::FOSESavedMove_Character()
   : Super()
   , bWantsToSprint(false)
   , bIsSliding(false)
   , bDistanceConstraintEnabled(false)
   , bExtendingDistanceConstraint(false)
   , bWantsToContractDistanceConstraint(false)
   , bWantsToScramble(false)
   , bWantsToWallClimb(false)
   , bWantsToMantle(false)
   , DistanceConstraintDelta(EDistanceConstraintDelta::None)
   , MaxSpeedMultiplier(1)
   , ExtraGravityScale(1.f)
   , SavedMantleState()
   , SavedMantleMontage(nullptr)
   , SavedMantleTimeElapsed(0.0f)
   , SavedMantleTimeRemaining(0.0f)
   , SavedMantleStartOffset(FVector::ZeroVector)
   , SavedMantleStartVelocity(FVector::ZeroVector)
   , SavedLedgeState()
   , SavedLedgeMontage(nullptr)
   , SavedLedgeMountTimeElapsed(0.0f)
   , SavedLedgeMountTimeRemaining(0.0f)
   , SavedLedgeMountStartOffset(FVector::ZeroVector)
   , SavedLedgeMountStartVelocity(FVector::ZeroVector)
{

}

void FOSESavedMove_Character::Clear()
{
   Super::Clear();

   bWantsToSprint = false;
   bIsSliding = false;
   bDistanceConstraintEnabled = false;
   bExtendingDistanceConstraint = false;
   bWantsToContractDistanceConstraint = false;
   bWantsToScramble = false;
   bWantsToWallClimb = false;
   bWantsToMantle = false;

   MaxSpeedMultiplier = 1.f;
   ExtraGravityScale = 1.f;

   SavedMantleState = FOSEMantleState();
   SavedMantleMontage = nullptr;
   SavedMantleTimeElapsed = 0.0f;
   SavedMantleTimeRemaining = 0.0f;
   SavedMantleStartOffset = FVector::ZeroVector;
   SavedMantleStartVelocity = FVector::ZeroVector;
   SavedMantleHadRootMotion = false;


   SavedLedgeState = FOSELedgeState();
   SavedLedgeMontage = nullptr;
   SavedLedgeMountTimeElapsed = 0.0f;
   SavedLedgeMountTimeRemaining = 0.0f;
   SavedLedgeMountStartOffset = FVector::ZeroVector;
   SavedLedgeMountStartVelocity = FVector::ZeroVector;

   DistanceConstraintDelta = EDistanceConstraintDelta::None;
   DistanceConstraint = FOSEDistanceConstraint();
}

uint8 FOSESavedMove_Character::GetCompressedFlags() const
{
   uint8 Result = Super::GetCompressedFlags();

   if (bWantsToSprint)
   {
      Result |= FLAG_Custom_0;
   }

   if (bIsSliding)
   {
      Result |= FLAG_Custom_1;
   }

   if (bWantsToMantle)
   {
      Result |= FLAG_Custom_2;
   }

   if (bWantsToScramble)
   {
      Result |= FLAG_Custom_3;
   }

   return Result;
}

bool FOSESavedMove_Character::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
   if (auto NewSavedMove = (const FOSESavedMove_Character*)NewMove.Get())
   {
      if (bDistanceConstraintEnabled != NewSavedMove->bDistanceConstraintEnabled)
      {
         return false;
      }

      // @TODO: Check if it's necessary to avoid move combining while
      // distance constraint is enabled, or just when the state changes
      if (bDistanceConstraintEnabled)
      {
         return false;
      }

      if (bDistanceConstraintEnabled && ((bWantsToContractDistanceConstraint != NewSavedMove->bWantsToContractDistanceConstraint) || (bExtendingDistanceConstraint != NewSavedMove->bExtendingDistanceConstraint)))
      {
         return false;
      }

      if (bWantsToWallClimb != NewSavedMove->bWantsToWallClimb)
      {
         return false;
      }

      // max speed multiplier is already covered by max-speed check in Super

      // Changes in actual mantle state
      // NOTE, should only be true when IsValid is also true
      if (SavedMantleState.IsMantling != NewSavedMove->SavedMantleState.IsMantling)
      {
         return false;
      }

      // Changes in mantle state validity
      if (SavedMantleState.IsValid != NewSavedMove->SavedMantleState.IsValid)
      {
         return false;
      }

      // Other saved mantle state only matters when valid
      if (SavedMantleState.IsValid)
      {
         if (SavedMantleState != NewSavedMove->SavedMantleState)
         {
            return false;
         }
      }

      // Some of the transient mantle state also needs to be considered
      {
         if (SavedMantleMontage != NewSavedMove->SavedMantleMontage)
         {
            return false;
         }

         if (SavedMantleTimeElapsed != NewSavedMove->SavedMantleTimeElapsed)
         {
            return false;
         }

         if (SavedMantleTimeRemaining != NewSavedMove->SavedMantleTimeRemaining)
         {
            return false;
         }

         if (!SavedMantleStartOffset.Equals(NewSavedMove->SavedMantleStartOffset))
         {
            return false;
         }

         if (!SavedMantleStartVelocity.Equals(NewSavedMove->SavedMantleStartVelocity))
         {
            return false;
         }
      }

      // Changes in actual Ledge state
      // NOTE, should only be true when IsValid is also true
      if (SavedLedgeState.IsLedgeStateActive != NewSavedMove->SavedLedgeState.IsLedgeStateActive)
      {
         return false;
      }

      // Changes in Ledge state validity
      if (SavedLedgeState.MountTarget.IsValid != NewSavedMove->SavedLedgeState.MountTarget.IsValid)
      {
         return false;
      }

      // Other saved Ledge state only matters when valid
      if (SavedLedgeState.MountTarget.IsValid)
      {
         if (SavedLedgeState != NewSavedMove->SavedLedgeState)
         {
            return false;
         }
      }

      // Some of the transient Ledge state also needs to be considered
      {
         if (SavedLedgeMontage != NewSavedMove->SavedLedgeMontage)
         {
            return false;
         }

         if (SavedLedgeMountTimeElapsed != NewSavedMove->SavedLedgeMountTimeElapsed)
         {
            return false;
         }

         if (SavedLedgeMountTimeRemaining != NewSavedMove->SavedLedgeMountTimeRemaining)
         {
            return false;
         }

         if (!SavedLedgeMountStartOffset.Equals(NewSavedMove->SavedLedgeMountStartOffset))
         {
            return false;
         }

         if (!SavedLedgeMountStartVelocity.Equals(NewSavedMove->SavedLedgeMountStartVelocity))
         {
            return false;
         }
      }
   }

   return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

bool FOSESavedMove_Character::IsImportantMove(const FSavedMovePtr& lastAckedMove) const
{
   if (Super::IsImportantMove(lastAckedMove))
   {
      return true;
   }

   if (DistanceConstraintDelta != EDistanceConstraintDelta::None)
   {
      return true;
   }

   if (auto lastMove = (const FOSESavedMove_Character*)lastAckedMove.Get())
   {
      if (SavedMantleState.IsValid != lastMove->SavedMantleState.IsValid)
      {
         return true;
      }

      if (SavedMantleState.IsMantling != lastMove->SavedMantleState.IsMantling)
      {
         return true;
      }

      if (SavedLedgeState.IsLedgeStateActive != lastMove->SavedLedgeState.IsLedgeStateActive)
      {
         return true;
      }

      if (SavedLedgeState.MountTarget.IsValid != lastMove->SavedLedgeState.MountTarget.IsValid)
      {
         return true;
      }

   }

   return false;
}

// Set the properties describing the position, etc. of the moved pawn at the start of the move.
void FOSESavedMove_Character::SetInitialPosition(ACharacter* character)
{
   Super::SetInitialPosition(character);

   // Mantle state is applied here as some of it may affect position
   // as data used in the custom movement mode
   {
      if (const auto charBase = Cast<AOSECharacterBase>(character))
      {
         SavedMantleState = charBase->GetMantleState();
      }

      if (const auto charMovement = Cast<UOSECharacterMovement>(character->GetCharacterMovement()))
      {
         SavedMantleMontage = charMovement->GetMantleMontage();
         SavedMantleTimeElapsed = charMovement->GetMantleTimeElapsed();
         SavedMantleTimeRemaining = charMovement->GetMantleTimeRemaining();
         SavedMantleStartOffset = charMovement->GetMantleStartOffset();
         SavedMantleStartVelocity = charMovement->GetMantleStartVelocity();
         SavedMantleHadRootMotion = charMovement->_mantleHadRootMotion;



         SavedLedgeMontage = charMovement->GetLedgeMountMontage();
         SavedLedgeMountTimeElapsed = charMovement->GetLedgeMountTimeElapsed();
         SavedLedgeMountTimeRemaining = charMovement->GetLedgeMountTimeRemaining();
         SavedLedgeMountStartOffset = charMovement->GetLedgeMountStartOffset();
         SavedLedgeMountStartVelocity = charMovement->GetLedgeMountStartVelocity();
      }
   }
}

void FOSESavedMove_Character::SetMoveFor(ACharacter* character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
   Super::SetMoveFor(character, InDeltaTime, NewAccel, ClientData);

   // Base class calls SetInitialPosition
   //    SavedMantleState
   //    SavedMantleMontage
   //    SavedMantleTimeElapsed
   //    SavedMantleTimeRemaining
   //    SavedMantleStartOffset
   //    SavedMantleStartVelocity

   if (const auto charMovement = Cast<UOSECharacterMovement>(character->GetCharacterMovement()))
   {
      bWantsToSprint = charMovement->bWantsToSprint;
      bIsSliding = charMovement->IsSliding();
      bDistanceConstraintEnabled = charMovement->GetDistanceConstraintEnabled();
      bExtendingDistanceConstraint = charMovement->IsExtendingDistanceConstraint();
      bWantsToContractDistanceConstraint = charMovement->IsContractingDistanceConstraint();
      bWantsToScramble = charMovement->GetWantsToScramble();
      bWantsToWallClimb = charMovement->GetWantsToWallClimb();
      bWantsToMantle = charMovement->GetWantsToMantle();

      DistanceConstraintDelta = charMovement->_lastDistanceConstraintDelta;
      DistanceConstraint = charMovement->_distanceConstraintDesired;
      MaxSpeedMultiplier = charMovement->_maxSpeedMultiplier;
      ExtraGravityScale = charMovement->_extraGravityScale;
   }
}

// Called before ClientUpdatePosition uses this SavedMove to make a predictive correction
void FOSESavedMove_Character::PrepMoveFor(ACharacter* character)
{
   Super::PrepMoveFor(character);

   if (auto charBase = Cast<AOSECharacterBase>(character))
   {
      charBase->SetMantleState(SavedMantleState);
      charBase->SetLedgeState(SavedLedgeState);
   }

   if (auto charMovement = Cast<UOSECharacterMovement>(character->GetCharacterMovement()))
   {
      charMovement->SetMantleTransients(*this);
      charMovement->SetLedgeTransients(*this);

      charMovement->SetWantsToWallClimb(bWantsToWallClimb);

      if (charMovement->_clientResimulateDistanceConstraint)
      {
         // if re-simulating the constraint, save this off in case we simulate again it again
         DistanceConstraint = charMovement->_distanceConstraintDesired;
      }
      else
      {
         // otherwise, use the saved constraint
         charMovement->_distanceConstraintDesired = DistanceConstraint;
      }
   }
}

void FOSESavedMove_Character::PostUpdate(ACharacter* character, EPostUpdateMode postUpdateMode)
{
   Super::PostUpdate(character, postUpdateMode);

   if (auto charMovement = Cast<UOSECharacterMovement>(character->GetCharacterMovement()))
   {
      // Set end distance for use in correction check
      if (bDistanceConstraintEnabled && (postUpdateMode == FSavedMove_Character::PostUpdate_Record || charMovement->_clientResimulateDistanceConstraint))
      {
         DistanceConstraintEndDistance = charMovement->GetDistanceConstraint().Distance;
      }
   }
}


//---------------------------------------------------------------------------------------
// Custom character movement class
//---------------------------------------------------------------------------------------

// Default initialization for the elapsed time since valid climb impact
const float UOSECharacterMovement::DEFAULT_TIME_SINCE_IMPACT = 420.69f;

// Default initialization for wall climb movement mode
const float UOSECharacterMovement::MIN_WALL_DIST = 1.9f;
const float UOSECharacterMovement::MAX_WALL_DIST = 2.4f;

UOSECharacterMovement::UOSECharacterMovement(const FObjectInitializer& ObjectInitializer /* = FObjectInitializer::Get() */)
   : Super(ObjectInitializer)
{
   // General
   {
      bMaintainHorizontalGroundVelocity = false;
      MaxAcceleration = 1500.0f;

      // From docs: This is 2 by default for historical reasons, a value of 1 gives the true drag equation
      BrakingFrictionFactor = 1.0f;

      // From docs: Helps remove frame-rate dependent jump height, but may alter base jump height
      // We need gravity on while jumping since we may add certain forces while jumping,
      // or other behaviors which need to be consistent with other movement modes or states.
      bApplyGravityWhileJumping = true;

      // These properties have related effects, but different defaults. When `bUseAccelerationForPaths` (default=false) is true,
      // RequestPathMove() is used instead of RequestDirectMove() for path following. This is similar to `bRequestedMoveUseAcceleration`
      // (default=true) in that it will compute the input acceleration needed to arrive at the desired velocity. This adds movement
      // input to the pawn just like player input. This allows us to handle movement input similarly for players and AI, as well as
      // handling when movement input is ignored, etc.
      NavMovementProperties.bUseAccelerationForPaths = true;
      bRequestedMoveUseAcceleration = true;
      PRAGMA_DISABLE_DEPRECATION_WARNINGS
      // Also set the deprecated field, so that UNavMovementComponent::Serialize does not clobber the value when trying to migrate it
      bUseAccelerationForPaths_DEPRECATED = true;
      PRAGMA_ENABLE_DEPRECATION_WARNINGS
   }

   // Walking
   {
      MaxWalkSpeed = 300;
      BrakingDecelerationWalking = 1000.0f;
   }

   // Crouching
   {
      MaxWalkSpeedCrouched = 160;
      NavAgentProps.bCanCrouch = true;
      bCanWalkOffLedgesWhenCrouching = bCanWalkOffLedges;

      _crouchJumpSettings.AllowJumpWhileCrouched = true;
      _crouchJumpSettings.UncrouchWhenJumping = true;
   }

   // Sprinting
   {
      bWantsToSprint = false;
      SprintSpeedIncrementBase = 200;
      SprintSpeedIncrementForward = SprintSpeedIncrementBase;
   }

   // Sliding
   {
      bIsSliding = false;
      SlidingEnabled = true;
      SlidingSettings.BrakingDeceleration = BrakingDecelerationFalling;
      SlidingSettings.FrictionFactor = 0.1f;
   }

   // Distance constraint
   {
      _affectedByDistanceConstraint = false;
      SwingAirControl = 0.1f;
      SwingAirControlBoostMultiplier = 2.f;
      SwingAirControlBoostVelocityThreshold = 25.f;
   }

   // Mantle
   {
      _wantsToMantle = false;
      _mantleEnabled = false;
      _mantleMontage = nullptr;
      _mantleTimeElapsed = 0.0f;
      _mantleTimeRemaining = 0.0f;
      _mantleStartOffset = FVector::ZeroVector;
      _mantleStartVelocity = FVector::ZeroVector;
   }

   // Ledge
   {
      _shimmyEnabled = false;
      _ledgeHangEnabled = false;
   }

   // Scrambling
   {
      _wantsToScramble = false;
      _scrambleEnabled = false;
      _scramblePendingResult.Init();
      _scrambleImpactResult.Init();
      _scrambleImpactElapsed = DEFAULT_TIME_SINCE_IMPACT;
      _scrambleDurationElapsed = 0.0f;
      _scrambleIsBlocked = false;
      _scrambleClearance = 0.0f;
      _scrambleForwardClearance = false;

      _scrambleSettings.JumpEnabled = true;
      _scrambleSettings.JumpPhysics.Velocity.X = JumpZVelocity * -1.0f;
      _scrambleSettings.JumpPhysics.Velocity.Z = JumpZVelocity * +1.0f;
   }

   // Wall Climb
   {
      _wantsToWallClimb = false;
      _wallClimbEnabled = false;
      _bIsWallDashing = false;
      _bClearedInitialWallClimbFloor = false;
      _wallClimbPendingResult.Init();
      _wallClimbImpactResult.Init();
      _wallClimbDashLocalAccelNormal = FVector::ZeroVector;
      _rotationInputThisFrame = FRotator::ZeroRotator;
      _wallClimbAutoTurnTarget = FRotator::ZeroRotator;
      _wallClimbAutoTurnTarget = FRotator::ZeroRotator;
      _bWallClimbCamAutoTurning = false;
      _bManuallyCanceledWallClimb = false;

      bForceNextWallCheck = true;
      bAlwaysCheckWall = true;
   }

   // Falling
   {
      _maxFallingSpeedOverride = MaxWalkSpeed;
   }

   _maxSpeedMultiplier = 1.f;

   SetNetworkMoveDataContainer(_networkMoveDataContainer);
   SetMoveResponseDataContainer(_responseDataContainer);
}

bool UOSECharacterMovement::ClientUpdatePositionAfterServerUpdate()
{
   if (!HasValidData())
   {
      return false;
   }

   // Save off values before calling base class
   const bool bRealWantsToSprint = bWantsToSprint;
   const bool bRealWantsToClimb = GetWantsToScramble();
   const bool bRealWantsToWallClimb = GetWantsToWallClimb();
   const bool bRealWantsToMantle = GetWantsToMantle();

   const bool bRealExtendingDistanceConstraint = _extendingDistanceConstraint;
   const bool bRealWantsToContractDistanceConstraint = _wantsToContractDistanceConstraint;
   
   const FOSEMantleState origMantleState = OSECharOwner->GetMantleState();
   const FOSELedgeState realLedgeState = OSECharOwner->GetLedgeState();

   const EDistanceConstraintDelta distanceConstraintDelta = _lastDistanceConstraintDelta;
   const FOSEDistanceConstraint realDistanceConstraint = _distanceConstraintDesired;

   const float realMaxSpeedMultiplier = _maxSpeedMultiplier;
   const float realExtraGravityScale = _extraGravityScale;

   const bool bResult = Super::ClientUpdatePositionAfterServerUpdate();

   // Restore values and return original result
   bWantsToSprint = bRealWantsToSprint;
   SetWantsToScramble(bRealWantsToClimb);
   SetWantsToWallClimb(bRealWantsToWallClimb);
   // TODO: Since wantsToMantle is cleared on mantle start,
   //       this could stay cleared even if the mantle does not
   //       start in the correction. Fixing this may require
   //       the purely-inputed-based portions of wantsToMantle.
   SetWantsToMantle(bRealWantsToMantle);

   _extendingDistanceConstraint = bRealExtendingDistanceConstraint;
   _wantsToContractDistanceConstraint = bRealWantsToContractDistanceConstraint;

   // If the mantling changed during the correction, fire client callbacks now
   const bool isMantling = OSECharOwner->GetMantleState().IsMantling;
   if (origMantleState.IsMantling != isMantling)
   {
      if (isMantling)
      {
         OSECharOwner->OnStartMantling();
      }
      else
      {
         OSECharOwner->OnStopMantling();
      }
   }

   _maxSpeedMultiplier = realMaxSpeedMultiplier;
   _extraGravityScale = realExtraGravityScale;

   _lastDistanceConstraintDelta = FMath::Max(distanceConstraintDelta, _lastDistanceConstraintDelta);
   if (!_clientResimulateDistanceConstraint)
   {
      _distanceConstraintDesired = realDistanceConstraint;
   }

   // If we were re-simulating the distance constraint, stop now
   _clientResimulateDistanceConstraint = false;
   
   return bResult;
}

void UOSECharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
   Super::UpdateFromCompressedFlags(Flags);

   bWantsToSprint = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
   _TrySetSliding((Flags & FSavedMove_Character::FLAG_Custom_1) != 0);
   SetWantsToMantle((Flags & FSavedMove_Character::FLAG_Custom_2) != 0);
   SetWantsToScramble((Flags & FSavedMove_Character::FLAG_Custom_3) != 0);

   // Apply changes from out MoveData subclass
   // This is one of the recommended places to do it, and it is a cleaner spot than MoveAutonomous
   //   "Useful for being able to access custom movement data during internal movement functions such as MoveAutonomous()
   //    or UpdateFromCompressedFlags() to be able to maintain backwards API compatibility."
   if (const FOSECharacterNetworkMoveData* currentMove = GetCurrentOSENetworkMoveData())
   {
      _wantsToContractDistanceConstraint = currentMove->bWantsToContractDistanceConstraint;
      _extendingDistanceConstraint = currentMove->bExtendingDistanceConstraint;
      _distanceConstraintDesired.Apply(currentMove->DistanceConstraintDelta, currentMove->DistanceConstraint);
      _maxSpeedMultiplier = currentMove->MaxSpeedMultiplier;
      _extraGravityScale = currentMove->ExtraGravityScale;

      SetWantsToWallClimb(currentMove->bWantsToWallClimb);
   }
}

void UOSECharacterMovement::OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& clientData, float timeStamp, FVector newLocation, FVector newVelocity, UPrimitiveComponent* newBase, FName newBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 serverMovementMode, FVector ServerGravityDirection)
{
   Super::OnClientCorrectionReceived(clientData, timeStamp, newLocation, newVelocity, newBase, newBaseBoneName, bHasBase, bBaseRelativePosition, serverMovementMode, ServerGravityDirection);

   const FOSECharacterMoveResponseDataContainer& oseResponse = GetOSEMoveResponseDataContainer();
   const auto ackedMove = static_cast<const FOSESavedMove_Character*>(clientData.LastAckedMove.Get());
   check(ackedMove);
   if (oseResponse.bDistanceConstraintCorrection)
   {
      // correct and re-simulate distance constraint length if the server value is different, and the distance constraint is still from the same position and will not be replaced anyways
      const float kDistanceCorrectionThreshold = 0.1f;
      if (!FMath::IsNearlyEqual(oseResponse.DistanceConstraintDistance, ackedMove->DistanceConstraintEndDistance, kDistanceCorrectionThreshold) &&
         ackedMove->DistanceConstraint.Enabled &&
         oseResponse.DistanceConstraintPosition.Equals(ackedMove->DistanceConstraint.Position) &&
         oseResponse.DistanceConstraintPosition.Equals(_distanceConstraintDesired.Position) &&
         _distanceConstraintDesired.Enabled)
      {
         UE_LOG(LogOSECharacterMovement, Log, TEXT("Server disagrees with Distance constraint length!! Server: %f Client: %f Client-Begin: %f Now-Client: %f"),
            oseResponse.DistanceConstraintDistance, ackedMove->DistanceConstraintEndDistance, ackedMove->DistanceConstraint.Distance, _distanceConstraintDesired.Distance);
         _distanceConstraintDesired.ForceSlack(oseResponse.DistanceConstraintDistance);
         // Set flag so distance constraint changes will be re-simulated rather than clobbered
         _clientResimulateDistanceConstraint = true;
      }
      else if (ackedMove->bDistanceConstraintEnabled && GetDistanceConstraintEnabled() &&
         !oseResponse.DistanceConstraintPosition.Equals(ackedMove->DistanceConstraint.Position) &&
         ackedMove->DistanceConstraint.Position.Equals(_distanceConstraintDesired.Position))
      {
         UE_LOG(LogOSECharacterMovement, Log, TEXT("Correction disagrees about constraint position, sending more"));
         _lastDistanceConstraintDelta = FMath::Max(_lastDistanceConstraintDelta, EDistanceConstraintDelta::Partial);
      }
   }

   if (oseResponse.bDistanceConstraintCorrection != ackedMove->bDistanceConstraintEnabled && ackedMove->bDistanceConstraintEnabled == GetDistanceConstraintEnabled())
   {
      UE_LOG(LogOSECharacterMovement, Log, TEXT("Correction disagrees about constraint state, sending more"));
      _lastDistanceConstraintDelta = EDistanceConstraintDelta::Full;
   }
}

void UOSECharacterMovement::OnClientTimeStampResetDetected()
{
   auto serverData = static_cast<FOSENetworkPredictionData_Server_Character*>(GetPredictionData_Server());
   serverData->LastReceivedClientDistanceConstraintTimeStamp -= MinTimeBetweenTimeStampResets;
}

FNetworkPredictionData_Client* UOSECharacterMovement::GetPredictionData_Client() const
{
   if (ClientPredictionData == nullptr)
   {
      UOSECharacterMovement* MutableThis = const_cast<UOSECharacterMovement*>(this);
      MutableThis->ClientPredictionData = new FOSENetPredictionData_Client_Character(*this);
   }

   return ClientPredictionData;
}

FNetworkPredictionData_Server* UOSECharacterMovement::GetPredictionData_Server() const
{
   if (ServerPredictionData == nullptr)
   {
      UOSECharacterMovement* MutableThis = const_cast<UOSECharacterMovement*>(this);
      MutableThis->ServerPredictionData = new FOSENetworkPredictionData_Server_Character(*this);
   }

   return ServerPredictionData;
}

void UOSECharacterMovement::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (!IsFalling())
   {
      _maxFallingSpeedOverride = GetMaxSpeed();
   }
}

void UOSECharacterMovement::PostLoad()
{
   Super::PostLoad();

   OSECharOwner = Cast<AOSECharacterBase>(CharacterOwner);

   _scramblePendingResult.Init();
   _scrambleImpactResult.Init();
   _scrambleImpactElapsed = DEFAULT_TIME_SINCE_IMPACT;
   _scrambleDurationElapsed = 0.0f;
   _scrambleIsBlocked = false;
   _scrambleClearance = 0.0f;
   _scrambleForwardClearance = false;
   _authorityRVOWasEnabled = bUseRVOAvoidance;

   _wallClimbPendingResult.Init();
   _wallClimbImpactResult.Init();
}

#if WITH_EDITOR
EDataValidationResult UOSECharacterMovement::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // If we allow wall climbing, but only on some surfaces, make sure our wall climb setting overrides
   // are all materials we can actually wall-climb on, otherwise we likely have mis-configured it
   if (_wallClimbEnabled && _allowWallClimbOnlyOnSomeSurfaces)
   {
      for (const TPair<TEnumAsByte<EPhysicalSurface>, FOSEWallClimbSettings>& materialAndSettings : _surfaceWallClimbSettingOverrides)
      {
         if (!_wallClimbableSurfaces.Contains(materialAndSettings.Key))
         {
            context.AddError(FText::FromString(FString::Printf(
               TEXT("%s.%s | CMC has a wall climbing settings override for physical material '%s', but it is not in _wallClimbableSurfaces."),
               *GetNameSafe(GetOwner()),
               *GetName(),
               *UEnum::GetDisplayValueAsText(materialAndSettings.Key.GetValue()).ToString())));
         }
      }
   }

   return (context.GetNumErrors() + context.GetNumWarnings() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void UOSECharacterMovement::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
   USceneComponent* oldUpdatedComponent = UpdatedComponent;
   Super::SetUpdatedComponent(NewUpdatedComponent);
   
   OSECharOwner = Cast<AOSECharacterBase>(CharacterOwner);
   if (oldUpdatedComponent != UpdatedComponent)
   {
      _scramblePendingResult.Init();
      _scrambleImpactResult.Init();
      _scrambleImpactElapsed = DEFAULT_TIME_SINCE_IMPACT;
      _scrambleDurationElapsed = 0.0f;
      _scrambleIsBlocked = false;
      _scrambleClearance = 0.0f;
      _scrambleForwardClearance = false;

      _wallClimbPendingResult.Init();
      _wallClimbImpactResult.Init();
   }
}

// Helper function to get our custom movement type given the current movement mode
ECustomMovementType UOSECharacterMovement::GetCustomMovementType() const
{
   return OSE::MovementUtils::GetCustomMovementType(MovementMode, CustomMovementMode);
}

// Helper function to set a custom movement type
void UOSECharacterMovement::SetCustomMovementType(ECustomMovementType customMode)
{
   SetMovementMode(MOVE_Custom, (uint8)customMode);
}

void UOSECharacterMovement::NotifyBumpedPawn(APawn* bumpedPawn)
{
   Super::NotifyBumpedPawn(bumpedPawn);

   // NOTE: This is called from ::HandleImpact, which seems like a better
   // place to handle this info because it comes along with the impact hit result
}

bool UOSECharacterMovement::VerifyClientTimeStamp(float timeStamp, FNetworkPredictionData_Server_Character& serverData)
{
   bool isValid = Super::VerifyClientTimeStamp(timeStamp, serverData);

   auto& oseServerData = static_cast<FOSENetworkPredictionData_Server_Character&>(serverData);
   const FOSECharacterNetworkMoveData* currentMove = GetCurrentOSENetworkMoveData();
   check(currentMove);

   if (currentMove->DistanceConstraintDelta != EDistanceConstraintDelta::None)
   {
      if (!isValid && timeStamp > oseServerData.LastReceivedClientDistanceConstraintTimeStamp)
      {
         UE_LOG(LogOSECharacterMovement, Log, TEXT("Move to be dropped has distance constraint that is not stale, applying it anyways"));
         _distanceConstraintDesired.Apply(currentMove->DistanceConstraintDelta, currentMove->DistanceConstraint);
      }
      oseServerData.LastReceivedClientDistanceConstraintTimeStamp = timeStamp;
   }

   return isValid;
}

void UOSECharacterMovement::PossessedBy(AController* newController)
{
   check(newController);

   if (GetOwner()->HasAuthority())
   {
      if (bUseRVOAvoidance && newController->IsPlayerController())
      {
         _authorityDisabledRVOAvoidanceForPlayer = true;

         // disable RVO Avoidance on CMC when possessed by a player since it's server-only
         // which causes corrections as the player moves the character around on the client
         SetAvoidanceEnabled(false);
      }

      // cache off whether or not we're using rvo avoidance when we're possessed
      _authorityRVOWasEnabled = bUseRVOAvoidance;
   }
}

void UOSECharacterMovement::UnPossessed()
{
   if (GetOwner()->HasAuthority())
   {
      // if we previously disabled it for player possession, then disable when we unpossess
      if (_authorityDisabledRVOAvoidanceForPlayer)
      {
         SetAvoidanceEnabled(true);
         _authorityDisabledRVOAvoidanceForPlayer = false;
      }

      // cache off whether or not we're using rvo avoidance when it may change
      _authorityRVOWasEnabled = bUseRVOAvoidance;
   }
}

void UOSECharacterMovement::AuthorityOnLyingDownChanged(const bool isLyingDown)
{
   check(GetOwner()->HasAuthority());

   // disable when lying down, restore when standing back up
   // ASSUMPTION: we are assuming that lying down is essentially 1:1 with capsule disabling, which is a set of states
   // where we don't want a character to be considered for RVO against another character
   SetAvoidanceEnabled(isLyingDown ? false : _authorityRVOWasEnabled);
}

#if WITH_EDITOR
bool UOSECharacterMovement::CanEditChange(const FProperty* inProperty) const
{
   // CrouchedHalfHeight is going to be private in UE5 so we need to check this member name by FName and not do a variable lookup w/ GET_MEMBER_NAME_CHECKED
   if (inProperty && inProperty->GetFName() == FName(TEXT("CrouchedHalfHeight")))
   {
      if(const auto oseOwner = Cast<AOSECharacterBase>(GetOwner()))
      {
         return oseOwner->CanMovementComponentEditCrouchHeight();
      }
   }
   return Super::CanEditChange(inProperty);
}
#endif

void UOSECharacterMovement::PhysCustom(float deltaTime, int32 iterations)
{
   switch (GetCustomMovementType())
   {
   case ECustomMovementType::Mantle:
      if(!MovementCVars::LedgeEnableLedgeMantleCheck)
      {
         PhysCustomMantle(deltaTime, iterations);
      }
      else
      {
         PhysCustomLedge(deltaTime, iterations);
      }
      break;

   case ECustomMovementType::Scramble:
      PhysCustomScramble(deltaTime, iterations);
      break;

   case ECustomMovementType::WallClimb:
      PhysCustomWallClimb(deltaTime, iterations);
      break;

   default:
      Super::PhysCustom(deltaTime, iterations);
      break;
   }
}

// Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner.
void UOSECharacterMovement::OnMovementModeChanged(EMovementMode previousMovementMode, uint8 previousCustomMode)
{
   if (!HasValidData())
   {
      return;
   }

   // Handle transitions to/from our custom movement modes. This is often more reliable then
   // explicit start/stop methods. Explicit start/stop methods may not exist, may not be needed,
   // or may not execute in all conditions (such as replays). This is a great place to ensure
   // movement is in the expected state (for example, limiting velocity when exiting these modes).
   {
      const ECustomMovementType previousCustomMovement = OSE::MovementUtils::GetCustomMovementType(previousMovementMode, previousCustomMode);
      if (previousCustomMovement != ECustomMovementType::None)
      {
         if (previousCustomMovement == ECustomMovementType::Scramble)
         {
            // Usually climbing is blocked until we hit the ground. If it's _not_ blocked when
            // we're exiting climbing, then usually it's because we're trying to do something
            // slightly more complicated (such as wall jump). In this situation, we don't want
            // to modify velocity or any other state. This is not a very obvious use case for
            // this flag, although it is fairly reliable.
            if (_scrambleIsBlocked)
            {
               // Remove lateral velocity when climbing finishes
               Velocity.X = 0;
               Velocity.Y = 0;

               // Clamp vertical velocity to the base climb speed (this is the same calculation used while
               // climbing with no acceleration)
               const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();
               const float scrambleSpeedBase = FMath::Max(settings.MinStopSpeed, settings.ScrambleSpeed * settings.ScrambleSpeedBaseRatio);
               Velocity.Z = FMath::Min(Velocity.Z, scrambleSpeedBase);
            }
         }
      }
   }

   // Handle transitions to our custom movement modes.
   {
      const ECustomMovementType currentCustomMovement = GetCustomMovementType();
      if (currentCustomMovement != ECustomMovementType::None)
      {
         if (currentCustomMovement == ECustomMovementType::Mantle)
         {
            if (!MovementCVars::LedgeEnableLedgeMantleCheck)
            {
               const FOSEMantleState& mantleState = OSECharOwner->GetMantleState();
               if (mantleState.IsValid)
               {
                  // Simply snapping the character to the query location and start rotation; the desired offset will
                  // get applied as the montage blends in during the movement mode.
                  const FVector mantleStartDelta = mantleState.QueryLocation - UpdatedComponent->GetComponentLocation();
                  const FQuat mantleStartRotation = mantleState.StartDirection.ToOrientationQuat();

                  FHitResult hit;
                  SafeMoveUpdatedComponent(mantleStartDelta, mantleStartRotation, true, hit);
               }
            }
            else
            {
               const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();
               if (ledgeState.MountTarget.IsValid)
               {
                  // Simply snapping the character to the query location and start rotation; the desired offset will
                  // get applied as the montage blends in during the movement mode.
                  const FVector ledgeStartDelta = ledgeState.MountTarget.QueryLocation - UpdatedComponent->GetComponentLocation();
                  const FQuat ledgeStartRotation = ledgeState.MountTarget.StartDirection.ToOrientationQuat();

                  FHitResult hit;
                  SafeMoveUpdatedComponent(ledgeStartDelta, ledgeStartRotation, true, hit);
               }
            }
         }
      }
   }

   // Call base class now that we've potentially modified some state
   Super::OnMovementModeChanged(previousMovementMode, previousCustomMode);

   if (IsMovingOnGround())
   {
      // Touched the ground; allow climbing to begin
      _scrambleIsBlocked = false;
   }
   else if (IsFalling())
   {
      // The base class keeps this flag on while moving on the ground, but turns it off
      // for all other movement modes. We are turning this on for falling to ensure
      // toggling crouch won't give an artificial height change while in the air.
      bCrouchMaintainsBaseLocation = true;
   }

   //separated from the above logic because IsMovingOnGround() is not equivalent to !IsFalling()
   bool bWasFalling = (previousMovementMode == MOVE_Falling);
   if (OSECharOwner)
   {
      if (IsFalling())
      {
         if (!bWasFalling)
         {
            OSECharOwner->OnStartFalling();
         }
      }
      else
      {
         if (bWasFalling)
         {
            OSECharOwner->OnStopFalling();
         }
      }
   }

   // If we are setting our movement mode to None, then we will not actually receive further calls to UpdateCharacterStateBeforeMovement,
   // which is where our custom character state is updated. By this point, the base CMC will have cleared our velocity and reset our jump state,
   // so we should do something similar for our custom movement state code, to prevent things like sprint/climb being stuck on when our movement mode is set to none
   if (MovementMode == MOVE_None)
   {
      _UpdateCustomCharacterStateOnMovementNone();
   }

   const ECustomMovementType previousCustomMovement = OSE::MovementUtils::GetCustomMovementType(previousMovementMode, previousCustomMode);
   if (previousCustomMovement == ECustomMovementType::WallClimb && GetCustomMovementType() != ECustomMovementType::WallClimb)
   {
      StopWallClimb();
   }

   // Do this after updating our character state, since that may effect what results we get here
   UpdatePhysicalMaterialContext();
}

float UOSECharacterMovement::GetMaxAcceleration() const
{
   if (IsSliding())
   {
      // Max acceleration while sliding is zero by default
      if (CharacterOwner != nullptr)
      {
         // Acceleration away from the direction we're sliding allows us to stop the slide
         const FVector viewDirection = PawnOwner->GetActorRotation().Vector().GetSafeNormal2D();
         const FVector velocityDirection = Velocity.GetSafeNormal2D();
         return -FMath::Min(0.0f, viewDirection | velocityDirection);
      }

      return 0.0f;
   }
   else if (IsWallClimbing())
   {
      const FOSEWallClimbSettings& settings = GetCurrentWallClimbSettings();
      return _bIsWallDashing ? settings.WallClimbDashAcceleration : settings.WallClimbAcceleration;
   }

   return Super::GetMaxAcceleration();
}

float UOSECharacterMovement::GetMaxBrakingDeceleration() const
{
   if (IsMovingOnGround() && IsSliding())
   {
      // Moving on ground and sliding
      return SlidingSettings.BrakingDeceleration;
   }
   else if(IsWallClimbing())
   {
      const FOSEWallClimbSettings& settings = GetCurrentWallClimbSettings();
      return settings.WallClimbBrakingAcceleration;
   }
   return Super::GetMaxBrakingDeceleration();
}

// Slows towards stop
void UOSECharacterMovement::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
   // Defer to a helper method that's marked const and doesn't change state.
   // This is helpful for estimating friction and braking deceleration for other use-cases (eg. simulated proxies).
   _ComputeFrictionAndBrakingDeceleration(Friction, BrakingDeceleration);

   Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
}

float UOSECharacterMovement::GetBaseMaxSpeed() const
{
   // Note: explicitly calling Super of a different method
   float maxSpeed = Super::GetMaxSpeed();

   // Check for climbing state (not necessarily while falling)
   if (IsScrambling())
   {
      const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();
      maxSpeed = FMath::Max(maxSpeed, settings.ScrambleSpeed);
      maxSpeed = FMath::Max(maxSpeed, settings.MinStartSpeed);
      maxSpeed = FMath::Max(maxSpeed, settings.MinStopSpeed);
   }
   else if (IsWallClimbing())
   {
      const FOSEWallClimbSettings& settings = GetCurrentWallClimbSettings();
      maxSpeed = _bIsWallDashing ? settings.WallClimbDashSpeed : settings.WallClimbSpeed;
   }

   return maxSpeed;
}

float UOSECharacterMovement::GetMaxSpeed() const
{
   // Note: explicitly calling GetBaseMaxSpeed (which calls Super::GetMaxSpeed())
   float maxSpeed = GetBaseMaxSpeed();

   // Speed isn't modified unless we're moving on ground
   if (IsMovingOnGround())
   {
      if (MovementCVars::EnableSlopesAffectSpeed && _slopesAffectMaxSpeed && CurrentFloor.HitResult.IsValidBlockingHit())
      {
         const FVector pawnVector = Velocity;
         const FVector slopeVector = ComputeGroundMovementDelta(pawnVector, CurrentFloor.HitResult, CurrentFloor.bLineTrace).GetSafeNormal();
         maxSpeed = maxSpeed - (maxSpeed * _slopeFrictionScale * slopeVector.Z);
      }

      if (IsSliding())
      {
         // This allows our max speed to exceed the threshold needed
         // to initiate sliding in the first place.
         const float maxBoost = FMath::Max(GetSprintSpeedIncrementBase(), GetSprintSpeedIncrementForward());
         maxSpeed = GetSlidingSpeedMax() + maxBoost;
      }
      else if (IsSprinting())
      {
         const FVector localVelocity = GetLocalVelocity().GetSafeNormal2D();

         // Base running speed increase when moving forward or laterally
         {
            const float sprintCosMax = FMath::Cos(FMath::DegreesToRadians(130.0f));
            const float sprintCosMin = FMath::Cos(FMath::DegreesToRadians(135.0f));
            const float sprintScalar = FMath::SmoothStep(sprintCosMin, sprintCosMax, static_cast<float>(localVelocity.X));
            maxSpeed += GetSprintSpeedIncrementBase() * sprintScalar;
         }

         // Additional running speed increase when moving forward
         {
            const float sprintCosMax = FMath::Cos(FMath::DegreesToRadians(40.0f));
            const float sprintCosMin = FMath::Cos(FMath::DegreesToRadians(45.0f));
            const float sprintScalar = FMath::SmoothStep(sprintCosMin, sprintCosMax, static_cast<float>(localVelocity.X));
            maxSpeed += GetSprintSpeedIncrementForward() * sprintScalar;
         }
      }
   }

   // Apply the max speed multiplier
   maxSpeed *= FMath::Clamp(_maxSpeedMultiplier, 0.f, MovementCVars::MaxSpeedMultiplierCap);

   // Carry momentum from the ground into the player's jump
   if (IsFalling())
   {
      float scaledMaxWalkingSpeed = MaxWalkSpeed * FMath::Clamp(_maxSpeedMultiplier, 0.f, MovementCVars::MaxSpeedMultiplierCap);
      maxSpeed = _maxFallingSpeedOverride < scaledMaxWalkingSpeed ? scaledMaxWalkingSpeed : _maxFallingSpeedOverride;
   }

   return maxSpeed;
}

float UOSECharacterMovement::GetGravityZ() const
{
   return Super::GetGravityZ() * _extraGravityScale;
}

// Draw debug information for character movement (called with p.VisualizeMovement > 0)
float UOSECharacterMovement::VisualizeMovement() const
{
   float HeightOffset = Super::VisualizeMovement();
   if (OSECharOwner == nullptr)
      return HeightOffset;

#if (ENABLE_DRAW_DEBUG)
   
   const float OffsetPerElement = 10.0f;
   const FVector TopOfCapsule = GetActorLocation() + FVector(0.f, 0.f, CharacterOwner->GetSimpleCollisionHalfHeight());

   // Mantle
   if (CanEverMantle())
   {
      const FColor DebugColor = FColor::Purple;
      HeightOffset += OffsetPerElement;
      FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);

      FString DebugText = FString::Printf(TEXT("WantsToMantle: %d IsMantling: %d"), GetWantsToMantle(), IsMantling());
      DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, DebugColor, 0.f, true);

      const FOSEMantleState& mantleState = OSECharOwner->GetMantleState();
      if (mantleState.IsValid)
      {
         DrawDebugCapsule(GetWorld(), mantleState.StartLocation, mantleState.CapsuleExtents.Z, mantleState.CapsuleExtents.X, mantleState.StartDirection.ToOrientationQuat(), DebugColor, false, -1.0f, (uint8)'\000', 0.5f);
         DrawDebugCapsule(GetWorld(), mantleState.FinalLocation, mantleState.CapsuleExtents.Z, mantleState.CapsuleExtents.X, mantleState.FinalDirection.ToOrientationQuat(), DebugColor, false, -1.0f, (uint8)'\000', 1.0f);

         // Show the height deltas and animations
         {
            HeightOffset += OffsetPerElement;
            DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);

            const float heightResolved = mantleState.FinalLocation.Z - mantleState.StartLocation.Z;
            const float heightActual = mantleState.FinalLocation.Z - mantleState.QueryLocation.Z;
            const UAnimMontage* montageAnim = mantleState.Montage.Get();
            const FString montageText = (montageAnim != nullptr) ? montageAnim->GetName() : TEXT("none");
            const FString mantleText = FString::Printf(TEXT("[%s] Height: %0.2f (Actual: %0.2f)"), *montageText, heightResolved, heightActual);
            DrawDebugString(GetWorld(), DebugLocation, mantleText, nullptr, DebugColor, 0.f, true);
         }
      }
   }

   // Ledge
   if (CanEverUseLedges())
   {
      HeightOffset = VisualizeLedgeState(HeightOffset);
   }

   // Climb
   if (CanEverScramble())
   {
      const FColor DebugColor = IsScrambling() ? FColor::Green : FColor::Emerald;
      
      {
         HeightOffset += OffsetPerElement;
         FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);
         FString DebugText = FString::Printf(TEXT("ClimbDuration: %0.2f ClimbLastImpact: %0.2f"), _scrambleDurationElapsed, _scrambleImpactElapsed);
         DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, DebugColor, 0.f, true);
      }

      {
         HeightOffset += OffsetPerElement;
         FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);
         FString DebugText = FString::Printf(TEXT("WantsToClimb: %d IsScrambling: %d Forward: %d HeadDist: %0.2f"), GetWantsToScramble(), IsScrambling(), _scrambleForwardClearance, _scrambleClearance);
         DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, DebugColor, 0.f, true);
      }

      {
         DrawDebugSphere(GetWorld(), _scrambleImpactResult.ImpactPoint, 10, 10, FColor::Orange, false, -1.0f, (uint8)'\000', 0.5);
         DrawDebugSphere(GetWorld(), _scrambleImpactResult.Location, 10, 10, DebugColor, false, -1.0f, (uint8)'\000', 0.5);
      }
   }

   // Wall Climb
   UWorld* world = GetWorld();
   if (IsWallClimbing() && IsValid(world))
   {
      FRotator viewRotation = OSECharOwner->GetViewRotation();
      viewRotation.Pitch = 0.0f;

      FVector viewVector = viewRotation.RotateVector(FVector::ForwardVector);

      // Green: Looking towards wall (Wall Dash)
      // Red: Looking away from wall (Wall Jump)
      FColor viewLineColor = FColor::Green;
      if (!IsVectorWithinWallMovementBounds(viewVector))
      {
         viewLineColor = FColor::Red;
      }

      // Draw player view
      DrawDebugLine(world, TopOfCapsule, TopOfCapsule + (viewVector * 100.0f), viewLineColor, false);

      FRotator lookAtWallRightBoundary;
      lookAtWallRightBoundary.Yaw = GetCurrentWallClimbSettings().WallMovementAngleDegrees;

      FRotator lookAtWallLeftBoundary;
      lookAtWallLeftBoundary.Yaw = -GetCurrentWallClimbSettings().WallMovementAngleDegrees;

      // Draw look-towards-wall boundaries
      DrawDebugLine(world, TopOfCapsule, TopOfCapsule + (lookAtWallRightBoundary.RotateVector(OSECharOwner->GetActorForwardVector()) * 100.0f), FColor::Blue, false);
      DrawDebugLine(world, TopOfCapsule, TopOfCapsule + (lookAtWallLeftBoundary.RotateVector(OSECharOwner->GetActorForwardVector()) * 100.0f), FColor::Blue, false);
   }

   // Movement Material Context
   {
      const FString PhysicalMaterialName = _currentMovementMaterialContext.PhysicalMaterial.IsValid() ? _currentMovementMaterialContext.PhysicalMaterial->GetName() : "UNKNOWN";
      const FString DebugText = FString::Printf(
         TEXT("PhysMat: %s, OnGround: %d, InAir: %d, Scrambling: %d"),
         *PhysicalMaterialName,
         _currentMovementMaterialContext.IsMovingOnGround,
         _currentMovementMaterialContext.IsInAir,
         _currentMovementMaterialContext.IsScrambling);

      HeightOffset += OffsetPerElement;
      FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);
      DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, FColor::Cyan, 0.f, true);
   }

   {
      const FString DebugText = FString::Printf(
         TEXT("Mantling: %d, Crouching: %d, Sliding: %d, Sprinting: %d"),
         _currentMovementMaterialContext.IsMantling,
         _currentMovementMaterialContext.IsCrouching,
         _currentMovementMaterialContext.IsSliding,
         _currentMovementMaterialContext.IsSprinting);

      HeightOffset -= OffsetPerElement * 0.5f;
      FVector DebugLocation = TopOfCapsule + FVector(0.f, 0.f, HeightOffset);
      DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, FColor::Cyan, 0.f, true);
   }
   

#endif

   return HeightOffset;
}

// Ensure the mantle / climb state logic to determine if a mantle / climb should start
// also matches the logic in this check.
bool UOSECharacterMovement::CanAttemptJump() const
{
   if (!IsJumpAllowed())
      return false;

   // Don't allow jumping if we want to mantle, and can mantle currently
   if (!IsMantling() && GetWantsToMantle() && CanMantleInCurrentState())
   {
      return false;
   }

   // Don't allow jumping if we want to climb, and can climb currently
   if (!IsScrambling() && GetWantsToScramble() && CanScrambleInCurrentState())
   {
      return false;
   }

   // Explicit check for jumping while scrambling. We can't fall back to the base class
   // validation, as the default checks for movement mode won't allow it.
   if (IsScrambling() && CanScrambleJumpInCurrentState())
   {
      return true;
   }

   // Explicit check for jumping while wall climbing. We can't fall back to the base class
   // validation, as the default checks for movement mode won't allow it.
   if (IsWallClimbing())
   {
      return true;
   }

   // Check if we allow jumping if we're crouched. The character still validates this
   // and has to explicitly support it in ACharacter::CanJumpInternal_Implementation().
   if (bWantsToCrouch && !GetCrouchJumpSettings().AllowJumpWhileCrouched)
   {
      return false;
   }
   
   // Falling included for double-jump and non-zero jump hold time, but validated by character
   if (!(IsMovingOnGround() || IsFalling()))
   {
      return false;
   }

   // We can jump
   return true;
}

bool UOSECharacterMovement::DoJump(bool bReplayingMoves, float deltaTime)
{
   if (CharacterOwner && CharacterOwner->CanJump())
   {
      // Initialize jump settings to the default values
      FOSEJumpSettings jumpSettings;
      jumpSettings.InitialPhysics.Velocity = FVector(0, 0, JumpZVelocity);
      jumpSettings.HoldPhysics.Velocity = FVector(0, 0, JumpZVelocity);

      // This represents the transition from not (actively) jumping to jumping
      const bool isInitialJump = (!CharacterOwner->bWasJumping);
      
      if (isInitialJump && bWantsToCrouch)
      {
         const FOSECrouchJumpSettings& crouchJump = GetCrouchJumpSettings();
         if (crouchJump.AllowJumpWhileCrouched && crouchJump.UncrouchWhenJumping)
         {
            // No longer want to crouch if we're attempting to jump
            bWantsToCrouch = false;
         }
      }

      // Now using the jump count _before_ CheckJumpInput() modifies it. This is a
      // more accurate representation of the jump index, and fixes the crash when
      // there is a server correction while in the air. This is a new member that
      // was added by Epic in 4.26 CL# 13959855 for Jira UE-95056
      const int32 jumpIdx = CharacterOwner->JumpCurrentCountPreJump;

      // Check if we specified alternate jump settings
      if (_jumpSettings.Num() > 0)
      {
         jumpSettings = _jumpSettings[FMath::Min(jumpIdx, _jumpSettings.Num() - 1)];

         // Check if the hold time is over our limit. If it is, clear our jump values
         const float maxHoldTime = FMath::Min(jumpSettings.MaxHoldTime, CharacterOwner->GetJumpMaxHoldTime());
         if ((maxHoldTime > 0.0f) && (CharacterOwner->JumpKeyHoldTime > maxHoldTime))
         {
            jumpSettings = FOSEJumpSettings();
         }
      }

      // Assume we'll use the hold values unless we just started jumping
      FOSEJumpPhysics jumpPhysics = isInitialJump ? jumpSettings.InitialPhysics : jumpSettings.HoldPhysics;

      if (jumpSettings.DeferHoldUntilApex)
      {
         if (isInitialJump)
         {
            // Wait for the apex
            bNotifyApex = true;
         }
         else
         {
            if (bNotifyApex)
            {
               // Zero out the acceleration and impulse unless we're falling
               // after reaching the jump apex. Applying these values during the
               // active jump itself may result in unexpected behavior.
               jumpPhysics.Acceleration = FVector::ZeroVector;
               jumpPhysics.Impulse = FVector::ZeroVector;
            }
         }
      }

      // If we're climbing and we can jump from a climb currently,
      // it's safe to assume this is a climb jump.
      if (IsScrambling() && CanScrambleJumpInCurrentState())
      {
         jumpPhysics = GetCurrentScrambleSettings().JumpPhysics;

         // If we applied climb jump physics, then clear the blocked flag.
         // This allows us to climb again when we reach another surface.
         // We have to set the flag prior to applying the jump, since the
         // flag may be checked on movement mode transitions.
         const bool prevClimbIsBlocked = _scrambleIsBlocked;
         _scrambleIsBlocked = false;

         if (!DoJumpPhysics(jumpPhysics))
         {
            // Restore the previous state since the jump failed
            _scrambleIsBlocked = prevClimbIsBlocked;
            return false;
         }

         OSECharOwner->OnScrambleJump(_scrambleImpactResult);

         // Jump was successfully applied
         return true;
      }

      if (IsWallClimbing())
      {
         if (!IsVectorWithinWallMovementBounds(OSECharOwner->GetViewRotation().RotateVector(FVector::ForwardVector)))
         {
            jumpPhysics = GetCurrentWallClimbSettings().JumpPhysics;

            if (!DoJumpPhysics(jumpPhysics, true))
            {
               return false;
            }

            // Jump was successfully applied
            return true;
         }
         else
         {
            FVector dashDirection = Acceleration;
            if (dashDirection.IsNearlyZero())
            {
               dashDirection = OSECharOwner->GetActorUpVector();
            }

            InitiateWallClimbDash(dashDirection.GetSafeNormal());
            return false;
         }
      }

      // Now actually apply the values
      DoJumpPhysics(jumpPhysics);
      return true;
   }

   return Super::DoJump(bReplayingMoves, deltaTime);
}

void UOSECharacterMovement::InitiateWallClimbDash(FVector DirectionNormal, bool bAutoWallDash)
{
   if (!_bIsWallDashing)
   {
      UWorld* world = GetWorld();
      if (IsValid(world) && IsValid(UpdatedComponent) && IsValid(OSECharOwner))
      {
         _bIsWallDashing = true;
         bForceMaxAccel = true;

         OSECharOwner->OnStartWallDash(bAutoWallDash);

         // Convert accel into local space before setting our wall dash direction
         _wallClimbDashLocalAccelNormal = UpdatedComponent->GetComponentTransform().InverseTransformVectorNoScale(DirectionNormal);
         world->GetTimerManager().SetTimer(_wallDashTimer, this, &UOSECharacterMovement::WallClimbDashFinished, GetCurrentWallClimbSettings().WallClimbDashDuration, false);
      }
   }
}

bool UOSECharacterMovement::DoJumpPhysics(const FOSEJumpPhysics& jumpPhysics, bool bUseViewRotation)
{
   bool appliedJump = false;

   if (CharacterOwner && CharacterOwner->CanJump())
   {
      // Rotate the local actor-space vectors to world space
      FRotator actorRotation = CharacterOwner->GetActorRotation();

      if (bUseViewRotation)
      {
         actorRotation = CharacterOwner->GetViewRotation();
         actorRotation.Pitch = 0.0f;
      }

      const FVector initialVelocity = Velocity;
      const FVector worldVelocity = actorRotation.RotateVector(jumpPhysics.Velocity);
      const FVector worldImpulse = actorRotation.RotateVector(jumpPhysics.Impulse);
      const FVector worldAccel = actorRotation.RotateVector(jumpPhysics.Acceleration);

      // Set the velocity X
      if (!FMath::IsNearlyZero(worldVelocity.X, KINDA_SMALL_NUMBER))
      {
         Velocity.X = worldVelocity.X;
         appliedJump = true;
      }

      // Set the velocity Y
      if (!FMath::IsNearlyZero(worldVelocity.Y, KINDA_SMALL_NUMBER))
      {
         Velocity.Y = worldVelocity.Y;
         appliedJump = true;
      }

      // Set the velocity Z
      if (!FMath::IsNearlyZero(worldVelocity.Z, KINDA_SMALL_NUMBER))
      {
         if (worldVelocity.Z > 0.0f)
         {
            Velocity.Z = FMath::Max(Velocity.Z, worldVelocity.Z);
         }
         else
         {
            Velocity.Z = FMath::Min(Velocity.Z, worldVelocity.Z);
         }

         appliedJump = true;
      }

      // Check for floor clearance when jumping
      if (MovementCVars::JumpEnableClearanceCheck && appliedJump && (IsMovingOnGround() && CurrentFloor.IsWalkableFloor()))
      {
         // The change in velocity will need to be re-applied
         const FVector deltaVelocity = Velocity - initialVelocity;

         // Project the initial velocity (most likely 2D) on the floor so we keep the total speed
         // and also have an accurate Z velocity to apply the extra jump velocity
         const FVector floorNormal = CurrentFloor.HitResult.ImpactNormal;
         const FVector floorVelocity = FVector::VectorPlaneProject(initialVelocity, floorNormal);

         // By default, only project velocity when going up, as it reduced the expected horizontal and vertical speed
         const bool projectVelocity = (MovementCVars::JumpAlwaysProjectVelocity || floorVelocity.Z >= KINDA_SMALL_NUMBER);
         Velocity = deltaVelocity + (projectVelocity ? floorVelocity : initialVelocity);
      }

      // Apply impulse
      if (!worldImpulse.IsNearlyZero(KINDA_SMALL_NUMBER))
      {
         // AddImpulse divides by mass, we want actual velocity
         AddImpulse(worldImpulse * Mass);
         appliedJump = true;
      }

      // Apply acceleration
      if (!worldAccel.IsNearlyZero(KINDA_SMALL_NUMBER))
      {
         // AddForce divides by mass, we want actual acceleration
         AddForce(worldAccel * Mass);
         appliedJump = true;
      }
   }

   if (appliedJump)
   {
      SetMovementMode(MOVE_Falling);
   }

   return appliedJump;
}

// Returns the velocity vector rotated to local space
FVector UOSECharacterMovement::GetLocalVelocity() const
{
   if (HasValidData())
   {
      return PawnOwner->GetActorRotation().UnrotateVector(Velocity);
   }

   return FVector::ZeroVector;
}

// Update the character state in PerformMovement right before doing the actual position change
void UOSECharacterMovement::UpdateCharacterStateBeforeMovement(float deltaSeconds)
{
   // reset last distance constraint delta
   _lastDistanceConstraintDelta = EDistanceConstraintDelta::None;
   
   // cached off for UpdateCharacterStateAfterMovement()
   _wasCrouchingBeforeMovementUpdate = IsCrouching();

   _timeSinceLastImpact += deltaSeconds;

   if(!MovementCVars::LedgeEnableLedgeMantleCheck)
   {
      // Mantling can be affected by jumping, which occurs before this call
      UpdateMantleState();
   }
   else
   {
      // Mantling can be affected by jumping, which occurs before this call
      UpdateLedgeState();
   }

   // Sprinting could change our desire to crouch as well
   UpdateSprintState();

   UpdateWallClimbState(deltaSeconds);

   // Modified version of crouch check before checking for slide. This updates the crouch state,
   // which is then used to determine if we can slide or not.
   // NOTE: No longer calling base class, so keep an eye on if it does anything other than crouch
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      // Check for a change in crouch state. Players toggle crouch by changing bWantsToCrouch.
      const bool isCrouching = IsCrouching();

      // Modified to stay crouching during a crouched mantle, so a player can't do weird stuff by un-crouching midway.
      // If this logic gets more complex, then extract predicate to function
      const bool wantsToCrouch = bWantsToCrouch || (IsMantling() && OSECharOwner->GetMantleState().IsCrouching);
      if (isCrouching && (!wantsToCrouch || !CanCrouchInCurrentState()))
      {
         UnCrouch(false);
      }
      else if (!isCrouching && wantsToCrouch && CanCrouchInCurrentState())
      {
         Crouch(false);
      }
   }

   UpdateSlideState();
}

// Update the character state in PerformMovement after the position change. Some rotation updates happen after this.
void UOSECharacterMovement::UpdateCharacterStateAfterMovement(float deltaSeconds)
{
   // If movement got us into a position where we can't crouch anymore, this will
   // be reflected in the base class call. It's a good time for us to update our slide
   // state as well, since it heavily depends on crouching.
   Super::UpdateCharacterStateAfterMovement(deltaSeconds);

   UpdateSlideState();

   UpdateScrambleState(deltaSeconds);

   // Check for a different physical material from last sent one
   UpdatePhysicalMaterialContext();

   // Update turn-in-place
   UpdateTurnInPlace(deltaSeconds);

   // bp's cant tell from the engine "OnCrouch" events if we're crouching or sliding, this event gives them a hook
   // where they can reason about crouch/slide after slide has fully resolved
   const bool isCrouching = IsCrouching();
   if (_wasCrouchingBeforeMovementUpdate != isCrouching)
   {
      if (OSECharOwner)
      {
         OSECharOwner->OnCrouchingStateChanged(isCrouching);
      }
   }
}

//---------------------------------------------------------------------------------------
// Vaulting
//---------------------------------------------------------------------------------------
bool UOSECharacterMovement::FindVaultLedges(FVector& FrontLedge, FVector& FrontLedgeNormal, FVector& BackLedge, FVector& BackLedgeNormal)
{
   UWorld* world = GetWorld();
   if (!IsValid(world) || !IsValid(OSECharOwner))
   {
      return false;
   }

   // Start downward traces from the character's height
   const FVector headLocation = GetActorLocation() + FVector(0.f, 0.f, CharacterOwner->GetSimpleCollisionHalfHeight());

   float distBetweenDownwardTraces = VaultForwardDistance / VaultDownwardTraceResolution;
   const FVector forwardVector = OSECharOwner->GetActorForwardVector() * distBetweenDownwardTraces;

   FVector startLocation = headLocation;
   TArray<FHitResult> downwardHits;

   for (int i = 0; i < VaultDownwardTraceResolution; ++i)
   {
      // End the traces a little past the character's feet
      FVector endLocation = startLocation + FVector(0.f, 0.f, -(CharacterOwner->GetSimpleCollisionHalfHeight() * 2) - 10.0f);

      FHitResult outHit;
      FCollisionQueryParams queryParams;
      queryParams.AddIgnoredActor(OSECharOwner);
      world->LineTraceSingleByChannel(outHit, startLocation, endLocation, ECC_Visibility, queryParams);

      //Bank the trace and set the new start location for the next downward trace
      downwardHits.Add(outHit);

      if (MovementCVars::DebugLedgeDetection)
      {
         DrawDebugLine(world, startLocation, endLocation, FColor::Green, false, 5.0f);
      }

      startLocation = startLocation + forwardVector;
   }

   // Lets now process our hits and look for a low, high, low height (Z) pattern
   bool bFoundVaultableWall = false;
   int wallStartDownwardHitsIdx = 0;
   int wallEndDownwardHitsIdx = 0;
   float wallHeight = 0.0f;
   for (int i = 0; i < downwardHits.Num() - 2; ++i)
   {
      // If there's a spike in height let's check the next few hits to see if it decreases again
      if (downwardHits[i].bBlockingHit && downwardHits[i + 1].bBlockingHit
         && downwardHits[i].ImpactPoint.Z < (downwardHits[i + 1].ImpactPoint.Z - VaultFrontLedgeHeightMin))
      {
         float currentHeight = downwardHits[i + 1].ImpactPoint.Z;
         for (int j = i + 2; j < downwardHits.Num(); ++j)
         {
            // If the height goes up again then we have not found the plateau-like shape we're looking for
            if (!downwardHits[j].bBlockingHit || (downwardHits[j].ImpactPoint.Z > currentHeight))
            {
               break;
            }
            // If the height stays the same we can keep looking for our height decrease (we don't mind walls with a lot of depth)
            else if (downwardHits[j].ImpactPoint.Z == currentHeight)
            {
               continue;
            }
            else
            {
               //One more line trace to ensure we can see over wall
               FVector startLoc = downwardHits[i].ImpactPoint;
               startLoc.Z = currentHeight + 10.0f;

               FVector endLoc = downwardHits[j].ImpactPoint;
               endLoc.Z = startLoc.Z;

               // One final line trace just above the wall
               //TODO: Will probably remove this since the blueprint for vaulting will already check for clearance via a capsule sweep
               FHitResult outForwardHit;
               FCollisionQueryParams queryParams;
               queryParams.AddIgnoredActor(OSECharOwner);
               world->LineTraceSingleByChannel(outForwardHit, startLoc, endLoc, ECC_Visibility, queryParams);

               if (MovementCVars::DebugLedgeDetection)
               {
                  DrawDebugLine(GetWorld(), startLoc, endLoc, FColor::Magenta, false, 5.0f);
               }

               // If the height drop is far enough to satisfy our minimum back ledge height then we've found a potentially valid vault wall
               if ((downwardHits[j].ImpactPoint.Z < (currentHeight - VaultBackLedgeHeightMin)) && !outForwardHit.bBlockingHit)
               {
                  bFoundVaultableWall = true;
                  wallStartDownwardHitsIdx = i;
                  wallEndDownwardHitsIdx = j;
                  wallHeight = currentHeight;
               }

               break;
            }
         }
      }

      // Don't keep looking if we found our wall
      if (bFoundVaultableWall)
      {
         break;
      }
   }

   if (bFoundVaultableWall)
   {
      // We find the front ledge by tracing horizontally towards our first downward hit on the top of the wall
      FVector frontLedgeTraceStart = downwardHits[wallStartDownwardHitsIdx].ImpactPoint;
      frontLedgeTraceStart.Z = wallHeight;

      FVector frontLedgeTraceEnd = frontLedgeTraceStart + forwardVector;

      FHitResult outFrontHit;
      FCollisionQueryParams queryParams;
      queryParams.AddIgnoredActor(OSECharOwner);
      world->LineTraceSingleByChannel(outFrontHit, frontLedgeTraceStart, frontLedgeTraceEnd, ECC_Visibility, queryParams);

      if (!outFrontHit.bBlockingHit)
      {
         // Something went wrong if we cant find a hit in the same spot we found one on our downward trace
         return false;
      }
      else
      {
         _frontVaultLedge = outFrontHit.ImpactPoint;
         _frontVaultLedgeNormal = outFrontHit.ImpactNormal;

         if (MovementCVars::DebugLedgeDetection)
         {
            DrawDebugDirectionalArrow(GetWorld(), frontLedgeTraceStart, frontLedgeTraceEnd, 10.0f, FColor::Blue, false, 5.0f);
            DrawDebugSphere(world, outFrontHit.ImpactPoint, 10.0f, 8, FColor::Blue, false, 5.0f);
         }
      }

      // We find the back ledge by tracing horizontally towards our last downward hit on the top of the wall
      FVector BackLedgeTraceStart = downwardHits[wallEndDownwardHitsIdx].ImpactPoint;
      BackLedgeTraceStart.Z = wallHeight;

      FVector BackLedgeTraceEnd = BackLedgeTraceStart - forwardVector;

      FHitResult outBackHit;
      world->LineTraceSingleByChannel(outBackHit, BackLedgeTraceStart, BackLedgeTraceEnd, ECC_Visibility, queryParams);

      if (!outBackHit.bBlockingHit)
      {
         // Something went wrong if we cant find a hit in the same spot we found one on our downward trace
         return false;
      }
      else
      {
         _backVaultLedge = outBackHit.ImpactPoint;
         _backVaultLedgeNormal = outBackHit.ImpactNormal;

         if (MovementCVars::DebugLedgeDetection)
         {
            DrawDebugDirectionalArrow(GetWorld(), BackLedgeTraceStart, BackLedgeTraceEnd, 10.0f, FColor::Red, false, 5.0f);
            DrawDebugSphere(world, outBackHit.ImpactPoint, 10.0f, 8, FColor::Red, false, 5.0f);
         }
      }

      FrontLedge = _frontVaultLedge;
      FrontLedgeNormal = _frontVaultLedgeNormal;
      BackLedge = _backVaultLedge;
      BackLedgeNormal = _backVaultLedgeNormal;
      return true;
   }

   return false;
}

//---------------------------------------------------------------------------------------
// Sliding
//---------------------------------------------------------------------------------------

float UOSECharacterMovement::GetSlidingSpeedMin() const
{
   // Always implemented in terms of crouch walking, which is what we transition to
   // when we stop sliding while crouched.
   return MaxWalkSpeedCrouched;
}

float UOSECharacterMovement::GetSlidingSpeedMax() const
{
   // Always implemented in terms of sprinting (moving faster than walking).
   // The sprint increments default to the same boost value. They are averaged here
   // since it is possible for them to be set to different values, and we want to
   // make sure all configuration works as expected.
   const float averageSpeedInc = (SprintSpeedIncrementBase + SprintSpeedIncrementForward) * 0.5f;
   return MaxWalkSpeed + averageSpeedInc;
}

void UOSECharacterMovement::_TrySetSliding(bool sliding)
{
   if (sliding == CanSlideInCurrentState())
   {
      _SetSliding(sliding);
   }
}

void UOSECharacterMovement::_SetSliding(bool sliding)
{
   if (bIsSliding != sliding)
   {
      bIsSliding = sliding;

      if (OSECharOwner)
      {
         if (bIsSliding)
         {
            OSECharOwner->OnStartSliding();
         }
         else
         {
            OSECharOwner->OnStopSliding();
         }
      }
   }
}

// Returns true if the character is currently sliding
bool UOSECharacterMovement::IsSliding() const
{
   return bIsSliding;
}

// Returns true if the character can ever slide
bool UOSECharacterMovement::CanEverSlide() const
{
   return CanEverCrouch() && SlidingEnabled;
}

// Returns true if the character is allowed to slide in the current state
bool UOSECharacterMovement::CanSlideInCurrentState() const
{
   if (CharacterOwner == nullptr)
      return false;

   // Must be able to slide
   if (!CanEverSlide())
      return false;

   // Owner must be able to slide
   if (!OSECharOwner->CanSlide())
      return false;

   // Must be able to crouch, and currently crouching
   if (!CanCrouchInCurrentState() || !IsCrouching())
      return false;

   // Must be moving on the ground, or falling while sliding
   if (!IsMovingOnGround() && !(IsFalling() && IsSliding()))
      return false;

   // We need to be above a certain speed to keep sliding.
   // We need to be above a higher speed to start sliding.
   const float SlidingSpeedMin = GetSlidingSpeedMin();
   const float SlidingSpeedMax = GetSlidingSpeedMax();
   const float SlideSpeedThreshold = (IsSliding() ? SlidingSpeedMin : SlidingSpeedMax);
   
   // Forward and lateral local velocity
   const FVector LocalVelocity = GetLocalVelocity();
   const float ForwardSpeed = LocalVelocity.X;
   const float LateralSpeed = (IsSliding() ? 0.0f : FMath::Abs(LocalVelocity.Y));

   return ((ForwardSpeed > SlideSpeedThreshold) && (ForwardSpeed > LateralSpeed));
}

// Checks for slide conditions, and handles the transition to and from sliding state
void UOSECharacterMovement::UpdateSlideState()
{
   if (IsSliding() && !CanSlideInCurrentState())
   {
      // Stop sliding
      _SetSliding(false);
   }
   else if (!IsSliding() && CanSlideInCurrentState())
   {
      // Start sliding
      _SetSliding(true);
   }
}


//---------------------------------------------------------------------------------------
// Scrambling
//---------------------------------------------------------------------------------------

// Enforce constraints on input given current state. For instance, don't move upwards if walking and looking up.
FVector UOSECharacterMovement::ConstrainInputAcceleration(const FVector& inputAcceleration) const
{
   FVector result = Super::ConstrainInputAcceleration(inputAcceleration);

   if (IsScrambling())
   {
      // Always remove vertical acceleration
      result.Z = 0.0f;

      if (MovementCVars::ClimbRemoveLateralInput && (PawnOwner != nullptr))
      {
         // Remove lateral input acceleration
         const FRotator rotation = PawnOwner->GetActorRotation();
         FVector localInput = rotation.UnrotateVector(result.GetSafeNormal());
         localInput.Y = 0.0f;
         localInput.Z = 0.0f;
         result = rotation.RotateVector(localInput.GetSafeNormal());
      }
   }

   return result;
}

// Handle a blocking impact. Calls ApplyImpactPhysicsForces for the hit, if bEnablePhysicsInteraction is true.
void UOSECharacterMovement::HandleImpact(const FHitResult& impact, float timeSlice /* = 0.0f */, const FVector& moveDelta /* = FVector::ZeroVector */)
{
   // Queue the most recent valid blocking hit for later consideration for climbing.
   // This ensures that multiple hits per tick will use the most recent move impact.
   if (impact.IsValidBlockingHit())
   {
      _scramblePendingResult = impact;
      _wallClimbPendingResult = impact;
      _timeSinceLastImpact = 0;
   }
   else if (!_scramblePendingResult.IsValidBlockingHit())
   {
      // Existing impact is not a valid blocking hit.
      // Replace it if the more recent impact is blocking.
      if (impact.bBlockingHit)
      {
         _scramblePendingResult = impact;
      }
      else if (!_scramblePendingResult.bBlockingHit)
      {
         // Existing impact is not a blocking hit.
         // Replace it with the more recent impact.
         _scramblePendingResult = impact;
      }
   }

   AOSECharacterBase* bumpedOSECharacter = Cast<AOSECharacterBase>(impact.GetActor());
   if (bumpedOSECharacter)
   {
      // tell our owner who we bumped into
      OSECharOwner->OnBumpedInto(bumpedOSECharacter, impact);

      // tell the other character we bumped into them
      bumpedOSECharacter->OnBumpedBy(OSECharOwner, impact);
   }

   Super::HandleImpact(impact, timeSlice, moveDelta);
}

bool UOSECharacterMovement::IsScrambling() const
{
   return OSECharOwner && OSECharOwner->IsScrambling();
}

bool UOSECharacterMovement::CanEverScramble() const
{
   return GetScrambleEnabled();
}

bool UOSECharacterMovement::CanEverScrambleJump() const
{
   if (!CanEverScramble())
      return false;

   // Slightly iff-ier, if surface dependent, but fine as used in practice
   if (!GetCurrentScrambleSettings().JumpEnabled)
      return false;

   return true;
}

bool UOSECharacterMovement::CanScrambleInCurrentState() const
{
   if (CharacterOwner == nullptr)
      return false;

   // Must be able to climb
   if (!CanEverScramble())
      return false;

   // Owner must be able to climb
   if (!OSECharOwner->CanScramble())
      return false;

   // Can't be sliding, mantling, or crouching
   if (IsSliding() || IsMantling() || IsCrouching())
      return false;

   const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();

   // Require recent valid result
   if (_scrambleImpactElapsed >= settings.ExpirationDelay)
      return false;

   // Determine if we're moving into or away from the wall.
   // Note this is a more restrictive, 2D angle check than the one in IsValidClimbHitResult().
   // This is intentional as this angle is only used when initiating a wall climb.
   // Using hit Normal over ImpactNormal, as ImpactNormal sometimes gave garbage values with mesh colliders
   const float radianFaceAngle = FMath::DegreesToRadians(settings.MaxAngleFacing);
   const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
   const float cosineWallAngle = (Acceleration.GetSafeNormal2D() | (-_scrambleImpactResult.Normal).GetSafeNormal2D());
   const bool isMovingAway = (cosineWallAngle <= -cosineFaceAngle);
   const bool isMovingInto = (cosineWallAngle >= +cosineFaceAngle) && !isMovingAway;

   if (!IsScrambling())
   {
      // We're not climbing yet. Make sure we're accelerating into the impact
      if (!isMovingInto)
         return false;

      // Make sure we're not blocked from starting climbing
      if (_scrambleIsBlocked)
         return false;

      // Make sure we have sufficient clearance
      if ((_scrambleClearance < settings.MinCeilingClearance) || (!_scrambleForwardClearance))
         return false;
   }
   else
   {
      // We cancel the climb if we're accelerating away
      const bool isClimbCancelled = (isMovingAway && MovementCVars::ClimbCancelWhenMovingAway);

      // Scrambling can end from external changes to movement mode
      const bool isClimbEnded = GetCustomMovementType() != ECustomMovementType::Scramble;

      if (isClimbCancelled || isClimbEnded)
         return false;
   }

   // Ensure we're at the correct vertical speed
   const float speedThreshold = IsScrambling() ? settings.MinStopSpeed : settings.MinStartSpeed;
   if (Velocity.Z < speedThreshold)
   {
      return false;
   }

   return true;
}

// Returns true if the character is allowed to jump from a climb in the current state
bool UOSECharacterMovement::CanScrambleJumpInCurrentState() const
{
   if (CharacterOwner == nullptr)
      return false;

   // Must be able to jump at all
   if (!IsJumpAllowed())
      return false;

   // Must be able to jump from a climb
   if (!CanEverScrambleJump())
      return false;

   // Must currently be climbing
   if (!IsScrambling())
      return false;

   // Ensure we're at the correct vertical speed
   const float speedThreshold = 0.0f;
   if (Velocity.Z < speedThreshold)
   {
      return false;
   }

   return true;
}

bool UOSECharacterMovement::IsValidScrambleHitResult(const FHitResult& impact) const
{
   if (!IsValidScrambleHitResult(impact, _GetScrambleSettings(impact), CharacterOwner))
      return false;

   // Only climb on components that can be stepped up on.
   // See UCharacterMovementComponent::CanStepUp
   {
      // No component for "fake" hits when we are on a known good base
      if (const UPrimitiveComponent* hitComponent = impact.Component.Get())
      {
         if (!hitComponent->CanCharacterStepUp(GetOwner<APawn>()))
         {
            return false;
         }
      }

      // No actor for "fake" hits when we are on a known good base
      if (const AActor* hitActor = impact.GetActor())
      {
         if (!hitActor->CanBeBaseForCharacter(GetOwner<APawn>()))
         {
            return false;
         }
      }
   }

   return true;
}

// Static utility version to ensure we only use explicit state
bool UOSECharacterMovement::IsValidScrambleHitResult(const FHitResult& impact, const FOSEScrambleSettings& climbSettings, const ACharacter* characterOwner)
{
   if (characterOwner == nullptr)
      return false;

   if (!impact.bBlockingHit)
      return false;

   // Ensure the impact is on our cylinder base
   if (MovementCVars::ClimbCylinderCheck)
   {
      const float capsuleHalfHeight = MovementCVars::ClimbCylinderBuffer + characterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere();
      const FVector impactOffset = impact.ImpactPoint - impact.Location;
      const bool impactAbove = ((impactOffset.Z >= -capsuleHalfHeight) && (impactOffset.Z <= capsuleHalfHeight));
      if (!impactAbove)
         return false;
   }

   // Ensure we're facing the impact
   {
      // Note that this is a less restrictive angle check than the one used to initiate climbing.
      // Initiating a climb uses this hit result if it is valid to apply the more restrictive check.
      const FVector pawnVector = characterOwner->GetActorRotation().Vector();
      const float radianFaceAngle = 2.0f * FMath::DegreesToRadians(climbSettings.MaxAngleFacing);
      const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
      const float cosineWallAngle = (pawnVector | -impact.Normal);
      if (cosineWallAngle < cosineFaceAngle)
         return false;
   }

   return true;
}

void UOSECharacterMovement::UpdateScrambleState(float deltaSeconds)
{
   if (CanEverScramble())
   {
      // This is constantly incremented until it's reset by finding a valid impact
      _scrambleImpactElapsed += deltaSeconds;

      // Update our pending hits, if any
      {
         const FHitResult impact = _scramblePendingResult;
         _scramblePendingResult.Init();

         if (IsValidScrambleHitResult(impact))
         {
            // Could climb this
            _scrambleImpactResult = impact;
            _scrambleImpactElapsed = 0;

            if (IsScrambling())
            {
               _cachedMovementMaterialHitResult = _scrambleImpactResult;
            }
         }
      }


      const FOSEScrambleSettings& settings = _GetScrambleSettings(_scrambleImpactResult);

      // Update our clearance height
      {
         const float radiusShrinkAmt = MovementCVars::ClimbRadiusBuffer;
         const float ceilingMaxHeight = settings.MinCeilingClearance + MovementCVars::ClimbCylinderBuffer;
         _scrambleClearance = CheckCeilingClearance(ceilingMaxHeight, radiusShrinkAmt);
      }

      // Check forward clearance
      {
         _scrambleForwardClearance = false;

         // Only check when we aren't climbing
         const bool fwdCastEnabled = MovementCVars::ClimbForwardCastEnabled > 0;
         if (fwdCastEnabled && !IsScrambling())
         {
            // Require recent valid result
            if (_scrambleImpactElapsed < settings.ExpirationDelay)
            {
               const float radiusShrinkAmt = MovementCVars::ClimbRadiusBuffer;
               const float forwardDistance = MovementCVars::ClimbForwardCastDistance;
               _scrambleForwardClearance = CheckForwardScrambleClearance(forwardDistance, radiusShrinkAmt);
            }
         }
         else
         {
            // Forward cast disabled, or we were already climbing
            _scrambleForwardClearance = true;
         }
      }
   }

   // Proxies get replicated state
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      if (!IsScrambling() && GetWantsToScramble() && CanScrambleInCurrentState())
      {
         StartScrambling(false);
      }
      else if (IsScrambling() && !CanScrambleInCurrentState())
      {
         StopScrambling(false);
      }
   }
}

const FOSEScrambleSettings& UOSECharacterMovement::_GetScrambleSettings(const FHitResult& impact) const
{
   if (UPhysicalMaterial* physMat = impact.PhysMaterial.Get())
   {
      if (const FOSEScrambleSettings* surfaceOverride = _surfaceScrambleSettingOverrides.Find(physMat->SurfaceType))
      {
         return *surfaceOverride;
      }
   }

   return _scrambleSettings;
}

void UOSECharacterMovement::StartScrambling(bool inClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   if (!inClientSimulation)
   {
      OSECharOwner->SetIsScrambling(true);
   }

   // Reset state
   _scrambleDurationElapsed = 0;
   _scrambleIsBlocked = true;

   // Rotate the character to face the wall
   const FVector wallDirection = (-_scrambleImpactResult.Normal).GetSafeNormal2D();
   const FRotator wallRotation = wallDirection.ToOrientationRotator();

   // Rotate the actor
   {
      FHitResult hit;
      SafeMoveUpdatedComponent(FVector::ZeroVector, wallRotation, true, hit);
   }

   // Apply a boost to velocity when we start climbing
   {
      const float doubleGravity = FMath::Abs(2.0f * GetGravityZ());
      if (doubleGravity > KINDA_SMALL_NUMBER)
      {
         const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();
         const float approxHeight = FMath::Max(0.0f, settings.StartBoostHeight);
         const float approxSpeed = FMath::Sqrt(approxHeight * doubleGravity);

         // Our speed must exceed our stopping speed, or this will be a short climb
         const float finalSpeed = FMath::Max(approxSpeed, settings.MinStopSpeed);
         Velocity.Z = FMath::Max(Velocity.Z, finalSpeed);
      }
   }

   // Require the request to be submitted again. This makes it much simpler to jump from
   // a climb, and simpler to start climbing again.
   // @TODO: Make this an option (or cvar)
   SetWantsToScramble(false);

   SetCustomMovementType(ECustomMovementType::Scramble);

   OSECharOwner->OnStartScrambling(_scrambleImpactResult);
}

void UOSECharacterMovement::StopScrambling(bool inClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   if (!inClientSimulation)
   {
      OSECharOwner->SetIsScrambling(false);
   }

   // Reset state; this ensures we don't re-trigger climbing
   const FHitResult lastClimbImpact = _scrambleImpactResult;
   _scrambleImpactElapsed = DEFAULT_TIME_SINCE_IMPACT;
   _scrambleClearance = 0;
   _scrambleForwardClearance = false;

   // Only change movement mode if we're still in climb mode. External logic could
   // have explicitly set a new movement mode which would trigger the StopClimbing(),
   // and we don't want to mess with what they set in that situation.
   if (GetCustomMovementType() == ECustomMovementType::Scramble)
   {
      SetMovementMode(MOVE_Falling);
   }

   OSECharOwner->OnStopScrambling(lastClimbImpact);
}

// Custom movement mode for climbing
void UOSECharacterMovement::PhysCustomScramble(float deltaTime, int32 iterations)
{
   if (deltaTime < MIN_TICK_TIME)
   {
      return;
   }

   float remainingTime = deltaTime;
   while ((remainingTime >= MIN_TICK_TIME) && (iterations < MaxSimulationIterations))
   {
      iterations++;
      const float timeTick = GetSimulationTimeStep(remainingTime, iterations);
      remainingTime -= timeTick;

      RestorePreAdditiveRootMotionVelocity();

      const FVector oldLocation = UpdatedComponent->GetComponentLocation();
      const FVector oldVelocity = Velocity;

      ApplyRootMotionToVelocity(deltaTime);

      // Root motion could have put us into falling
      if (IsFalling())
      {
         StartNewPhysics(remainingTime, iterations);
         return;
      }

      // While climbing, our duration is updated and we're blocked from
      // starting a climb again until we touch the ground
      {
         _scrambleDurationElapsed += timeTick;
         _scrambleIsBlocked = true;
      }

      float velocityZFloor = TNumericLimits<float>::Lowest();

      // Apply acceleration input while climbing and under the climb duration.
      // We ignore acceleration magnitude, and just rely on the direction (which
      // has been constrained). If we're accelerating "forward", we apply velocity
      // in the forward and vertical directions.
      const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();
      if (_scrambleDurationElapsed < settings.Duration)
      {
         const FRotator rotation = PawnOwner->GetActorRotation();

         FVector localVelocity = rotation.UnrotateVector(Acceleration.GetSafeNormal());

         // Move up at a minimum base speed. If acceleration is applied, give an extra boost
         const float climbSpeedBase = FMath::Max(settings.MinStopSpeed, settings.ScrambleSpeed * settings.ScrambleSpeedBaseRatio);
         const float climbSpeedExtra = FMath::Max(0.0f, settings.ScrambleSpeed - climbSpeedBase);
         const float climbAcceleration = localVelocity.X;

         // Always move forward while climbing
         localVelocity.Z = climbSpeedBase + (climbAcceleration * climbSpeedExtra);
         localVelocity.X = MaxWalkSpeedCrouched;

         FVector worldVelocity = rotation.RotateVector(localVelocity);
         velocityZFloor = climbAcceleration > 0 ? worldVelocity.Z : worldVelocity.Z - 1;

         Velocity.X = (worldVelocity.X);
         Velocity.Y = (worldVelocity.Y);
         Velocity.Z = FMath::Max(Velocity.Z, worldVelocity.Z);
      }

      // Always apply gravity. Applying this when over the duration will effectively
      // cause us to slow down, ensuring that we stop once we reach the speed
      // threshold required to continue climbing.
      {
         const FVector gravityAccel(0.f, 0.f, GetGravityZ());
         Velocity = NewFallVelocity(Velocity, gravityAccel, timeTick);

         // Set min velocity to a floor of what it is "accelerating" to, otherwise the gravity causes frame-dependent jitter, since it is clobbered later
         Velocity.Z = FMath::Max(Velocity.Z, velocityZFloor);
      }

      // Default delta position is the velocity change (using midpoint integration method)
      FVector deltaPosition = 0.5f * (oldVelocity + Velocity) * timeTick;

      // Move; this is combined root motion and the delta position
      FHitResult moveHit(1.0f);
      SafeMoveUpdatedComponent(deltaPosition, UpdatedComponent->GetComponentQuat(), true, moveHit);

      if (moveHit.Time < 1.0f)
      {
         // We hit something; adjust and move again
         HandleImpact(moveHit, timeTick, deltaPosition);
         SlideAlongSurface(deltaPosition, (1.f - moveHit.Time), moveHit.Normal, moveHit, true);

         // If hit something, our final velocity is set to how much we actually moved (in the last iteration)
         Velocity = (UpdatedComponent->GetComponentLocation() - oldLocation) / timeTick;
      }
      _RelaxDistanceConstraintIfNeeded();
   }
}


//---------------------------------------------------------------------------------------
// Sprinting
//---------------------------------------------------------------------------------------

// Returns true if the character is currently sprinting
bool UOSECharacterMovement::IsSprinting() const
{
   return OSECharOwner && OSECharOwner->IsSprinting();
}

// Returns true if the character can ever sprint
bool UOSECharacterMovement::CanEverSprint() const
{
   return ((GetSprintSpeedIncrementBase() > KINDA_SMALL_NUMBER) || (GetSprintSpeedIncrementForward() > KINDA_SMALL_NUMBER));
}

// Returns true if the character is allowed to sprint in the current state
bool UOSECharacterMovement::CanSprintInCurrentState() const
{
   if (CharacterOwner == nullptr)
      return false;

   // Must be able to sprint
   if (!CanEverSprint())
      return false;

   // Owner must be able to sprint
   if (!OSECharOwner->CanSprint())
      return false;

   // Can't be sliding
   if (IsSliding())
      return false;

   // Check if we allow sprinting while climbing
   if (MovementCVars::ClimbBlocksSprint && IsScrambling())
      return false;

   // Check if we allow sprinting while mantling
   if (MovementCVars::MantleBlocksSprint && IsMantling())
      return false;

   // Must be moving to sprint
   const FVector localVelocity = GetLocalVelocity();
   if (localVelocity.SizeSquared2D() < KINDA_SMALL_NUMBER)
      return false;

   // Can't sprint backwards, though we're more forgiving once we're already sprinting
   // @TODO: Expose values
   // @TODO: Match values in GetMaxSpeed
   const float sprintAngleMax = FMath::Cos(FMath::DegreesToRadians(135.0f));
   const float sprintAngleMin = FMath::Cos(FMath::DegreesToRadians(140.0f));
   const float sprintAngleThreshold = (IsSprinting() ? sprintAngleMin : sprintAngleMax);
   return (localVelocity.GetSafeNormal2D().X > sprintAngleThreshold);
}

// Checks for sprint conditions, and handles the transition to and from sprinting state
void UOSECharacterMovement::UpdateSprintState()
{
   // Proxies get replicated sprint state
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      if (IsSprinting() && (!bWantsToSprint || !CanSprintInCurrentState()))
      {
         StopSprinting(false);
      }
      else if (!IsSprinting() && (bWantsToSprint && CanSprintInCurrentState()))
      {
         StartSprinting(false);
      }
   }
}

// Sets sprinting state, and trigger OnStartSprinting() on the owner if successful.
// In general you should set bWantsToSprint instead to have the sprint persist during movement, or just use the sprint functions on the owning character.
// @param   bClientSimulation   True when called when bIsSprinting is replicated to non owned clients
void UOSECharacterMovement::StartSprinting(bool bClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      OSECharOwner->SetIsSprinting(true);
   }

   OSECharOwner->OnStartSprinting();
}

// Sets sprinting state, and trigger OnStopSprinting() on the owner if successful.
// @param   bClientSimulation   True when called when bIsSprinting is replicated to non owned clients
void UOSECharacterMovement::StopSprinting(bool bClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      OSECharOwner->SetIsSprinting(false);
   }

   OSECharOwner->OnStopSprinting();
}


//---------------------------------------------------------------------------------------
// Mantle / Vault
//---------------------------------------------------------------------------------------

bool UOSECharacterMovement::IsMantling() const
{
   return OSECharOwner && OSECharOwner->IsMantling();
}

bool UOSECharacterMovement::CanEverMantle() const
{
   // @TODO: also check for valid settings struct?
   return GetMantleEnabled();
}

// Helper for saved moves. Allows us to keep the actual transient state protected
// while modified explicitly and with clear purpose.
void UOSECharacterMovement::SetMantleTransients(const FOSESavedMove_Character& savedMove)
{
   SetMantleMontage(savedMove.SavedMantleMontage.Get());
   SetMantleTimeElapsed(savedMove.SavedMantleTimeElapsed);
   SetMantleTimeRemaining(savedMove.SavedMantleTimeRemaining);
   SetMantleStartOffset(savedMove.SavedMantleStartOffset);
   SetMantleStartVelocity(savedMove.SavedMantleStartVelocity);
   _mantleHadRootMotion = savedMove.SavedMantleHadRootMotion;
}

// Given a mantle query, resolves it to a suitable mantle state. Returns true on success, and false on failure
bool UOSECharacterMovement::ResolveMantleState(FOSEMantleState& outState, const FOSEMantleQueryResult& inQuery) const
{
   outState = FOSEMantleState();

   if (!inQuery.IsValid())
      return false;

   // Check if the final position is a valid landing spot
   if (!IsValidLandingSpot(inQuery.FinalHitResult.Location, inQuery.FinalHitResult))
      return false;

   // The height delta for this query
   const float heightDelta = inQuery.FinalHitResult.Location.Z - inQuery.StartHitResult.Location.Z;

   const FOSEMantleAnimSearchParams searchParams = {
      .Movement = MovementMode,
      .CustomType = GetCustomMovementType(),
      .IsCrouching = IsCrouching(),
      .Speed = static_cast<float>(Velocity.Size2D()),
      .Height = heightDelta
   };

   // Search for a matching animation
   const TArray<UOSEMantleAnimSet*>& mantleAnims = GetMantleAnimations();
   for (const auto animSet : mantleAnims)
   {
      if (animSet == nullptr)
         continue;

      FOSEMantleAnim matchedAnim;
      if (animSet->FindAnimation(matchedAnim, searchParams))
      {
         outState.IsValid = inQuery.IsValid();
         outState.IsCrouching = searchParams.IsCrouching;
         outState.CapsuleExtents = FVector(inQuery.CapsuleRadius, inQuery.CapsuleRadius, inQuery.CapsuleHalfHeight);
         outState.Montage = matchedAnim.Montage;
         outState.QueryLocation = inQuery.QueryLocation;
         outState.QueryDirection = inQuery.QueryRotation.Vector();
         outState.StartLocation = inQuery.StartHitResult.Location;
         outState.StartDirection = (-inQuery.StartHitResult.Normal).GetSafeNormal2D();
         outState.StartPhysicalMaterial = UOSECommon::GetPhysicalMaterialFromHitResult(inQuery.StartHitResult);
         outState.FinalLocation = inQuery.FinalHitResult.Location;
         outState.FinalDirection = outState.StartDirection;

         // Offset the start to be the exact delta we require
         outState.StartLocation.Z = outState.FinalLocation.Z - matchedAnim.Height;

         // Matched
         return true;
      }
   }

   return false;
}

bool UOSECharacterMovement::CanMantleInCurrentState() const
{
   if (OSECharOwner == nullptr)
      return false;

   // Must be able to mantle
   if (!CanEverMantle())
      return false;

   // Owner must be able to mantle
   if (!OSECharOwner->CanMantle())
      return false;

   // Can't be sliding or crouching
   if (IsSliding() || (IsCrouching() && !_allowMantleFromCrouch))
      return false;

   FVector startDirection;
   if(!MovementCVars::LedgeEnableLedgeMantleCheck)
   {
      // Must have valid mantle data
      const FOSEMantleState& mantleState = OSECharOwner->GetMantleState();
      if (!mantleState.IsValid)
         return false;

      startDirection = mantleState.StartDirection;
   }
   else
   {
      // Must have valid ledge data
      const FOSELedgeState& ledgeState = OSECharOwner->GetLedgeState();
      if (!ledgeState.MountTarget.IsValid)
         return false;
      startDirection = ledgeState.MountTarget.StartDirection;
   }

   // Determine if we're moving into or away from the wall
   const float radianFaceAngle = 2.0f * FMath::DegreesToRadians(GetMantleSettings().MaxAngleFacing);
   const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
   const float cosineWallAngle = (Acceleration.GetSafeNormal2D() | startDirection);
   const bool isMovingAway = (cosineWallAngle <= -cosineFaceAngle);
   const bool isMovingInto = (cosineWallAngle >= +cosineFaceAngle);

   if (!IsMantling())
   {
      // We're not mantling yet. We can mantle if we're accelerating into the wall.
      return isMovingInto;
   }
   else
   {
      // We cancel the mantle if we're accelerating away and don't want to mantle anymore
      const bool isMantleCancelled = !GetWantsToMantle() && isMovingAway;

      // Mantle can end normally, or from external changes to movement mode
      const bool isMantleOver = GetMantleTimeRemaining() < MIN_TICK_TIME;
      const bool isMantleEnded = GetCustomMovementType() != ECustomMovementType::Mantle;

      // Keep mantling until we should stop
      return !(isMantleCancelled || isMantleEnded || isMantleOver);
   }
}

void UOSECharacterMovement::UpdateMantleState()
{
   // Proxies get replicated state
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      // If we're not mantling but we can mantle, then update our state
      if (!IsMantling() && OSECharOwner->CanMantle())
      {
         // Mantle query is initialized to invalid
         FOSEMantleQueryResult mantleQuery;

         // FIXME: Temporarily computing this unconditionally, as it was preventing the jump
         //        from being suppressed when there would have been a valid mantle location
         //        on the ground. But this will cause all mantle-capable characters to
         //        run this query, which is a sizable perf regression if NPCs have mantle
         //        enabled. So this should be followed up with a fix that keeps the mantle
         //        query pay-as-needed.
         //if (GetWantsToMantle())
         {
            // Query the raw mantle data only if we want to mantle
            mantleQuery = UOSEMantleQuery::TraceMantleCharacterWithVelocity(
               GetMantleSettings(), CharacterOwner, GetLastUpdateVelocity());
         }

         // Resolve it into the state we need for our conditions
         FOSEMantleState mantleState = OSECharOwner->GetMantleState();
         ResolveMantleState(mantleState, mantleQuery);
         OSECharOwner->SetMantleState(mantleState);
      }

      // Transition in and out of mantle mode
      if (!IsMantling() && GetWantsToMantle() && CanMantleInCurrentState())
      {
         StartMantle(false);
      }
      else if (IsMantling() && !CanMantleInCurrentState())
      {
         StopMantle(false);
      }
   }
}

// Sets mantle / vault state, and trigger OnStartMantle() on the owner if successful.
// In general you should set _wantsToMantle instead to have the mantle persist during movement, or just use the mantle functions on the owning character.
// @param   bClientSimulation   True when called when _mantleState is replicated to non owned clients
void UOSECharacterMovement::StartMantle(bool bClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   const FOSEMantleState& mantleState = OSECharOwner->GetMantleState();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   // Dump logging info when mantle state is not valid
   if (!ensure(mantleState.IsValid))
   {
      const FString mantleText = FString::Printf(TEXT("[%s %s]\t\tmantleState"), *GetReadableName(), *UEnum::GetValueAsString(GetOwnerRole()));
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s IsValid        : %d"), *mantleText, mantleState.IsValid);
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s IsMantling     : %d"), *mantleText, mantleState.IsMantling);
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s Sim Proxy      : %d"), *mantleText, bClientSimulation);
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s Capsule        : %s"), *mantleText, *mantleState.CapsuleExtents.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s Montage        : %s"), *mantleText, mantleState.Montage.IsValid() ? *(mantleState.Montage.Get()->GetName()) : TEXT("none"));
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s QueryLocation  : %s"), *mantleText, *mantleState.QueryLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s QueryDirection : %s"), *mantleText, *mantleState.QueryDirection.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s StartLocation  : %s"), *mantleText, *mantleState.StartLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s StartDirection : %s"), *mantleText, *mantleState.StartDirection.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s FinalLocation  : %s"), *mantleText, *mantleState.FinalLocation.ToCompactString());
      UE_LOG(LogOSECharacterMovement, Warning, TEXT("%s FinalDirection : %s"), *mantleText, *mantleState.FinalDirection.ToCompactString());
   }
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   if (!bClientSimulation)
   {
      // Simulated proxies get this via replication
      OSECharOwner->SetIsMantling(true);
   }

   // Change the start location if we're planning to scale the root motion to reach our goal
   const FVector startLocationResolved = mantleState.StartLocation;
   const FVector startLocationActual = FVector(
      mantleState.StartLocation.X,
      mantleState.StartLocation.Y,
      mantleState.QueryLocation.Z);

   const FVector startLocationToUse = (MovementCVars::MantleScaleRootMotionZ > 0) ? startLocationActual : startLocationResolved;

   // Clamp our start velocity to our current max speed
   const FVector startVelocity = GetLastUpdateVelocity().GetClampedToMaxSize(GetMaxSpeed());

   // Set transient mantle state
   SetMantleMontage(mantleState.Montage.Get());
   SetMantleTimeElapsed(0.0f);
   SetMantleTimeRemaining(0.0f);
   SetMantleStartOffset(startLocationToUse - mantleState.QueryLocation);
   SetMantleStartVelocity(startVelocity);
   _mantleHadRootMotion = false;

   // Store an approximate mantle duration; we use this to ensure the mantle ends eventually
   if (UAnimMontage* mantleMontage = GetMantleMontage())
   {
      const float animDuration = CharacterOwner->PlayAnimMontage(mantleMontage);
      const float finalDuration = FMath::Max(0.0f, animDuration - mantleMontage->GetDefaultBlendOutTime());
      SetMantleTimeRemaining(finalDuration);
   }

   SetCustomMovementType(ECustomMovementType::Mantle);

   // defer this if replaying after a correction
   if (!CharacterOwner->bClientUpdating)
   {
      OSECharOwner->OnStartMantling();
   }
}

// Sets mantle / vault state, and trigger OnStopMantle() on the owner if successful.
// @param   bClientSimulation   True when called when _isMantling is replicated to non owned clients
void UOSECharacterMovement::StopMantle(bool bClientSimulation /* = false */)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      // Simulated proxies get this via replication
      OSECharOwner->SetIsMantling(false);
   }

   // Stop the animation montage
   if (UAnimMontage* mantleMontage = GetMantleMontage())
   {
      CharacterOwner->StopAnimMontage(mantleMontage);
   }

   if (MovementCVars::MantleMaxVerticalExitSpeed >= 0)
   {
      Velocity.Z = FMath::Clamp(Velocity.Z, -MovementCVars::MantleMaxVerticalExitSpeed, MovementCVars::MantleMaxVerticalExitSpeed);
   }
   if (MovementCVars::MantleMaxLateralExitSpeed >= 0)
   {
      Velocity = Velocity.GetClampedToMaxSize2D(MovementCVars::MantleMaxLateralExitSpeed);
   }

   // Only change movement mode if we're still in mantle mode. External logic could
   // have explicitly set a new movement mode which would trigger the StopMantle(),
   // and we don't want to mess with what they set in that situation.
   if (GetCustomMovementType() == ECustomMovementType::Mantle)
      SetMovementMode(MOVE_Falling);

   // Reset the internal mantle state as well
   SetMantleMontage(nullptr);
   SetMantleTimeElapsed(0.0f);
   SetMantleTimeRemaining(0.0f);
   SetMantleStartOffset(FVector::ZeroVector);
   SetMantleStartVelocity(FVector::ZeroVector);
   _mantleHadRootMotion = false;

   // Require the request to be submitted again
   // @TODO: Make this an option (or cvar)
   SetWantsToMantle(false);
   SetWantsToScramble(false);
   
   // defer this if after a correction
   if (!CharacterOwner->bClientUpdating)
   {
      OSECharOwner->OnStopMantling();
   }
}

// Custom movement mode for mantle
void UOSECharacterMovement::PhysCustomMantle(float deltaTime, int32 iterations)
{
   if (deltaTime < MIN_TICK_TIME)
   {
      return;
   }

   const FOSEMantleState& mantleState = OSECharOwner->GetMantleState();
   if (!mantleState.IsValid)
   {
      return;
   }

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

      // Update the remaining time; the value is used to swap us out of mantle mode
      const float mantleTimeElapse = GetMantleTimeElapsed();
      const float mantleTimeRemain = GetMantleTimeRemaining();
      SetMantleTimeElapsed(mantleTimeElapse + timeTick);
      SetMantleTimeRemaining(FMath::Max(0.0f, mantleTimeRemain - timeTick));

      // MOMENTUM
      if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
      {
         const bool scaleRootMotionZ = MovementCVars::MantleScaleRootMotionZ > 0;
         if (scaleRootMotionZ)
         {
            // During the first half of the mantle, we'll use the height 
            // differences to compute a velocity to add every tick. This
            // velocity will be enough to compensate for the potential
            // difference in heights between our animation and our destination.
            if (mantleTimeElapse < mantleTimeRemain)
            {
               const float heightResolved = mantleState.FinalLocation.Z - mantleState.StartLocation.Z;
               const float heightActual = mantleState.FinalLocation.Z - mantleState.QueryLocation.Z;

               const float mantleTimeTotal = mantleTimeElapse + mantleTimeRemain;
               const float mantleTimeRootZ = mantleTimeTotal * 0.5f;
               if (mantleTimeRootZ > 0.0f)
               {
                  const float mantleHeightDiff = FMath::Max(0.0f, heightActual - heightResolved);
                  const float mantleHeightSpeed = mantleHeightDiff / mantleTimeRootZ;
                  Velocity += FVector::UpVector * mantleHeightSpeed;
               }
            }
         }

         const bool keepMomentum = MovementCVars::MantleKeepMomentum > 0;
         if (keepMomentum)
         {
            // If we're applying input during the second half of the mantle, accumulate
            // extra velocity such that we approximately match our speed when we
            // initiated the mantle. This allows us to continue momentum even if the
            // animation we're using would normally bring us to a full stop.
            if (mantleTimeRemain < mantleTimeElapse)
            {
               // We apply the speed in our expected final direction, and use it
               // to project our acceleration (input) direction
               const FVector finalVector = mantleState.FinalDirection;
               const float accelBias = finalVector | Acceleration.GetSafeNormal();

               // Our initial forward speed when we queried the mantle. Ensure we'll
               // move at least at walking speed (it is filtered later)
               const FVector queryVector = mantleState.QueryDirection;
               const float querySpeed = queryVector | GetMantleStartVelocity();
               const float speedNext = FMath::Max( MaxWalkSpeed, querySpeed);

               // We don't want to exceed the speed if our animation would bring us there anyways
               const float speedCurr = Velocity.Size2D();
               const float speedDiff = FMath::Max(0.0f, speedNext - speedCurr);

               // Progressively ramp up the amount of speed as our animation ends
               const float ratio = 1.0f - (mantleTimeRemain / mantleTimeElapse);
               const FVector extraVelocity = finalVector * speedDiff * accelBias * ratio;
               Velocity += extraVelocity;
            }
         }
      }

      // BLEND IN; BLEND OUT
      if (UAnimMontage* mantleMontage = GetMantleMontage())
      {
         FVector deltaPosition = FVector::ZeroVector;

         // Use the montage blend in time to move towards the start
         {
            const float defaultBlendInTime = mantleMontage->BlendIn.GetBlendTime();
            const float blendInRemains = defaultBlendInTime - mantleTimeElapse;
            const FVector mantleStartOffset = GetMantleStartOffset();

            if (blendInRemains < MIN_TICK_TIME)
            {
               // Blend in is done; make sure we apply the rest of the offset and clear it
               deltaPosition += mantleStartOffset;
               SetMantleStartOffset(FVector::ZeroVector);
            }
            else
            {
               // Compute how fast we need to finish the offset based on the blend in time,
               // apply it and clamp the distance, subtracting the result from the offset.
               const float offsetDistRemain = mantleStartOffset.Size();
               const float offsetSpeed = offsetDistRemain / blendInRemains;
               const float offsetDistClamped = FMath::Min(offsetDistRemain, offsetSpeed * timeTick);
               const FVector offsetAmount = mantleStartOffset.GetSafeNormal() * offsetDistClamped;

               deltaPosition += offsetAmount;
               SetMantleStartOffset(mantleStartOffset - offsetAmount);
            }
         }

         // Use the montage blend out time to move towards the floor
         {
            const float defaultBlendOutTime = mantleMontage->GetDefaultBlendOutTime();
            const float blendOutFactor = 1.0f - (mantleTimeRemain / defaultBlendOutTime);
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
                  if (floorResult.FloorDist <= MovementCVars::MantleFloorCheckDistance)
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
            else
            {
               if (MovementCVars::MantleCancelIfMontageEnded && _mantleHadRootMotion && !HasAnimRootMotion())
               {
                  UE_LOG(LogOSECharacterMovement, Log, TEXT("Canceling mantle with no more root motion"));
                  SetMovementMode(MOVE_Falling);
                  StartNewPhysics(remainingTime, iterations);
                  return;
               }
            }
         }

         Velocity += (deltaPosition / timeTick);
      }

      _mantleHadRootMotion = HasAnimRootMotion();

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

//---------------------------------------------------------------------------------------
// Wall Climbing
//---------------------------------------------------------------------------------------

bool UOSECharacterMovement::IsWallClimbing() const
{
   return IsValid(OSECharOwner) && OSECharOwner->IsWallClimbing();
}

bool UOSECharacterMovement::IsClimbableHit(const FHitResult& hitResult) const
{
   return hitResult.bBlockingHit;
}

void UOSECharacterMovement::StartWallClimb(bool bClientSimulation)
{
   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      OSECharOwner->SetIsWallClimbing(true);
   }

   if (MovementCVars::UseCrouchCapsuleForWallClimb)
   {
      ConvertCapsuleToCrouchSize(bClientSimulation);
   }

   SetWantsToWallClimb(false);
   _bManuallyCanceledWallClimb = false;

   SetCustomMovementType(ECustomMovementType::WallClimb);

   APlayerController* playerController = CharacterOwner->GetController<APlayerController>();
   if (playerController && playerController->IsLocalController())
   {
      UCommonInputSubsystem* inputSubsystem = UCommonInputSubsystem::Get(playerController->GetLocalPlayer());
      if (inputSubsystem && inputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
      {
         // Activate auto camera turn
         FVector wallClimbAccel = GetWallClimbAccelValue();
         FRotator newRotation = GetWallClimbAutoLookMountDirection(wallClimbAccel);
         float wallAngleInDegrees = GetAngleBetweenVectors(FVector::UpVector, _wallClimbImpactResult.Normal);
         if ((((-FVector::UpVector) | newRotation.Vector()) < 0.0f) && wallAngleInDegrees > GetCurrentWallClimbSettings().minimumCameraAutoTurnDegrees)
         {
            InitiateWallClimbAutoTurn(newRotation);
         }
      }
   }

   OSECharOwner->OnStartWallClimbing();
}

void UOSECharacterMovement::StopWallClimb(bool bClientSimulation)
{
   _wallClimbPendingResult.Init();
   _wallClimbImpactResult.Init();
   CurrentWall.HitResult.Init();

   _bClearedInitialWallClimbFloor = false;

   if (OSECharOwner == nullptr)
      return;

   if (!bClientSimulation)
   {
      OSECharOwner->SetIsWallClimbing(false);
   }

   if (_bIsWallDashing)
   {
      WallClimbDashFinished();
   }

   if (_bWallClimbCamAutoTurning)
   {
      ClearAutoTurn();
   }

   APlayerController* playerController = CharacterOwner->GetController<APlayerController>();
   if (playerController && playerController->IsLocalController())
   {
      UCommonInputSubsystem* inputSubsystem = UCommonInputSubsystem::Get(playerController->GetLocalPlayer());
      if (inputSubsystem && inputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
      {
         // Return view to a neutral pitch
         FRotator newRotationTarget = OSECharOwner->GetViewRotation();
         newRotationTarget.Pitch = 0.0f;
         InitiateWallClimbAutoTurn(newRotationTarget);
      }
   }

   RotateCharacterTowardsView();

   if (GetCustomMovementType() == ECustomMovementType::WallClimb)
      SetMovementMode(MOVE_Falling);

   RevertCapsuleToNormalSize(bClientSimulation);

   OSECharOwner->OnStopWallClimbing();
}

void UOSECharacterMovement::ConvertCapsuleToCrouchSize(bool bClientSimulation)
{
   // See if collision is already at desired size.
   if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == GetCrouchedHalfHeight())
   {
      return;
   }

   if (bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
   {
      // restore collision size before crouching
      ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
      CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
      bShrinkProxyCapsule = true;
   }

   // Change collision size to crouching dimensions
   const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
   const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
   const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
   // Height is not allowed to be smaller than radius.
   const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, GetCrouchedHalfHeight());
   CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedCrouchedHalfHeight);
   float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
   float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

   if (!bClientSimulation)
   {
      // Crouching to a larger height? (this is rare)
      if (ClampedCrouchedHalfHeight > OldUnscaledHalfHeight)
      {
         FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
         FCollisionResponseParams ResponseParam;
         InitCollisionParams(CapsuleParams, ResponseParam);
         const bool bEncroached = GetWorld()->OverlapBlockingTestByChannel(UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, ScaledHalfHeightAdjust), FQuat::Identity,
            UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

         // If encroached, cancel
         if (bEncroached)
         {
            CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, OldUnscaledHalfHeight);
            return;
         }
      }

      if (bCrouchMaintainsBaseLocation)
      {
         // Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
         UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
      }
   }

   bForceNextFloorCheck = true;

   // We take the change from the Default size, not the current one (though they are usually the same).
   const float MeshAdjust = ScaledHalfHeightAdjust;
   ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
   HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedCrouchedHalfHeight);
   ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

   AdjustProxyCapsuleSize();

   // Don't smooth this change in mesh position
   if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
   {
      FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
      if (ClientData)
      {
         ClientData->MeshTranslationOffset -= FVector(0.f, 0.f, MeshAdjust);
         ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
      }
   }
}

void UOSECharacterMovement::RevertCapsuleToNormalSize(bool bClientSimulation)
{
   ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

   // See if collision is already at desired size.
   if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())
   {
      return;
   }

   const float CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

   const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
   const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
   const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
   const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
   const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

   // Grow to uncrouched size.
   check(CharacterOwner->GetCapsuleComponent());

   if (!bClientSimulation)
   {
      // Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
      const UWorld* MyWorld = GetWorld();
      const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
      FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
      FCollisionResponseParams ResponseParam;
      InitCollisionParams(CapsuleParams, ResponseParam);

      // Compensate for the difference between current capsule size and standing size
      const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
      const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
      bool bEncroached = true;

      if (!bCrouchMaintainsBaseLocation)
      {
         // Expand in place
         bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

         if (bEncroached)
         {
            // Try adjusting capsule position to see if we can avoid encroachment.
            if (ScaledHalfHeightAdjust > 0.f)
            {
               // Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
               float PawnRadius, PawnHalfHeight;
               CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
               const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
               const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
               const FVector Down = FVector(0.f, 0.f, -TraceDist);

               FHitResult Hit(1.f);
               const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
               const bool bBlockingHit = MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
               if (Hit.bStartPenetrating)
               {
                  bEncroached = true;
               }
               else
               {
                  // Compute where the base of the sweep ended up, and see if we can stand there
                  const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
                  const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
                  bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
                  if (!bEncroached)
                  {
                     // Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
                     UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
                  }
               }
            }
         }
      }
      else
      {
         // Expand while keeping base location the same.
         FVector StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
         bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

         if (bEncroached)
         {
            if (IsMovingOnGround())
            {
               // Something might be just barely overhead, try moving down closer to the floor to avoid it.
               const float MinFloorDist = UE_KINDA_SMALL_NUMBER * 10.f;
               if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
               {
                  StandingLocation.Z -= CurrentFloor.FloorDist - MinFloorDist;
                  bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
               }
            }
         }

         if (!bEncroached)
         {
            // Commit the change in location.
            UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
            bForceNextFloorCheck = true;
         }
      }

      // If still encroached then abort.
      if (bEncroached)
      {
         return;
      }

      CharacterOwner->bIsCrouched = false;
   }
   else
   {
      bShrinkProxyCapsule = true;
   }

   // Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
   CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);

   const float MeshAdjust = ScaledHalfHeightAdjust;
   AdjustProxyCapsuleSize();
   CharacterOwner->OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

   // Don't smooth this change in mesh position
   if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
   {
      FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
      if (ClientData)
      {
         ClientData->MeshTranslationOffset += FVector(0.f, 0.f, MeshAdjust);
         ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
      }
   }
}

bool UOSECharacterMovement::CanEverWallClimb() const
{
   return GetWallClimbEnabled();
}

bool UOSECharacterMovement::CanWallClimbInCurrentState(bool bIgnoreWalkableFloor) const
{
   if (!IsValid(OSECharOwner))
      return false;

   // Owner must be able to wall climb
   if (!OSECharOwner->CanWallClimb())
      return false;

   // Can't be sliding or crouching or ledge climbing or mantling
   FOSEWallClimbSettings wallClimbSettings = GetCurrentWallClimbSettings();
   if (IsSliding() || IsCrouching() || IsInLedgeState() || IsMantling() || (wallClimbSettings.bNoWalkingWallAttachment && IsWalking()))
      return false;

   // Determine if we're moving into or away from the wall.
   // Note this is a more restrictive, 2D angle check than the one in IsValidWallClimbHitResult().
   // This is intentional as this angle is only used when initiating a wall climb.
   const bool bWantsToMoveAway = !IsVectorWithinWallMovementBounds(Acceleration);
   const bool bIsMovingAway = !IsVectorWithinWallMovementBounds(Velocity);
   const bool bIsGlancingHit = !IsVectorWithinWallMovementBounds(OSECharOwner->GetActorRotation().Vector());
   
   if (_bIsWallDashing && !_wallClimbImpactResult.IsValidBlockingHit())
   {
      return false;
   }

   if (!IsWallClimbing())
   { 
      // Always reject wall climb if our manual cancel cooldown is active
      UWorld* world = GetWorld();
      if (IsValid(world) && world->GetTimerManager().GetTimerRemaining(_manuallyCanceledWallClimbCooldownTimer) > 0)
      {
         return false;
      }

      // Don't attach to a wall if it's not a blocking hit
      if (!_wallClimbImpactResult.IsValidBlockingHit())
      {
         return false;
      }

      // If the wall impact is walkable, or the impact is glancing, don't transition to wall climb
      if (IsWalkable(_wallClimbImpactResult) || bIsGlancingHit)
      {
         return false;
      }

      // Do we want to require input for the player to attach to a wall?
      if (wallClimbSettings.bRequireInputToAttach && Acceleration == FVector::ZeroVector)
      {
         return false;
      }

      // Determine if we're inputting a direction away from the wall
      float accelWallAngle = GetAngleBetweenVectors(Acceleration.GetSafeNormal2D(), -_wallClimbImpactResult.ImpactNormal.GetSafeNormal2D());
      if (Acceleration != FVector::ZeroVector && accelWallAngle > wallClimbSettings.DeflectionWallAngleDegrees)
      {
         return false;
      }

      // Player manually canceled wall climb. Don't reattach unless they DI manually towards the wall again.
      if (Acceleration == FVector::ZeroVector && _bManuallyCanceledWallClimb)
      {
         return false;
      }

      //Check if there is floor immediately below the wall, if there is don't transition to wall climb
      FFindFloorResult floorResult;
      FindFloor(UpdatedComponent->GetComponentLocation(), floorResult, false, NULL);
      if (!bIgnoreWalkableFloor && floorResult.IsWalkableFloor() && IsWalkable(floorResult.HitResult))
      {
         return false;
      }
   }

   // Check if this is a surface we should not be allowed to wall climb on
   if (!_CanWallClimbOnSurface(_wallClimbImpactResult))
   {
      return false;
   }

   return true;
}

bool UOSECharacterMovement::IsVectorWithinWallMovementBounds(const FVector& queryVector) const
{
   if (!IsValid(UpdatedComponent) || !IsValid(OSECharOwner))
   {
      return false;
   }

   float lookYawAngle = GetAngleBetweenVectors(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), OSECharOwner->GetViewRotation().Vector().GetSafeNormal2D());
   return lookYawAngle < GetCurrentWallClimbSettings().WallMovementAngleDegrees;
}

bool UOSECharacterMovement::IsValidWallClimbHitResult(const FHitResult& impact) const
{
   if (!IsValid(CharacterOwner))
      return false;

   if (!IsClimbableHit(impact))
      return false;

   // Determine if wall normal is a valid climable angle
   float wallAngleInDegrees = GetAngleBetweenVectors(FVector::UpVector, impact.Normal);
   const FOSEWallClimbSettings& settings = GetCurrentWallClimbSettings();
   if (wallAngleInDegrees < settings.MinClimbableWallAngle || wallAngleInDegrees > settings.MaxClimbableWallAngle)
   {
      return false;
   }

   if (!IsWallWideEnough(impact))
   {
      return false;
   }

   // Ensure we're facing the impact
   if (!IsWallClimbing())
   {
      // Note that this is a less restrictive angle check than the one used to initiate climbing.
      // Initiating a climb uses this hit result if it is valid to apply the more restrictive check.
      float MaxAngleFacing = 30.0f; //Maybe make wall climb settings for this
      const FVector pawnVector = CharacterOwner->GetActorRotation().Vector();
      const float radianFaceAngle = 2.0f * FMath::DegreesToRadians(MaxAngleFacing);
      const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
      const float cosineWallAngle = (pawnVector | -impact.Normal);
      if (cosineWallAngle < cosineFaceAngle)
         return false;

      // Determine if wall contact point is too off center
      FVector playerToImpactPoint = (impact.ImpactPoint - CharacterOwner->GetActorLocation()).GetSafeNormal2D();
      float wallPositionAngleInDegrees = GetAngleBetweenVectors(CharacterOwner->GetActorForwardVector(), playerToImpactPoint);
      if (wallPositionAngleInDegrees > settings.DeflectionWallAngleDegrees)
         return false;
   }

   return true;
}

bool UOSECharacterMovement::IsWallWideEnough(const FHitResult& impact) const
{
   // If we're not opting in to this check, just return true so that we're not blocking a potential climb.
   if(!_checkIfWallIsWideEnoughToClimb)
   {
      return true;
   }
   
   UWorld* world = GetWorld();
   if (!IsValid(world) || !IsValid(CharacterOwner))
   {
      return false;
   }

   FVector wallToCharacter = CharacterOwner->GetActorLocation() - impact.ImpactPoint;
   wallToCharacter.Z = 0.0f;

   FVector relativeCharacterCenter = impact.ImpactPoint + wallToCharacter;
   FVector characterToWall = -wallToCharacter;
   FVector characterLeftSideDirection = FVector::CrossProduct(characterToWall, FVector::UpVector);
   FVector characterLeftSideStart = relativeCharacterCenter + characterLeftSideDirection;
   FVector characterRightSideStart = relativeCharacterCenter - characterLeftSideDirection;
   FVector scaledCharacterToWall = characterToWall * 1.2f;

   FVector aroundCornerDestination = relativeCharacterCenter + (2.0f * characterToWall);
   FVector aroundCornerLeftDirection = aroundCornerDestination - characterLeftSideStart;
   FVector aroundCornerRightDirection = aroundCornerDestination - characterRightSideStart;
   FVector scaledAroundCornerLeftDirection = aroundCornerLeftDirection * 1.2f;
   FVector scaledAroundCornerRightDirection = aroundCornerRightDirection * 1.2f;

   FCollisionQueryParams traceQueryParams;
   traceQueryParams.AddIgnoredActor(CharacterOwner);

   // We first do a trace from the left side of the capsule towards the wall on the same axis as the impact point
   FHitResult leftHit;
   world->LineTraceSingleByChannel(leftHit, characterLeftSideStart, characterLeftSideStart + scaledCharacterToWall, ECollisionChannel::ECC_Visibility, traceQueryParams);

   // We then do a trace from the right side of the capsule towards the wall
   FHitResult rightHit;
   world->LineTraceSingleByChannel(rightHit, characterRightSideStart, characterRightSideStart + scaledCharacterToWall, ECollisionChannel::ECC_Visibility, traceQueryParams);

   // Both traces missed. Before we do angled traces to test if we're going around a corner, 
   // lets make sure those traces don't detect a wall that is detached from our current wall.
   if (!leftHit.bBlockingHit && !rightHit.bBlockingHit)
   {
      FHitResult middleHit;
      FVector traceStart = relativeCharacterCenter + scaledCharacterToWall;
      world->LineTraceSingleByChannel(middleHit, traceStart, traceStart + characterToWall, ECollisionChannel::ECC_Visibility, traceQueryParams);
      if (middleHit.bBlockingHit)
      {
         return false;
      }
   }

   if (!leftHit.bBlockingHit)
   {
      // If we didn't hit anything on the first trace we may be going around a corner. We then test at an angle to see if there's a climbable wall around the corner.
      world->LineTraceSingleByChannel(leftHit, characterLeftSideStart + scaledCharacterToWall, characterLeftSideStart + scaledCharacterToWall + scaledAroundCornerLeftDirection, ECollisionChannel::ECC_Visibility, traceQueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
      DrawDebugDirectionalArrow(world, characterLeftSideStart + scaledCharacterToWall, characterLeftSideStart + scaledCharacterToWall + scaledAroundCornerLeftDirection, 10.0f, FColor::Green);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   }

   if (!rightHit.bBlockingHit)
   {
      // Just like with the left side we test at an angle if we hit nothing
      world->LineTraceSingleByChannel(rightHit, characterRightSideStart + scaledCharacterToWall, characterRightSideStart + scaledCharacterToWall + scaledAroundCornerRightDirection, ECollisionChannel::ECC_Visibility, traceQueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
      DrawDebugDirectionalArrow(world, characterRightSideStart + scaledCharacterToWall, characterRightSideStart + scaledCharacterToWall + scaledAroundCornerRightDirection, 10.0f, FColor::Green);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   DrawDebugDirectionalArrow(world, impact.ImpactPoint, relativeCharacterCenter, 10.0f, FColor::Blue);
   DrawDebugDirectionalArrow(world, characterLeftSideStart, characterLeftSideStart + scaledCharacterToWall, 10.0f, FColor::Green);
   DrawDebugDirectionalArrow(world, characterRightSideStart, characterRightSideStart + scaledCharacterToWall, 10.0f, FColor::Green);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   if (!IsClimbableHit(leftHit) || !IsClimbableHit(rightHit))
   {
      return false;
   }

   return true;
}

float UOSECharacterMovement::GetAngleBetweenVectors(const FVector& firstVector, const FVector& secondVector) const
{
   float angleDotProduct = FVector::DotProduct(firstVector, secondVector);
   FVector angleCrossProduct = FVector::CrossProduct(firstVector, secondVector);
   float angleInRadians = FMath::Atan2(angleCrossProduct.Size(), angleDotProduct);
   return FMath::RadiansToDegrees(angleInRadians);
}

void UOSECharacterMovement::RotateCharacterTowardsWall()
{
   const FRotator wallRotation = _wallDirection.ToOrientationRotator();

   FHitResult hit;
   SafeMoveUpdatedComponent(FVector::ZeroVector, wallRotation, true, hit);
}

void UOSECharacterMovement::RotateCharacterTowardsView()
{
   if (!IsValid(CharacterOwner))
   {
      return;
   }

   FRotator viewRotation = CharacterOwner->GetViewRotation();
   viewRotation.Pitch = 0.0f;

   FHitResult hit;
   SafeMoveUpdatedComponent(FVector::ZeroVector, viewRotation, true, hit);
}

void UOSECharacterMovement::UpdateWallClimbState(float deltaTime)
{
   if (!IsValid(OSECharOwner))
   {
      return;
   }

   if (!OSECharOwner->CanWallClimb())
   {
      return;
   }

   if (CanEverWallClimb())
   {
      // Update our pending hits, if any
      {
         const FHitResult impact = _wallClimbPendingResult;
         _wallClimbPendingResult.Init();

         if (IsValidWallClimbHitResult(impact))
         {
            // Could climb this
            _wallClimbImpactResult = impact;
            SetWallClimbDirection((-_wallClimbImpactResult.Normal).GetSafeNormal2D());

            if (IsWallClimbing())
            {
               _cachedMovementMaterialHitResult = _wallClimbImpactResult;
            }
         }
         else
         {
            _wallClimbImpactResult.Init();
         }
      }
   }

   // Proxies get replicated state
   if (OSECharOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      bool bCanWallClimb = CanWallClimbInCurrentState();

      if (!IsWallClimbing())
      {
         if (bCanWallClimb)
         {
            StartWallClimb(false);
         }
         else if (CanWallClimbAutoJump())
         {
            StartWallClimb(false);
            InitiateWallClimbDash(OSECharOwner->GetActorUpVector(), true);
         }
      }
      else if (!bCanWallClimb || bWantsToCrouch)
      {
         if (bWantsToCrouch)
         {
            bWantsToCrouch = false;
            _bManuallyCanceledWallClimb = true;

            // Start cooldown for manual crouch cancel. We dont allow the player to reattach for a set period of time.
            UWorld* world = GetWorld();
            if (IsValid(world))
            {
               world->GetTimerManager().SetTimer(_manuallyCanceledWallClimbCooldownTimer, this, &UOSECharacterMovement::ManuallyCanceledWallClimbCooldownFinished, GetCurrentWallClimbSettings().manuallyCanceledWallClimbCooldown, false);
            }
         }

         StopWallClimb(false);
      }
   }

   if (IsWallClimbing())
   {
      Acceleration = GetWallClimbAccelValue();
   }

   if (OSECharOwner->IsLocallyControlled() && _bWallClimbCamAutoTurning)
   {
      FRotator currentViewRotaion = OSECharOwner->GetViewRotation();
      bool bHasRotationInputThisFrame = _rotationInputThisFrame != FRotator::ZeroRotator;
      if (_wallClimbAutoTurnTarget.Equals(currentViewRotaion, 1.0f) || bHasRotationInputThisFrame)
      {
         ClearAutoTurn();
      }
      else
      {
         float angleBetweenOriginAndTarget = GetAngleBetweenVectors(_wallClimbAutoTurnOrigin.Vector(), _wallClimbAutoTurnTarget.Vector());
         float angleBetweenCurrentAndTarget = GetAngleBetweenVectors(OSECharOwner->GetViewRotation().Vector(), _wallClimbAutoTurnTarget.Vector());

         float autoTurnCompletionPercentage = 1.0f - (angleBetweenCurrentAndTarget / angleBetweenOriginAndTarget);

         APlayerController* pc = OSECharOwner->GetController<APlayerController>();
         if (IsValid(pc))
         {
            if (UCurveFloat* autoTurnSpeedCurve = GetCurrentWallClimbSettings().cameraAutoTurnSpeedCurve)
            {
               pc->SetControlRotation(FMath::RInterpTo(currentViewRotaion, _wallClimbAutoTurnTarget, deltaTime, autoTurnSpeedCurve->GetFloatValue(autoTurnCompletionPercentage)));
            }
            else
            {
               UE_LOG(LogOSECharacterMovement, Warning, TEXT("cameraAutoTurnSpeedCurve curve not found for '%s'"), *GetName());
            }
         }
      }
   }

   _rotationInputThisFrame = FRotator::ZeroRotator;
}

void UOSECharacterMovement::InitiateWallClimbAutoTurn(const FRotator& turnTarget)
{
   if (!IsValid(OSECharOwner) || !GetCurrentWallClimbSettings().bEnableCameraAutoTurn)
   {
      return;
   }

   _wallClimbAutoTurnOrigin = OSECharOwner->GetViewRotation();
   _wallClimbAutoTurnTarget = turnTarget;
   _bWallClimbCamAutoTurning = true;
}

FRotator UOSECharacterMovement::GetWallClimbAutoLookMountDirection(const FVector& currentAccel)
{
   APlayerController* pc = IsValid(OSECharOwner) ? OSECharOwner->GetController<APlayerController>() : NULL;
   if (!IsValid(pc))
   {
      if (IsValid(OSECharOwner))
      {
         return OSECharOwner->GetViewRotation();
      }

      return FRotator::ZeroRotator;
   }

   FVector newLookDirection = currentAccel.GetSafeNormal();
   FVector wallNormal = GetWallAngleNormal();

   // If wall is inverted, just look directly up
   if ((wallNormal | OSECharOwner->GetActorUpVector()) < 0)
   {
      wallNormal = (UpdatedComponent->GetComponentLocation() - _wallClimbImpactResult.ImpactPoint).GetSafeNormal2D();
   }

   newLookDirection = FVector::VectorPlaneProject(newLookDirection, wallNormal).GetSafeNormal();

   // If yaw is beyond limit for wall climb movement, rotate new look direction until it's within movement bounds
   FVector localLookDirection = FRotationMatrix::MakeFromX(UpdatedComponent->GetForwardVector()).InverseTransformVector(newLookDirection);
   float lookYawAngle = GetAngleBetweenVectors(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), newLookDirection.GetSafeNormal2D());
   if (lookYawAngle > GetCurrentWallClimbSettings().WallMovementAngleDegrees)
   {
      float angleDifference = lookYawAngle - GetCurrentWallClimbSettings().WallMovementAngleDegrees;
      if (localLookDirection.Y > 0.0f)
      {
         angleDifference *= -1.0;
      }

      newLookDirection = newLookDirection.RotateAngleAxis(angleDifference, FVector::UpVector);
   }

   FRotator newRotation = newLookDirection.Rotation();
   if (newRotation.Yaw == 0.0f)
   {
      newRotation.Yaw = OSECharOwner->GetViewRotation().Yaw;
   }

   if (IsValid(pc->PlayerCameraManager) && newRotation.Pitch > pc->PlayerCameraManager->ViewPitchMax)
   {
      newRotation.Pitch = pc->PlayerCameraManager->ViewPitchMax;
      newRotation.Yaw = OSECharOwner->GetViewRotation().Yaw;
   }

   return newRotation;
}

FVector UOSECharacterMovement::GetWallClimbAccelValue()
{
   if (!IsValid(UpdatedComponent) || !IsValid(CharacterOwner))
   {
      return Acceleration;
   }

   if (_bIsWallDashing)
   {
      // Accel variable is in local space. We convert to world space here.
      return UpdatedComponent->GetComponentTransform().TransformVectorNoScale(_wallClimbDashLocalAccelNormal) * GetCurrentWallClimbSettings().WallClimbDashAcceleration;
   }
   else if (!IsVectorWithinWallMovementBounds(CharacterOwner->GetViewRotation().RotateVector(FVector::ForwardVector)))
   {
      // If the character is facing away from the wall we want to stop movement
      return FVector::ZeroVector;
   }
   else
   {
      // Rotate acceleration to align with the wall direction as the X-axis
      FVector rotatedAccel;
      FVector axis = CharacterOwner->GetViewRotation().RotateVector(FVector::ForwardVector);
      FVector axisNormal = axis.GetSafeNormal();
      FVector forwardVector = FVector(axisNormal.X, axisNormal.Y, 0.0f);
      FQuat rotationQuat = FQuat::FindBetween(forwardVector, UpdatedComponent->GetForwardVector());
      rotatedAccel = Acceleration.RotateAngleAxis(rotationQuat.Rotator().Yaw, UpdatedComponent->GetUpVector());

      // Turn Accel values into local space and convert x to z axis (climb up wall)
      FVector localAccel = FRotationMatrix::MakeFromX(UpdatedComponent->GetForwardVector()).InverseTransformVector(rotatedAccel);
      localAccel = FVector(0.0f, localAccel.Y, localAccel.X);

      // Change Accel back into world space
      FVector localX = UpdatedComponent->GetForwardVector();
      FVector localY = FVector::CrossProduct(UpdatedComponent->GetUpVector(), localX);
      FVector localZ = FVector::CrossProduct(localX, localY);
      FTransform localToWorldTransform = FTransform(localX, localY, localZ, FVector::ZeroVector);
      return localToWorldTransform.TransformVector(localAccel);
   }
}

void UOSECharacterMovement::ManuallyCanceledWallClimbCooldownFinished()
{
   UWorld* world = GetWorld();
   if (IsValid(world))
   {
      world->GetTimerManager().ClearTimer(_manuallyCanceledWallClimbCooldownTimer);
   }
}

void UOSECharacterMovement::ClearAutoTurn()
{
   _wallClimbAutoTurnOrigin = FRotator::ZeroRotator;
   _wallClimbAutoTurnTarget = FRotator::ZeroRotator;
   _bWallClimbCamAutoTurning = false;
}

bool UOSECharacterMovement::HasWallClimbClearance()
{
   FHitResult hit;
   GetCharacterRaycastHit(hit, RAY_Head);
   if (!IsValidWallClimbHitResult(hit))
   {
      return false;
   }

   GetCharacterRaycastHit(hit, RAY_Feet);
   if (!IsValidWallClimbHitResult(hit))
   {
      return false;
   }

   return true;
}

bool UOSECharacterMovement::CanWallClimbAutoJump() const
{
   const bool isMovingInto = IsVectorWithinWallMovementBounds(Acceleration);
   return CanWallClimbInCurrentState(true) && IsWalking() && isMovingInto;
}

void UOSECharacterMovement::PhysCustomWallClimb(float deltaTime, int32 iterations)
{
   if (deltaTime < MIN_TICK_TIME)
   {
      return;
   }

   bJustTeleported = false;
   bool bCheckedFall = false;
   bool bTriedLedgeMove = false;
   float remainingTime = deltaTime;

   // Perform the move
   while ((remainingTime >= MIN_TICK_TIME) && (iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
   {
      iterations++;
      bJustTeleported = false;
      const float timeTick = GetSimulationTimeStep(remainingTime, iterations);
      remainingTime -= timeTick;

      // Save current values
      UPrimitiveComponent* const OldBase = GetMovementBase();
      const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
      const FVector OldLocation = UpdatedComponent->GetComponentLocation();
      const FFindFloorResult OldFloor = CurrentFloor;

      CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());

      // Compute move parameters
      const FVector MoveVelocity = Velocity;
      const FVector Delta = timeTick * MoveVelocity;
      const bool bZeroDelta = Delta.IsNearlyZero();
      FStepDownResult StepDownResult;

      if (bZeroDelta)
      {
         remainingTime = 0.f;
      }
      else
      {
         MoveAlongWall(MoveVelocity, timeTick, &StepDownResult);
      }

      FVector currentAccelNormal = Acceleration.GetSafeNormal();
      if (_bIsWallDashing)
      {
         currentAccelNormal = _wallClimbDashLocalAccelNormal;
      }
      if (currentAccelNormal.Z != 0.0f)
      {
         FFindFloorResult floorResult;
         FindFloor(UpdatedComponent->GetComponentLocation(), floorResult, bZeroDelta, NULL);
         //only check the ground for wall climb exit if we ware headed down
         if (floorResult.IsWalkableFloor() && IsWalkable(floorResult.HitResult))
         {
            if (_bClearedInitialWallClimbFloor)
            {
               StopWallClimb(false);
               SetMovementMode(MOVE_Walking);
               break;
            }
         }
         else
         {
            _bClearedInitialWallClimbFloor = true;
         }
      }

      FFindWallResult PendingCurrentWall;
      FindWall(UpdatedComponent->GetComponentLocation(), PendingCurrentWall, bZeroDelta);

      // Check angle restriction for corners
      const float radianFaceAngle = FMath::DegreesToRadians(180.0f - GetCurrentWallClimbSettings().MinAngleCornerTraversal);
      const float cosineFaceAngle = FMath::Cos(radianFaceAngle);
      const float cosineViewWallAngle = (PendingCurrentWall.HitResult.ImpactNormal.GetSafeNormal2D() | CurrentWall.HitResult.ImpactNormal.GetSafeNormal2D());
      
      // Revert moves if...
      // The new wall is not valid for wall climb
      // The new wall is too acute of an angle to traverse to
      if (!IsValidWallClimbHitResult(PendingCurrentWall.HitResult) || (CurrentWall.HitResult.ImpactNormal != FVector::ZeroVector && CurrentWall.HitResult.ImpactNormal != PendingCurrentWall.HitResult.ImpactNormal && cosineViewWallAngle < cosineFaceAngle))
      {
         RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);
      }
      else if (PendingCurrentWall.bBlockingHit)
      {
         CurrentWall = PendingCurrentWall;
         _wallClimbPendingResult = CurrentWall.HitResult;
      }

      AdjustWallDist();
      RotateCharacterTowardsWall();
   }
}

void UOSECharacterMovement::AdjustWallDist()
{
   // If we have a wall check that hasn't hit anything, don't adjust distance.
   if (!CurrentWall.IsClimableWall())
   {
      return;
   }

   float oldWallDist = CurrentWall.WallDist;
   if (CurrentWall.bLineTrace)
   {
      if (oldWallDist < MIN_WALL_DIST && CurrentWall.LineDist >= MIN_WALL_DIST)
      {
         return;
      }
      else
      {
         // Falling back to a line trace means the sweep was unclimable (or in penetration). Use the line distance for the adjustment.
         oldWallDist = CurrentWall.LineDist;
      }
   }

   // Move up or down to maintain wall distance.
   if (oldWallDist < MIN_WALL_DIST || oldWallDist > MAX_WALL_DIST)
   {
      FHitResult adjustHit(1.f);
      const FVector initialLoc = UpdatedComponent->GetComponentLocation();
      const float avgWallDist = (MIN_WALL_DIST + MAX_WALL_DIST) * 0.5f;
      const float moveDist = avgWallDist - oldWallDist;

      SafeMoveUpdatedComponent(-UpdatedComponent->GetForwardVector() * moveDist, UpdatedComponent->GetComponentQuat(), true, adjustHit);

      if (!adjustHit.IsValidBlockingHit())
      {
         CurrentWall.WallDist += moveDist;
      }
      else if (moveDist > 0.f)
      {
         const FVector currentLoc = UpdatedComponent->GetComponentLocation();
         CurrentWall.WallDist += (currentLoc - initialLoc).Size();
      }
      else
      {
         checkSlow(moveDist < 0.f);
         const FVector currentLoc = UpdatedComponent->GetComponentLocation();
         CurrentWall.WallDist = -(currentLoc - adjustHit.Location).Size();
         if (adjustHit.IsValidBlockingHit())
         {
            CurrentWall.SetFromSweep(adjustHit, CurrentWall.WallDist, true);
         }
      }

      bJustTeleported |= oldWallDist < 0.f;

      // If something caused us to adjust our wall distance (especially a depentration) we should ensure another check next frame or we will keep a stale result.
      if (CharacterOwner && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
      {
         bForceNextWallCheck = true;
      }
   }
}

const FOSEWallClimbSettings& UOSECharacterMovement::_GetWallClimbSettings(const FHitResult& impact) const
{
   if (UPhysicalMaterial* physMat = impact.PhysMaterial.Get())
   {
      if (const FOSEWallClimbSettings* surfaceOverride = _surfaceWallClimbSettingOverrides.Find(physMat->SurfaceType))
      {
         return *surfaceOverride;
      }
   }

   return _wallClimbSettings;
}

void UOSECharacterMovement::MoveAlongWall(const FVector& inVelocity, float deltaSeconds, FStepDownResult* outStepDownResult)
{
   if (!IsValid(UpdatedComponent))
   {
      return;
   }

   FVector mutableVelocity = inVelocity;

   // We used to have negative collisions but our comp (TOTK) does not have that. 
   // We want to try to not have negative collisions for a while so I'm removing this for now.
   //CheckNegativeCollision(mutableVelocity);

   FHitResult hit(1.f);
   FVector delta = mutableVelocity * deltaSeconds;
   FVector rampVector = ComputeWallMovementDelta(delta, _wallClimbImpactResult);
   SafeMoveUpdatedComponent(rampVector, UpdatedComponent->GetComponentQuat(), true, hit);
   float lastMoveTimeSlice = deltaSeconds;

   if (hit.bStartPenetrating)
   {
      // Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
      //HandleImpact(hit);
      //SlideAlongSurface(helta, 1.f, hit.Normal, hit, true);

      //if (hit.bStartPenetrating)
      //{
      //   OnCharacterStuckInGeometry(&hit);
      //}
   }
   else if (hit.IsValidBlockingHit())
   {
      _wallClimbPendingResult = hit;

      FVector awayFromWall = -UpdatedComponent->GetForwardVector();
      FVector awayFromWallAxis = hit.Normal.ProjectOnTo(awayFromWall);

      // We impacted something (most likely another ramp, but possibly a barrier).
      float percentTimeApplied = hit.Time;
      if ((hit.Time > 0.f) && (awayFromWallAxis.Size() > UE_KINDA_SMALL_NUMBER) /*CanWallClimbOnSurface() maybe?*/)
      {
         // Another walkable ramp.
         const float initialPercentRemaining = 1.f - percentTimeApplied;
         rampVector = ComputeWallMovementDelta(delta * initialPercentRemaining, hit);
         lastMoveTimeSlice = initialPercentRemaining * lastMoveTimeSlice;
         SafeMoveUpdatedComponent(rampVector, UpdatedComponent->GetComponentQuat(), true, hit);

         const float secondHitPercent = hit.Time * initialPercentRemaining;
         percentTimeApplied = FMath::Clamp(percentTimeApplied + secondHitPercent, 0.f, 1.f);
      }

      //if (hit.IsValidBlockingHit())
      //{
      //   if (CanStepUp(hit) || (CharacterOwner->GetMovementBase() != nullptr && hit.HitObjectHandle == CharacterOwner->GetMovementBase()->GetOwner()))
      //   {
      //      // hit a barrier, try to step up
      //      const FVector preStepUpLocation = UpdatedComponent->GetComponentLocation();
      //      const FVector gravDir(0.f, 0.f, -1.f);
      //      if (!StepUp(gravDir, Delta * (1.f - percentTimeApplied), hit, outStepDownResult))
      //      {
      //         UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *hit.ImpactNormal.ToString(), *hit.Normal.ToString());
      //         HandleImpact(hit, lastMoveTimeSlice, rampVector);
      //         SlideAlongSurface(delta, 1.f - percentTimeApplied, hit.Normal, hit, true);
      //      }
      //      else
      //      {
      //         UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *hit.ImpactNormal.ToString(), *hit.Normal.ToString());
      //         if (!bMaintainHorizontalGroundVelocity)
      //         {
      //            // Don't recalculate velocity based on this height adjustment, if considering vertical adjustments. Only consider horizontal movement.
      //            bJustTeleported = true;
      //            const float stepUpTimeSlice = (1.f - percentTimeApplied) * DeltaSeconds;
      //            if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && StepUpTimeSlice >= UE_KINDA_SMALL_NUMBER)
      //            {
      //               Velocity = (UpdatedComponent->GetComponentLocation() - preStepUpLocation) / stepUpTimeSlice;
      //               Velocity.Z = 0;
      //            }
      //         }
      //      }
      //   }
      //   else if (hit.Component.IsValid() && !hit.Component.Get()->CanCharacterStepUp(CharacterOwner))
      //   {
      //      HandleImpact(hit, lastMoveTimeSlice, rampVector);
      //      SlideAlongSurface(delta, 1.f - percentTimeApplied, hit.Normal, hit, true);
      //   }
      //}
   }
}

void UOSECharacterMovement::GetCharacterRaycastHit(FHitResult& outHit, ECharacterRaycastLocation direction) const
{
   UWorld* world = GetWorld();
   UCapsuleComponent* characterCapsule = IsValid(OSECharOwner) ? OSECharOwner->GetCapsuleComponent() : NULL;
   if (!IsValid(UpdatedComponent) || !IsValid(world) || !IsValid(characterCapsule))
   {
      return;
   }

   outHit.Init();

   FCharacterRaycastInfo raycastInfo;
   GetCharacterRaycastInfo(raycastInfo, direction);

   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(NegCollisionTrace), false, CharacterOwner);
   FCollisionResponseParams responseParam;
   InitCollisionParams(queryParams, responseParam);
   const FVector towardsWall = -_wallClimbImpactResult.Normal * MovementCVars::NegativeCollisionTraceDistance;

   FHitResult hit(1.f);
   FVector start = UpdatedComponent->GetComponentLocation() + (raycastInfo.directionVector * raycastInfo.distanceFromCenter);
   world->LineTraceSingleByChannel(outHit, start, start + towardsWall, ECC_WorldStatic, queryParams, responseParam);
}

void UOSECharacterMovement::CheckNegativeCollision(FVector& outVelocity)
{
   UCapsuleComponent* capComp = IsValid(CharacterOwner) ? CharacterOwner->GetCapsuleComponent() : NULL;
   if (!IsValid(capComp))
   {
      return;
   }

   // We do raycasts towards the wall from the left, right, top, and bottom of the character to check for negative collisions
   FHitResult hit;
   const FOSEWallClimbSettings& settings = GetCurrentWallClimbSettings();
   if (!settings.bAllowHorizontalCornerTraversal)
   {
      GetCharacterRaycastHit(hit, RAY_Left);
      ResolveNegativeCollisionHit(hit, outVelocity, RAY_Left);

      GetCharacterRaycastHit(hit, RAY_Right);
      ResolveNegativeCollisionHit(hit, outVelocity, RAY_Right);
   }

   if (!settings.bAllowVerticalCornerTraversal)
   {
      GetCharacterRaycastHit(hit, RAY_Head);
      ResolveNegativeCollisionHit(hit, outVelocity, RAY_Head);

      GetCharacterRaycastHit(hit, RAY_Feet);
      ResolveNegativeCollisionHit(hit, outVelocity, RAY_Feet);
   }
}

void UOSECharacterMovement::ResolveNegativeCollisionHit(const FHitResult& hit, FVector& outVelocity, ECharacterRaycastLocation direction)
{
   FCharacterRaycastInfo raycastInfo;
   GetCharacterRaycastInfo(raycastInfo, direction);

   const FVector towardsWall = -_wallClimbImpactResult.Normal * MovementCVars::NegativeCollisionTraceDistance;
   FColor DebugColor = FColor::Green;

   if (!IsValidWallClimbHitResult(hit))
   {
      // We cannot move any further in this direction. Remove that direction from the character's velocity.
      RemoveDirectionFromVector(outVelocity, raycastInfo.directionVector);
      DebugColor = FColor::Red;
   }

   UWorld* world = GetWorld();
   if (IsValid(world) && IsValid(UpdatedComponent) && MovementCVars::DebugNegativeCollisions)
   {
      FVector start = UpdatedComponent->GetComponentLocation() + (raycastInfo.directionVector * raycastInfo.distanceFromCenter);
      DrawDebugLine(world, start, start + towardsWall, DebugColor);
   }
}

void UOSECharacterMovement::RemoveDirectionFromVector(FVector& outVector, const FVector& Direction)
{
   // Project velocity onto the direction we want to remove. If the projection size is positive then that direction is present in our velocity.
   float dotProduct = (outVector | Direction);
   if (dotProduct > 0.0f)
   {
      outVector -= Direction * dotProduct;
   }
}

void UOSECharacterMovement::GetCharacterRaycastInfo(FCharacterRaycastInfo& outRaycastInfo, ECharacterRaycastLocation direction) const
{
   const UCapsuleComponent* characterCapsule = IsValid(OSECharOwner) ? OSECharOwner->GetCapsuleComponent() : NULL;
   if (!IsValid(characterCapsule))
   {
      return;
   }

   switch (direction)
   {
   case ECharacterRaycastLocation::RAY_Left:
      outRaycastInfo.directionVector = -CharacterOwner->GetActorRightVector();
      outRaycastInfo.distanceFromCenter = characterCapsule->GetScaledCapsuleRadius();
      break;
   case ECharacterRaycastLocation::RAY_Right:
      outRaycastInfo.directionVector = CharacterOwner->GetActorRightVector();
      outRaycastInfo.distanceFromCenter = characterCapsule->GetScaledCapsuleRadius();
      break;
   case ECharacterRaycastLocation::RAY_Head:
      outRaycastInfo.directionVector = CharacterOwner->GetActorUpVector();
      outRaycastInfo.distanceFromCenter = characterCapsule->GetScaledCapsuleHalfHeight();
      break;
   case ECharacterRaycastLocation::RAY_Feet:
      outRaycastInfo.directionVector = -CharacterOwner->GetActorUpVector();
      outRaycastInfo.distanceFromCenter = characterCapsule->GetScaledCapsuleHalfHeight();
      break;
   }
}

FVector UOSECharacterMovement::ComputeWallMovementDelta(const FVector& delta, const FHitResult& rampHit) const
{
   if (!IsValid(UpdatedComponent))
   {
      return delta;
   }

   const FVector wallNormal = rampHit.ImpactNormal;
   const FVector contactNormal = rampHit.Normal;

   // The player always stays vertical but faces towards the wall. 
   // This is useful for getting an axis pointing away from it without regard for the angle.
   // This axis is used much like the z-axis when walking over angled surfaces.
   const FVector awayFromWall = -UpdatedComponent->GetForwardVector();
   const FVector awayFromWallProjection = wallNormal.ProjectOnTo(awayFromWall);
   
   if (awayFromWallProjection.Size() < (1.f - UE_KINDA_SMALL_NUMBER) && awayFromWallProjection.Size() > UE_KINDA_SMALL_NUMBER)
   {
      return FVector::VectorPlaneProject(delta, wallNormal);
   }

   return delta;
}

void UOSECharacterMovement::FindWall(const FVector& capsuleLocation, FFindWallResult& outWallResult, bool bCanUseCachedLocation)
{
   // No collision, no wall...
   if (!HasValidData() || !UpdatedComponent->IsQueryCollisionEnabled())
   {
      outWallResult.Clear();
      return;
   }

   // Increase height check slightly if wall climbing, to prevent wall height adjustment from later invalidating the wall result.
   const float heightCheckAdjust = (IsWallClimbing() ? MAX_WALL_DIST + UE_KINDA_SMALL_NUMBER : -MAX_WALL_DIST);

   float wallSweepTraceDist = FMath::Max(MAX_WALL_DIST, MaxStepHeight + heightCheckAdjust);
   float wallLineTraceDist = wallSweepTraceDist;
   bool bNeedToValidateWall = true;

   // Sweep Wall
   if (wallLineTraceDist > 0.f || wallSweepTraceDist > 0.f)
   {
      UOSECharacterMovement* mutableThis = const_cast<UOSECharacterMovement*>(this);

      if (bAlwaysCheckWall || !bCanUseCachedLocation || bForceNextWallCheck || bJustTeleported)
      {
         mutableThis->bForceNextWallCheck = false;
         ComputeWallDist(capsuleLocation, wallLineTraceDist, wallSweepTraceDist, outWallResult, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius());
      }
      else
      {
         // Force floor check if base has collision disabled or if it does not block us.
         UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
         const AActor* BaseActor = MovementBase ? MovementBase->GetOwner() : NULL;
         const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

         if (MovementBase != NULL)
         {
            mutableThis->bForceNextWallCheck = !MovementBase->IsQueryCollisionEnabled()
               || MovementBase->GetCollisionResponseToChannel(CollisionChannel) != ECR_Block
               || MovementBaseUtility::IsDynamicBase(MovementBase);
         }

         const bool IsActorBasePendingKill = BaseActor && !IsValid(BaseActor);

         if (!bForceNextWallCheck && !IsActorBasePendingKill && MovementBase)
         {
            outWallResult = CurrentWall;
            bNeedToValidateWall = false;
         }
         else
         {
            mutableThis->bForceNextWallCheck = false;
            ComputeWallDist(capsuleLocation, wallLineTraceDist, wallSweepTraceDist, outWallResult, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius());
         }
      }
   }

   // OutWallResult.HitResult is now the result of the horizontal wall check.
   // See if we should try to "perch" at this location.
   if (bNeedToValidateWall && outWallResult.bBlockingHit && !outWallResult.bLineTrace)
   {
      const bool bCheckRadius = true;
      if (ShouldComputePerchResult(outWallResult.HitResult, bCheckRadius))
      {
         float maxPerchWallDist = FMath::Max(MAX_WALL_DIST, MaxStepHeight + heightCheckAdjust);
         if (IsWallClimbing())
         {
            maxPerchWallDist += FMath::Max(0.f, PerchAdditionalHeight);
         }

         FFindWallResult perchWallResult;
         if (ComputePerchResult_WallClimb(GetValidPerchRadius(), outWallResult.HitResult, maxPerchWallDist, perchWallResult))
         {
            // Don't allow the wall distance adjustment to push us too far from the wall, or we will move beyond the perch distance and fall next time.
            const float avgWallDist = (MIN_WALL_DIST + MAX_WALL_DIST) * 0.5f;
            const float moveAwayDist = (avgWallDist - outWallResult.WallDist);
            if (moveAwayDist + perchWallResult.WallDist >= maxPerchWallDist)
            {
               outWallResult.WallDist = avgWallDist;
            }

            // If the regular capsule is on an unclimable surface but the perched one would allow us to climb, override the normal to be one that is climable.
            if (!outWallResult.bClimableWall)
            {
               // Wall distances are used as the distance of the regular capsule to the point of collision, to make sure AdjustWallHeight() behaves correctly.
               outWallResult.SetFromLineTrace(perchWallResult.HitResult, outWallResult.WallDist, FMath::Max(outWallResult.WallDist, MIN_WALL_DIST), true);
            }
         }
         else
         {
            // We had no wall (or an invalid one because it was unclimable), and couldn't perch here, so invalidate wall
            outWallResult.bClimableWall = false;
         }
      }
   }
}

void UOSECharacterMovement::ComputeWallDist(const FVector& capsuleLocation, float lineDistance, float sweepDistance, FFindWallResult& outWallResult, float sweepRadius) const
{
   outWallResult.Clear();

   float pawnRadius, pawnHalfHeight;
   CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(pawnRadius, pawnHalfHeight);

   // We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
   if (sweepDistance < lineDistance)
   {
      ensure(sweepDistance >= lineDistance);
      return;
   }

   bool bBlockingHit = false;
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(ComputeWallDist), false, CharacterOwner);
   FCollisionResponseParams responseParam;
   InitCollisionParams(queryParams, responseParam);
   const ECollisionChannel collisionChannel = UpdatedComponent->GetCollisionObjectType();

   // Sweep test
   if (sweepDistance > 0.f && sweepRadius > 0.f)
   {
      // Use a shorter height to avoid sweeps giving weird results if we start on a surface.
      // This also allows us to adjust out of penetrations.
      const float shrinkScale = 0.95f;
      const float shrinkScaleOverlap = 0.1f;
      float shrinkRadius = (pawnHalfHeight - pawnRadius) * (1.f - shrinkScale);
      float traceDist = sweepDistance + shrinkRadius;
      FCollisionShape capsuleShape = FCollisionShape::MakeCapsule(sweepRadius - shrinkRadius, pawnHalfHeight);

      FHitResult hit(1.f);
      bBlockingHit = WallSweepTest(hit, capsuleLocation, capsuleLocation + (UpdatedComponent->GetForwardVector() * traceDist), collisionChannel, capsuleShape, queryParams, responseParam);

      if (!Acceleration.IsNearlyZero())
      {
         FHitResult accelDirHit(1.f);
         bool bAccelDirBlockingHit = WallSweepTest(accelDirHit, capsuleLocation, capsuleLocation + (Acceleration.GetSafeNormal2D() * traceDist * 0.1f), collisionChannel, capsuleShape, queryParams, responseParam);
         if (bAccelDirBlockingHit)
         {
            bBlockingHit = bAccelDirBlockingHit;
            hit = accelDirHit;
         }
      }

      if (bBlockingHit)
      {
         // Reduce hit distance by ShrinkRadius because we shrank the capsule for the trace.
         // We allow negative distances here, because this allows us to pull out of penetrations.
         const float maxPenetrationAdjust = FMath::Max(MAX_WALL_DIST, pawnHalfHeight);
         const float sweepResult = FMath::Max(-maxPenetrationAdjust, hit.Time * traceDist - shrinkRadius);

         outWallResult.SetFromSweep(hit, sweepResult, false);
         if (hit.IsValidBlockingHit())
         {
            if (sweepResult <= sweepDistance)
            {
               // Hit within test distance.
               outWallResult.bClimableWall = true;
               return;
            }
         }
      }
   }

   // Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
   // We do however want to try a line trace if the sweep was stuck in penetration.
   if (!outWallResult.bBlockingHit && !outWallResult.HitResult.bStartPenetrating)
   {
      outWallResult.WallDist = sweepDistance;
      return;
   }

   // Line trace
   if (lineDistance > 0.f)
   {
      const float shrinkRadius = pawnRadius;
      const FVector lineTraceStart = capsuleLocation;
      const float traceDist = lineDistance + shrinkRadius;
      const FVector towardsWall = UpdatedComponent->GetForwardVector() * traceDist;
      queryParams.TraceTag = SCENE_QUERY_STAT_NAME_ONLY(WallLineTrace);

      FHitResult hit(1.f);
      bBlockingHit = GetWorld()->LineTraceSingleByChannel(hit, lineTraceStart, lineTraceStart + towardsWall, collisionChannel, queryParams, responseParam);

      if (bBlockingHit)
      {
         if (hit.Time > 0.f)
         {
            // Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
            // We allow negative distances here, because this allows us to pull out of penetrations.
            const float maxPenetrationAdjust = FMath::Max(MAX_WALL_DIST, pawnHalfHeight);
            const float lineResult = FMath::Max(-maxPenetrationAdjust, hit.Time * traceDist - shrinkRadius);

            outWallResult.bBlockingHit = true;
            if (lineResult <= lineDistance)
            {
               outWallResult.SetFromLineTrace(hit, outWallResult.WallDist, lineResult, true);
               return;
            }
         }
      }
   }

   // No hits were acceptable.
   outWallResult.bClimableWall = false;
}

bool UOSECharacterMovement::WallSweepTest(struct FHitResult& outHit, const FVector& start, const FVector& end, ECollisionChannel traceChannel, const struct FCollisionShape& collisionShape, const struct FCollisionQueryParams& params, const struct FCollisionResponseParams& responseParam) const
{
   return GetWorld()->SweepSingleByChannel(outHit, start, end, FQuat::Identity, traceChannel, collisionShape, params, responseParam);
}

FVector UOSECharacterMovement::GetWallAngleNormal() const
{
   FHitResult headHit;
   GetCharacterRaycastHit(headHit, RAY_Head);

   FHitResult feetHit;
   GetCharacterRaycastHit(feetHit, RAY_Feet);

   return (headHit.ImpactNormal + feetHit.ImpactNormal).GetSafeNormal();
}

FRotator UOSECharacterMovement::GetWallAngleNormalRotation() const
{
   return GetWallAngleNormal().ToOrientationRotator();
}

bool UOSECharacterMovement::ComputePerchResult_WallClimb(const float testRadius, const FHitResult& inHit, const float inMaxFloorDist, FFindWallResult& outPerchWallResult) const
{
   if (inMaxFloorDist <= 0.f)
   {
      return false;
   }

   // Sweep further than actual requested distance, because a reduced capsule radius means we could miss some hits that the normal radius would contact.
   float pawnRadius, pawnHalfHeight;
   CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(pawnRadius, pawnHalfHeight);
   const FVector CapsuleLocation = inHit.Location;

   const float inHitAboveBase = FMath::Max<float>(0.f, (inHit.ImpactPoint - ((UpdatedComponent->GetForwardVector() * pawnRadius) - CapsuleLocation)).Size());
   const float perchLineDist = FMath::Max(0.f, inMaxFloorDist - inHitAboveBase);
   const float perchSweepDist = FMath::Max(0.f, inMaxFloorDist);

   const float actualSweepDist = perchSweepDist + pawnRadius;
   ComputeWallDist(CapsuleLocation, perchLineDist, actualSweepDist, outPerchWallResult, testRadius);

   if (!outPerchWallResult.IsClimableWall())
   {
      return false;
   }
   else if (inHitAboveBase + outPerchWallResult.WallDist > inMaxFloorDist)
   {
      // Hit something past max distance
      outPerchWallResult.bClimableWall = false;
      return false;
   }

   return true;
}

void FFindWallResult::SetFromSweep(const FHitResult& inHit, const float inSweepWallDist, const bool bIsClimableWall)
{
   bBlockingHit = inHit.IsValidBlockingHit();
   bClimableWall = bIsClimableWall;
   bLineTrace = false;
   WallDist = inSweepWallDist;
   LineDist = 0.f;
   HitResult = inHit;
}

void FFindWallResult::SetFromLineTrace(const FHitResult& inHit, const float inSweepWallDist, const float inLineDist, const bool bIsClimableWall)
{
   // We require a sweep that hit if we are going to use a line result.
   check(HitResult.bBlockingHit);
   if (HitResult.bBlockingHit && inHit.bBlockingHit)
   {
      // Override most of the sweep result with the line result, but save some values
      FHitResult OldHit(HitResult);
      HitResult = inHit;

      // Restore some of the old values. We want the new normals and hit actor, however.
      HitResult.Time = OldHit.Time;
      HitResult.ImpactPoint = OldHit.ImpactPoint;
      HitResult.Location = OldHit.Location;
      HitResult.TraceStart = OldHit.TraceStart;
      HitResult.TraceEnd = OldHit.TraceEnd;

      bLineTrace = true;
      WallDist = inSweepWallDist;
      LineDist = inLineDist;
      bClimableWall = bIsClimableWall;
   }
}

bool UOSECharacterMovement::CanCrouchInCurrentState() const
{
   if (!CanEverCrouch())
   {
      return false;
   }

   return (IsFalling() || IsMovingOnGround() || (IsMantling() && OSECharOwner->GetMantleState().IsCrouching)) && !IsWallClimbing() && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

void UOSECharacterMovement::WallClimbDashFinished()
{
   UWorld* world = GetWorld();
   if (IsValid(world))
   {
      world->GetTimerManager().ClearTimer(_wallDashTimer);
   }

   _bIsWallDashing = false;
   bForceMaxAccel = false;

   OSECharOwner->OnStopWallDash();
}

bool UOSECharacterMovement::_CanWallClimbOnSurface(const FHitResult& impact) const
{
   if (!_allowWallClimbOnlyOnSomeSurfaces)
   {
      return true;
   }

   if (UPhysicalMaterial* physMat = impact.PhysMaterial.Get())
   {
      if (_wallClimbableSurfaces.Contains(physMat->SurfaceType))
      {
         return true;
      }
   }

   return false;
}

//---------------------------------------------------------------------------------------
// Dodge
//---------------------------------------------------------------------------------------

bool UOSECharacterMovement::CanEverDodge() const
{
   return GetDodgeEnabled();
}

bool UOSECharacterMovement::CanDodgeInCurrentState() const
{
   return CanEverDodge() && IsMovingOnGround() && !IsCrouching() && !Acceleration.IsZero();
}

FVector UOSECharacterMovement::GetDodgeDirection(bool& outIsStationaryDodge) const
{
   FVector moveDirection = Acceleration;

   // if player is stationary when dodging, then use default no-input dodge direction
   outIsStationaryDodge = moveDirection.SizeSquared2D() < FMath::Square(_dodgeMinimumAccelMagnitude);
   if (outIsStationaryDodge)
   {
      // Transform direction to be relative to camera-forward
      FRotator viewRot;
      FVector viewLoc;
      GetCharacterOwner()->GetActorEyesViewPoint(viewLoc, viewRot);
      const FTransform characterViewTransform(viewRot, viewLoc, FVector::OneVector);
      moveDirection = characterViewTransform.TransformVectorNoScale(_dodgeDirectionStationary);
   }

   return MakeDodgeDirectionFromVector(moveDirection);
}

FVector UOSECharacterMovement::MakeDodgeDirectionFromVector(FVector moveDirection) const
{
   // If dodging down a slanted floor, project the movement direction onto the floor
   // Do not do this when going up a ramp, as this could cause excess vertical momentum if reaching the top
   // NOTE: This does not prevent going airborne if dodging from the top of the landing, so more substantive
   //       changes may be required if that is an issue.
   if (CurrentFloor.IsWalkableFloor())
   {
      const FVector floorNormal = CurrentFloor.HitResult.Normal;
      FVector projectedMoveDirection = FVector::VectorPlaneProject(moveDirection, floorNormal);
      if (projectedMoveDirection.Z < 0)
      {
         moveDirection = projectedMoveDirection;
      }
      else
      {
         moveDirection.Z = 0;
      }
   }
   else
   {
      moveDirection.Z = 0;
   }

   return moveDirection.GetSafeNormal();
}

//FVector UOSECharacterMovement::GetDodgeDirection() const
//{
//   FVector moveDirection = Acceleration;
//
//   // if we have no movement direction, then allow that (TODO: configure, if there is a use-case?)
//
//   // If dodging down a slanted floor, project the movement direction onto the floor
//   // Do not do this when going up a ramp, as this could cause excess vertical momentum if reaching the top
//   // NOTE: This does not prevent going airborne if dodging from the top of the landing, so more substantive
//   //       changes may be required if that is an issue.
//   if (CurrentFloor.IsWalkableFloor())
//   {
//      const FVector floorNormal = CurrentFloor.HitResult.Normal;
//      FVector projectedMoveDirection = FVector::VectorPlaneProject(moveDirection, floorNormal);
//      if (projectedMoveDirection.Z < 0)
//      {
//         moveDirection = projectedMoveDirection;
//      }
//      else
//      {
//         moveDirection.Z = 0;
//      }
//   }
//   else
//   {
//      moveDirection.Z = 0;
//   }
//
//   return moveDirection.GetSafeNormal();
//}

float UOSECharacterMovement::CheckCeilingClearance(float maxHeight, float radiusShrinkAmount /* = 0.0f */) const
{
   FCollisionQueryParams capsuleParams(SCENE_QUERY_STAT(CheckCeilingClearance), false, CharacterOwner);
   FCollisionResponseParams responseParam;
   InitCollisionParams(capsuleParams, responseParam);

   const FCollisionShape capsuleShape = GetPawnCapsuleCollisionShape(SHRINK_RadiusCustom, radiusShrinkAmount);
   const ECollisionChannel collisionChannel = UpdatedComponent->GetCollisionObjectType();

   const float traceDistance = capsuleShape.GetCapsuleHalfHeight() + maxHeight;
   const FVector startLocation = UpdatedComponent->GetComponentLocation();
   const FVector endLocation = startLocation + (FVector::UpVector * traceDistance);

   FHitResult hitResult(1.0f);
   GetWorld()->SweepSingleByChannel(hitResult, startLocation, endLocation, FQuat::Identity, collisionChannel, capsuleShape, capsuleParams, responseParam);
   if (hitResult.bBlockingHit)
   {
      return hitResult.Distance;
   }

   return traceDistance;
}

bool UOSECharacterMovement::CheckForwardScrambleClearance(float castDistance, float radiusShrinkAmount /* = 0.0f */) const
{
   const FOSEScrambleSettings& settings = GetCurrentScrambleSettings();

   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(CheckForwardClearance), false, CharacterOwner);
   FCollisionResponseParams responseParam;
   InitCollisionParams(queryParams, responseParam);

   // Get the current capsule size. This can change, and will be different when crouched.
   float capsuleRadius, capsuleHalfHeight;
   CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(capsuleRadius, capsuleHalfHeight);
   const float capsuleHalfHeightWithoutHemisphere = capsuleHalfHeight - capsuleRadius;

   const float collisionRadius = FMath::Max(1.0f, capsuleRadius - radiusShrinkAmount);
   const FCollisionShape collisionShape = FCollisionShape::MakeSphere(collisionRadius);
   const ECollisionChannel collisionChannel = UpdatedComponent->GetCollisionObjectType();

   const FVector componentLocation = UpdatedComponent->GetComponentLocation();
   const FRotator componentRotation = UpdatedComponent->GetComponentRotation();
   const FVector componentDirection = componentRotation.Vector();

   // Forward clearance consists of three sweeps:
   // 
   // First, we sweep a sphere forward from the top of the capsule
   // to ensure we have a surface directly in front of us to climb.
   // 
   // Next, we approximate how far up the surface we aim the camera,
   // and ensure it's clear of obstructions directly above us.
   // 
   // Finally, we sweep forward from the previous position to ensure
   // we have a surface to climb.
   {
      // Including radius shrink amount to ensure distance is relative to the edge of the capsule
      const float forwardCastAmount = castDistance + radiusShrinkAmount;

      const FVector capsuleTop = componentLocation + (FVector::UpVector * capsuleHalfHeightWithoutHemisphere);
      const FVector capsuleFwd = capsuleTop + (componentDirection * forwardCastAmount);

      // Sweep forward from top of capsule
      FHitResult hitResultFwd;
      if (GetWorld()->SweepSingleByChannel(hitResultFwd, capsuleTop, capsuleFwd, FQuat::Identity, collisionChannel, collisionShape, queryParams, responseParam))
      {
         // Height approximation assumes we're aiming up from the rear of the capsule.
         // This is not necessarily accurate, but gives us the most vertical distance. We
         // also offset the height based on our start boost (if any), and how much we'd
         // travel while waiting for another valid hit.
         const float capsuleDiameter = capsuleRadius * 2.0f;
         const float cameraFocusHeight = capsuleDiameter * FMath::Tan(FMath::DegreesToRadians(settings.MaxCameraPitch));
         const float totalHeightOffset = (settings.ScrambleSpeed * settings.ExpirationDelay) + cameraFocusHeight + settings.StartBoostHeight;

         const FVector capsuleTopCam = capsuleTop + (FVector::UpVector * totalHeightOffset);
         const FVector capsuleFwdCam = capsuleFwd + (FVector::UpVector * totalHeightOffset);

         // @TODO: Revisit these checks. The vertical sweep is NOT necessarily clear, and can register as
         // an impact in certain situations where the roof angle slopes in, but is within our expected threshold.

         // Sweep up from top of capsule. This sweep is expected to be clear (no blocking hit)
         FHitResult hitResultUp;
         if (!GetWorld()->SweepSingleByChannel(hitResultUp, capsuleTop, capsuleTopCam, FQuat::Identity, collisionChannel, collisionShape, queryParams, responseParam))
         {
            // Sweep forward from clear height position, ensuring we are blocked
            FHitResult hitResultCam;
            if (GetWorld()->SweepSingleByChannel(hitResultCam, capsuleTopCam, capsuleFwdCam, FQuat::Identity, collisionChannel, collisionShape, queryParams, responseParam))
            {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
               static const auto consoleVarVisualizeMovement = IConsoleManager::Get().FindConsoleVariable(TEXT("p.VisualizeMovement"));
               if ((consoleVarVisualizeMovement != nullptr) && consoleVarVisualizeMovement->GetBool())
               {
                  DrawDebugSphere(GetWorld(), hitResultFwd.Location, collisionShape.GetSphereRadius(), 12, FColor::Blue, false, -1.0f, 255);
                  DrawDebugSphere(GetWorld(), hitResultUp.TraceEnd, collisionShape.GetSphereRadius(), 12, FColor::Cyan, false, -1.0f, 255);
                  DrawDebugSphere(GetWorld(), hitResultCam.Location, collisionShape.GetSphereRadius(), 12, FColor::Green, false, -1.0f, 255);
               }
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

               // Success
               return true;
            }
         }
      }
   }

   return false;
}


//---------------------------------------------------------------------------------------
// Delta rotation
//---------------------------------------------------------------------------------------

// Returns how far to rotate character during the time interval DeltaTime
FRotator UOSECharacterMovement::GetDeltaRotation(float deltaTime) const
{
   // While on the ledge or wall climbing, don't support rotation from input.
   if(IsInLedgeState() || IsWallClimbing())
      return FRotator::ZeroRotator;

   // Check if movement allows turn-in-place
   const bool movementAllowsTurnInPlace =
      !SlidingEnabled || (SlidingSettings.AllowTurnInPlace || !IsSliding());

   // Fall back to default behavior if turn-in-place is not enabled or active.
   // Turn-in-place can only be activated when enabled, but can remain active
   // for a time while disabled.
   const bool useTurnInPlace = (GetTurnInPlaceEnabled() || IsTurnInPlaceActive())
      && bUseControllerDesiredRotation
      && !PawnOwner->bUseControllerRotationYaw
      && !IsMoveInputIgnored()
      && movementAllowsTurnInPlace;

   const FRotator defaultDeltaRotation = Super::GetDeltaRotation(deltaTime);
   if (!useTurnInPlace)
      return defaultDeltaRotation;

   // The delta rotation is rate * time. We just want the rate, so need to undo
   // the time. We could simply read the property value, but this way is better:
   // if a base class is modifying the rotation (like we are) both of them
   // will still work as expected!
   const FRotator defaultRotationRate = defaultDeltaRotation * (1.0f / deltaTime);
   FRotator modifiedRotationRate = defaultRotationRate;

   // Turn-in-place rotation rates are always applied while enabled. Rates are
   // calculated for speed even when inactive, for example.
   {
      const FOSETurnInPlaceState& turnInPlaceState = GetTurnInPlaceState();
      modifiedRotationRate.Yaw = FMath::Max(modifiedRotationRate.Yaw, turnInPlaceState.RotationRate);
   }

   // We've modified our rotation rate. Now we simply need to re-apply the delta time and return the result.
   return modifiedRotationRate * deltaTime;
}


//---------------------------------------------------------------------------------------
// Turn-in-place
//---------------------------------------------------------------------------------------

float UOSECharacterMovement::GetArcLengthRotationRate(float speed, float radius)
{
   // Using the radius we compute the rotation rate in degrees per second
   // such that the arc length is equal to the distance traveled at that speed
   speed = FMath::Max(0.0f, speed);
   radius = FMath::Max(1.0f, radius);
   return FMath::RadiansToDegrees(speed / radius);
}

float UOSECharacterMovement::GetArcLengthRotationRate(float speed) const
{
   // Use the scaled capsule radius
   check(CharacterOwner != nullptr);
   const float radius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
   return GetArcLengthRotationRate(speed, radius);
}

void UOSECharacterMovement::UpdateTurnInPlace(float deltaSeconds)
{
   // Turn-in-place is *always* updated. However, it is only _activated_ when it is enabled.
   // This allows turn-in-place to complete if it is turned off at run-time.
   
   // Copy the state for comparison against previous state, and update the new local value
   // before assigning it to the state member.
   const FOSETurnInPlaceState prevState = _turnInPlaceState;
   FOSETurnInPlaceState currState = prevState;

   // We always initialize the rotation rate based on current velocity
   currState.RotationRate = GetArcLengthRotationRate(Velocity.Size());

   // Aim delta
   {
      // We use view rotation (control rotation) as opposed to base aim rotation
      // (camera rotation) as it will not include any camera modifications, and thus
      // have a much more consistent result.
      // 
      // Normalizing converts it to -180..+180 range, so we can compare values
      // against our thresholds consistently.
      const FRotator aimRotation = PawnOwner->GetViewRotation().GetNormalized();

      // Aim rotation relative to where the actor is oriented
      const FRotator aimDelta = (aimRotation - PawnOwner->GetActorRotation()).GetNormalized();

      // Store the yaw delta abs and sign values
      currState.YawDeltaAbs = FMath::Abs(aimDelta.Yaw);
      currState.YawDeltaSign = FMath::Sign(aimDelta.Yaw);
   }

   // Turn-in-place (angle thresholds)
   if (currState.IsActive)
   {
      // Still active, update duration
      currState.ActiveDuration += deltaSeconds;

      // The default rotation rate for turn-in-place is intentionally constant.
      // We use a constant speed value to derive this, using our capsule radius sizes
      // to provide an arc length to travel.
      {
         const float speedValueWalk = MaxWalkSpeed * MovementCVars::TurnInPlaceRateWalkSpeedScalar;
         const float speedValueCrouch = MaxWalkSpeedCrouched * MovementCVars::TurnInPlaceRateCrouchSpeedScalar;
         const float speedValueToUse = FMath::Min(speedValueWalk, speedValueCrouch);

         // Now compute and quantize the rotation rate
         const float rotationRateRaw = GetArcLengthRotationRate(speedValueToUse);
         const float rotationRateQuantized = int32(rotationRateRaw / MovementCVars::TurnInPlaceRateQuantization) * MovementCVars::TurnInPlaceRateQuantization;
         currState.RotationRate = FMath::Max(currState.RotationRate, rotationRateQuantized);
      }

      // Check if we've centered on our actor rotation, or if our direction changed
      const bool checkDirection = MovementCVars::TurnInPlaceStopOnDirectionChange > 0;
      const bool isNewDirection = checkDirection && ((currState.YawDeltaSign * prevState.YawDeltaSign) < 0.0f);
      const bool isCentered = (currState.YawDeltaAbs <= MovementCVars::TurnInPlaceCenteredThreshold);

      if (isCentered || isNewDirection)
      {
         // We're within tolerance, reset the turn-in-place data
         currState.IsActive = false;
         currState.StartTimer = 0.0f;
      }
   }
   else
   {
      const FOSETurnInPlaceSettings& settings = GetTurnInPlaceSettings();

      // We're not turning in place yet. Map our current angle delta to the min/max angle range.
      // We intentionally do not clamp the value. It will be negative after interpolation when
      // it's less than the min angle.
      const float pctRange = FMath::GetRangePct(settings.AngleDeltaMin, settings.AngleDeltaMax, currState.YawDeltaAbs);
      const bool exceedsMinAngle = (pctRange >= 0.0f);

      // Continue if turn-in-place is enabled and we've exceeded the min angle.
      if (GetTurnInPlaceEnabled() && exceedsMinAngle)
      {
         // Remaps 0..1 to 0..1 with cosine fall off (tends to stay closer to the minimum).
         // Make sure we don't remap beyond PI, as it will then decrease.
         const float cosRatio = 1.0f - FMath::Cos(FMath::Min(2.0f, pctRange) * HALF_PI);
         const float timeLimit = FMath::Max(deltaSeconds, FMath::Lerp(settings.TimeLimitMin, settings.TimeLimitMax, cosRatio));

         // We've established the limit based on our current angle.
         // Accumulate time and activate turn-in-place as soon as we've exceeded it.
         currState.StartTimer += deltaSeconds;
         if (currState.StartTimer > timeLimit)
         {
            currState.IsActive = true;
            currState.ActiveDuration = 0.0f;
         }
      }
      else
      {
         // If we're not enabled, or if we're not facing beyond the minimum threshold,
         // then we start releasing any time we might have accumulated up to this point.
         const float decayAmount = deltaSeconds * MovementCVars::TurnInPlaceTimeDecayScalar;
         currState.StartTimer = FMath::Max(0.0f, currState.StartTimer - decayAmount);
      }
   }

   // Finally, store the updated state
   _turnInPlaceState = currState;
}


//---------------------------------------------------------------------------------------
// Distance constraints
//---------------------------------------------------------------------------------------

void UOSECharacterMovement::SetDistanceConstraint(const FOSEDistanceConstraint& distanceConstraint)
{
   // New value is always assigned to desired value; the current value is also updated if the
   // enable / disable state changed.

   if (distanceConstraint.Enabled != _distanceConstraintDesired.Enabled)
   {
      _affectedByDistanceConstraint = false;
   }

   _lastDistanceConstraintDelta = FMath::Max(_lastDistanceConstraintDelta, distanceConstraint.GetDeltaTo(_distanceConstraintDesired));
   _distanceConstraintDesired = distanceConstraint;
}

void UOSECharacterMovement::_RelaxDistanceConstraintIfNeeded()
{
   const FOSEDistanceConstraint& distConst = GetDistanceConstraint();
   if (distConst.Enabled)
   {
      // Current location
      const FVector currentLocation = UpdatedComponent->GetComponentLocation();
      const FVector currentToOrigin = distConst.Position - currentLocation;

      const float distanceSquared = currentToOrigin.SizeSquared();
      if(distanceSquared > FMath::Square(distConst.Distance))
      {
         const float currentToDesired = FVector::Distance(_distanceConstraintDesired.Position, currentLocation);
         const float desiredNewLength = FMath::Max(currentToDesired, _distanceConstraintDesired.Distance);
         _distanceConstraintDesired.ForceSlack(desiredNewLength);
      }
   }
}

void UOSECharacterMovement::_UpdateDistanceConstraintControls(float deltaSeconds)
{
   if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
   {
      if (OSECharOwner)
      {
         // Update replicated distance constraint so that it will be replicated to simulated proxies
         // Only matters when updating the slack amount, otherwise it is essentially a noop
         OSECharOwner->SetDistanceConstraintRaw(_distanceConstraintDesired);

         OSECharOwner->SetExtendingDistanceConstraint(_extendingDistanceConstraint);
         OSECharOwner->SetContractingDistanceConstraint(_wantsToContractDistanceConstraint);
      }
   }
}


bool UOSECharacterMovement::_FindFloorDistanceWhenFalling(float& outFloorDistance) const
{
   if (MovementMode != MOVE_Falling)
   {
      UE_LOG(LogOSECharacterMovement, Error, TEXT("_FindFloorDistanceWhenFalling called when not falling"));
      return false;
   }

   const ECollisionChannel collisionChannel = UpdatedComponent->GetCollisionObjectType();
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(FindFloorDistanceWhenFalling), false, CharacterOwner);
   FCollisionResponseParams responseParam;
   InitCollisionParams(queryParams, responseParam);

   const FCollisionShape capsuleShape = CharacterOwner->GetCapsuleComponent()->GetCollisionShape();

   const FVector traceStart = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();
   const FVector traceEnd = traceStart + FVector(0.f, 0.f, -FallingFindFloorMaxDistance);

   FHitResult hit(1.f);
   const bool wasBlockingHit = GetWorld()->SweepSingleByChannel(hit, traceStart, traceEnd, FQuat::Identity, collisionChannel, capsuleShape, queryParams, responseParam);

   if (wasBlockingHit)
   {
      const float sweepResult = FMath::Max(0.0f, hit.Distance);
      outFloorDistance = sweepResult;
      return true;
   }
   else
   {
      return false;
   }
}

void UOSECharacterMovement::_UpdateCustomCharacterStateOnMovementNone()
{
   if (!MovementCVars::LedgeEnableLedgeMantleCheck)
   {
      UpdateMantleState();
   }
   else
   {
      UpdateLedgeState();
   }

   UpdateSprintState();

   UpdateSlideState();

   const float deltaSeconds = 0.0f;
   UpdateScrambleState(deltaSeconds);
}

void UOSECharacterMovement::_ComputeFrictionAndBrakingDeceleration(float& inOutFriction, float& inOutBrakingDeceleration) const
{
   if (IsMovingOnGround())
   {
      inOutFriction *= _frictionMultiplier;
      inOutBrakingDeceleration *= _brakingDecelerationMultiplier;
   }

   if (IsMovingOnGround() && IsSliding())
   {
      inOutFriction = inOutFriction * SlidingSettings.FrictionFactor;

      if (CurrentFloor.IsWalkableFloor())
      {
         // Reduce or increase friction based on the slope we're on
         const FVector PawnVector = PawnOwner->GetActorRotation().Vector().GetSafeNormal2D();
         const FVector RampVector = ComputeGroundMovementDelta(PawnVector, CurrentFloor.HitResult, CurrentFloor.bLineTrace).GetSafeNormal();
         inOutFriction *= (RampVector.Z + 1.0f);
      }
   }
}

bool UOSECharacterMovement::GetDistanceToFloorWhenFalling(float& outFloorDistance, float& outMaxFloorDistanceThisFall) const
{
   if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
   {
      UE_LOG(LogOSECharacterMovement, Error,
         TEXT("UOSECharacterMovement::GetDistanceToFloorWhenFalling called on simulated proxy '%s', the information is not valid there"),
         *CharacterOwner->GetName());
      return false;
   }

   if (!ShouldFindFloorWhenFalling)
   {
      UE_LOG(LogOSECharacterMovement, Error,
         TEXT("UOSECharacterMovement::GetDistanceToFloorWhenFalling called on '%s' which has ShouldFindFloorWhenFalling set to false"),
         *CharacterOwner->GetName());
      return false;
   }

   if (MovementMode == MOVE_Falling)
   {
      outFloorDistance = _fallingFloorDistance;
      outMaxFloorDistanceThisFall = _fallingMaxFloorDistanceThisFall;
      return true;
   }

   return false;
}

FVector UOSECharacterMovement::GetAirControl(float deltaTime, float tickAirControl, const FVector& fallAcceleration)
{
   // Use separately configurable air control values when swinging
   // If _affectedByDistanceConstraint becomes a continuous value, can interpolate
   if (_affectedByDistanceConstraint)
   {
      tickAirControl = SwingAirControl;
      // Allow a burst of initial acceleration
      // TODO: adjust to not boost during tops of swings that have slowed down?
      if (SwingAirControlBoostMultiplier > 0.f && Velocity.SizeSquared2D() < FMath::Square(SwingAirControlBoostVelocityThreshold))
      {
         tickAirControl = FMath::Min(1.f, SwingAirControlBoostMultiplier * tickAirControl);
      }
      return tickAirControl * fallAcceleration;
   }

   return Super::GetAirControl(deltaTime, tickAirControl, fallAcceleration);
}

// @TODO: This is a temporary, non-ideal spot for the distance constraint update. Better to have it in MoveComponent,
// ResolvePenetration, and associated methods. Useful to be here for now for testing.
void UOSECharacterMovement::OnMovementUpdated(float deltaSeconds, const FVector& oldLocation, const FVector& oldVelocity)
{
   Super::OnMovementUpdated(deltaSeconds, oldLocation, oldVelocity);

   if (IsMovingOnGround())
   {
      _cachedMovementMaterialHitResult = CurrentFloor.HitResult;
   }

   const FOSEDistanceConstraint& distConst = GetDistanceConstraint();
   if (distConst.Enabled)
   {
      // Current location
      const FVector currentLocation = UpdatedComponent->GetComponentLocation();
      const FVector currentToOrigin = distConst.Position - currentLocation;

      const float distanceSquared = currentToOrigin.SizeSquared();
      float maxDistSqr = FMath::Max(0.0f, (distConst.Distance * distConst.Distance) - 1.0f);

      UE_VLOG_HISTOGRAM(GetOwner(), LogOSECharacterMovement, Verbose, "DistanceContraints", "Z", FVector2D(GetWorld()->GetTimeSeconds(), currentLocation.Z));
      UE_VLOG_HISTOGRAM(GetOwner(), LogOSECharacterMovement, Verbose, "DistanceContraints", "Distance", FVector2D(GetWorld()->GetTimeSeconds(), distConst.Distance));
      UE_VLOG_HISTOGRAM(GetOwner(), LogOSECharacterMovement, Verbose, "DistanceContraints", "Slack", FVector2D(GetWorld()->GetTimeSeconds(), distConst.SlackAmount));
      UE_VLOG_HISTOGRAM(GetOwner(), LogOSECharacterMovement, Verbose, "DistanceContraints", "ExcessDistance", FVector2D(GetWorld()->GetTimeSeconds(), FMath::Sqrt(maxDistSqr - distanceSquared)));

      float slackExpansion = 0;
      bool skipVelocityChange = false;

      const auto updateVelocity = [this](const FVector& planeNormal, float slackExpansion, float deltaSeconds) -> FVector {
         FVector newVelocity = FVector::VectorPlaneProject(Velocity, planeNormal);
         const float brakingDecel = _distanceConstraintDesired.BrakingDeceleration;
         const FVector reverseAccel = -brakingDecel * newVelocity.GetSafeNormal();
         newVelocity = newVelocity + (reverseAccel * deltaSeconds);

         // Allow movement away from anchor at tension as much as the current rate of slack expansion
         newVelocity -= planeNormal * (slackExpansion / deltaSeconds);
         return newVelocity;
      };
      const auto moveOrSlideAlong = [this](const FVector& deltaPosition, float deltaSeconds) -> void {
         FHitResult moveHit(1.0f);
         SafeMoveUpdatedComponent(deltaPosition, UpdatedComponent->GetComponentQuat(), true, moveHit);

         if (moveHit.Time < 1.0f)
         {
            // We hit something; adjust and move again
            HandleImpact(moveHit, deltaSeconds, deltaPosition);
            // use base-class slide to side-step constraints in walk mode
            UMovementComponent::SlideAlongSurface(deltaPosition, (1.f - moveHit.Time), moveHit.Normal, moveHit, true);
         }
      };

      if (distanceSquared > maxDistSqr && _extendingDistanceConstraint)
      {
         const auto surface = IsMovingOnGround() ? EOSEDistanceConstraintSurface::Ground : EOSEDistanceConstraintSurface::Air;
         const float slackVelocity = _distanceConstraintDesired.SlackVelocityForSurface(surface);
         const float desiredExpansion = FMath::Sqrt(distanceSquared - maxDistSqr);
         slackExpansion = _distanceConstraintDesired.IncreaseSlack(slackVelocity, deltaSeconds, desiredExpansion);
         maxDistSqr = FMath::Max(0.0f, (distConst.Distance * distConst.Distance) - 1.0f);
         UE_VLOG_HISTOGRAM(GetOwner(), LogOSECharacterMovement, Verbose, "DistanceContraints", "AdjustingSlack", FVector2D(GetWorld()->GetTimeSeconds(), 100));
      }
      // Skip simulating the climb on simulated proxies so that it does not get out of sync with the replicated distance constraint
      else if (_wantsToContractDistanceConstraint && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
      {
         // when reducing the distance constraint, use up any current excess slack
         const float nominalChange = -deltaSeconds * distConst.ContractSpeed;
         const float oldToDesired = FVector::Distance(distConst.Position, oldLocation);
         const float desiredNewLength = distConst.ClampSlack(FMath::Min(oldToDesired, _distanceConstraintDesired.Distance) + nominalChange);

         const FVector planeNormal = currentToOrigin.GetSafeNormal();
         const FVector planeOrigin = distConst.Position - (planeNormal * desiredNewLength);

         const FVector newLocation = FVector::PointPlaneProject(currentLocation, planeOrigin, planeNormal);
         const FVector deltaPosition = newLocation - currentLocation;
         moveOrSlideAlong(deltaPosition, deltaSeconds);

         // new length accounts for actual contraction
         const FVector newCurrentToOrigin = distConst.Position - UpdatedComponent->GetComponentLocation();
         const float delta = _distanceConstraintDesired.ForceSlack(newCurrentToOrigin.Size());
         slackExpansion = FMath::Clamp(nominalChange, delta, 0.f);
         Velocity = updateVelocity(planeNormal, slackExpansion, deltaSeconds);

         maxDistSqr = FMath::Max(0.0f, (distConst.Distance * distConst.Distance) - 1.0f);
         skipVelocityChange = true;
      }

      if (distanceSquared > maxDistSqr && !skipVelocityChange)
      {
         const FVector planeNormal = currentToOrigin.GetSafeNormal();
         const FVector planeOrigin = distConst.Position - (planeNormal * distConst.Distance);

         const FVector newLocation = FVector::PointPlaneProject(currentLocation, planeOrigin, planeNormal);
         const FVector deltaPosition = newLocation - currentLocation;
         Velocity = updateVelocity(planeNormal, slackExpansion, deltaSeconds);
         moveOrSlideAlong(deltaPosition, deltaSeconds);
      }

      if (slackExpansion != 0 && MovementCVars::DistanceConstraintClientForcePartialUpdates)
      {
         _lastDistanceConstraintDelta = EDistanceConstraintDelta::Partial;
      }

      // Apply hysteresis to _affectedByDistanceConstraint: exits state farther than distance to enter it
      // thresholds can be converted to variables if there is a use-case for actually changing them
      constexpr float kEnterTensionDistance = 1;
      constexpr float kExitTensionDistance = 10;
      if (_affectedByDistanceConstraint && distanceSquared < FMath::Square(distConst.Distance - kExitTensionDistance))
      {
         _affectedByDistanceConstraint = false;
      }
      else if (!_affectedByDistanceConstraint && distanceSquared > FMath::Square(distConst.Distance - kEnterTensionDistance))
      {
         _affectedByDistanceConstraint = true;
      }

      // Since the simulated proxy can lag, assume at tension if climbing up the distance constraint
      if (_wantsToContractDistanceConstraint && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
      {
         _affectedByDistanceConstraint = true;
      }
   }
   else
   {
      // Distance constraint is disabled, clearly not affected
      _affectedByDistanceConstraint = false;
   }

   _UpdateDistanceConstraintControls(deltaSeconds);

   // If we're falling and we want to check our floor distance, do that here
   // Don't bother doing this on simulated proxies
   if (ShouldFindFloorWhenFalling && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy))
   {
      if (MovementMode == MOVE_Falling)
      {
         if (_FindFloorDistanceWhenFalling(_fallingFloorDistance))
         {
            _fallingMaxFloorDistanceThisFall = FMath::Max(_fallingMaxFloorDistanceThisFall, _fallingFloorDistance);
         }
      }
      else
      {
         _fallingMaxFloorDistanceThisFall = 0.0f;
      }
   }
}

void UOSECharacterMovement::UpdatePhysicalMaterialContext()
{
   FOSEMovementMaterialContext newContext;
   PopulatePhysicalMaterialContext(newContext);
   
   // Check for any changes since the last time we checked, and if so fire the delegate
   if (newContext != _currentMovementMaterialContext)
   {
      OnPhysicalMaterialChanged.Broadcast(newContext, _currentMovementMaterialContext);
      _currentMovementMaterialContext = newContext;
   }
}

void UOSECharacterMovement::PopulatePhysicalMaterialContext(FOSEMovementMaterialContext& newContext) const
{
   // Stash all the flags
   newContext.IsMovingOnGround = IsMovingOnGround();
   newContext.IsInAir = IsFalling() || IsFlying();
   newContext.IsScrambling = IsScrambling();
   newContext.IsClimbing = IsWallClimbing();
   newContext.IsMantling = IsMantling();
   newContext.IsCrouching = IsCrouching();
   newContext.IsSprinting = IsSprinting();
   newContext.IsSliding = IsSliding();

   if (newContext.IsMantling)
   {
      newContext.PhysicalMaterial = OSECharOwner->GetMantleState().StartPhysicalMaterial;
   }
   else
   {
      // Retrieve the physical material from the updated cached movement material hit result
      newContext.PhysicalMaterial = UOSECommon::GetPhysicalMaterialFromHitResult(_cachedMovementMaterialHitResult);
   }
}

void UOSECharacterMovement::SetPostLandedPhysics(const FHitResult& Hit)
{
   Super::SetPostLandedPhysics(Hit);

   ManuallyCanceledWallClimbCooldownFinished();
   _bManuallyCanceledWallClimb = false;

   // We just landed, use the incoming hit result for the next update of the movement material context
   if (_currentMovementMaterialContext.IsInAir)
   {
      _cachedMovementMaterialHitResult = Hit;
   }
}

FRotator UOSECharacterMovement::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
   if (_maxSpeedMultiplier == 0.0f)
   {
      return CurrentRotation;
   }

   return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}
