// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// OSE
#include "Character/OSECharacterMovementTypes.h"
#include "Character/OSECharacterMovementReplication.h"
#include "Character/OSECharacterRotation.h"
#include "Traversal/OSEDistanceConstraint.h"
#include "Traversal/Scrambling/OSEScrambleSettings.h"
#include "Traversal/WallClimb/OSEWallClimbSettings.h"
#include "Traversal/Jumping/OSEJumpSettings.h"
#include "Traversal/Mantle/OSEMantleLocation.h"
#include "Traversal/Mantle/OSELedgeState.h"
#include "Traversal/Mantle/OSELedgeSettings.h"
#include "Traversal/Sliding/OSESlidingSettings.h"

// UE4
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "OSECharacterMovement.generated.h"

/// Delegate for when the physical material / state changes during movement
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOSEOnPhysicalMaterialChanged, const FOSEMovementMaterialContext&, CurrentContext, const FOSEMovementMaterialContext&, PreviousContext);

struct FOSELedgeQueryResult;

/** Raycast locations on the character */
enum ECharacterRaycastLocation
{
   RAY_Left,
   RAY_Right,
   RAY_Head,
   RAY_Feet
};

struct FCharacterRaycastInfo
{
   FVector directionVector;
   float distanceFromCenter = 0.0f;
};

/** Data about the wall for wall climb movement */
USTRUCT(BlueprintType)
struct FFindWallResult
{
	GENERATED_USTRUCT_BODY()

	/**
	* True if there was a blocking hit in the wall test that was NOT in initial penetration.
	* The HitResult can give more info about other circumstances.
	*/
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	uint32 bBlockingHit : 1;

	/** True if the hit found a valid climable wall. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	uint32 bClimableWall : 1;

	/** True if the hit found a valid climable wall using a line trace (rather than a sweep test, which happens when the sweep test fails to yield a climable wall). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	uint32 bLineTrace : 1;

	/** The distance to the wall, computed from the swept capsule trace. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	float WallDist;

	/** The distance to the wall, computed from the trace. Only valid if bLineTrace is true. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	float LineDist;

	/** Hit result of the test that found a wall. Includes more specific data about the point of impact and surface normal at that point. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CharacterWall)
	FHitResult HitResult;

public:

   FFindWallResult()
		: bBlockingHit(false)
		, bClimableWall(false)
		, bLineTrace(false)
		, WallDist(0.f)
		, LineDist(0.f)
		, HitResult(1.f)
	{
	}

	/** Returns true if the wall result hit a climbable surface. */
	bool IsClimableWall() const
	{
		return bBlockingHit;
	}

	void Clear()
	{
		bBlockingHit = false;
      bClimableWall = false;
		bLineTrace = false;
      WallDist = 0.f;
		LineDist = 0.f;
		HitResult.Reset(1.f, false);
	}

	/** Gets the distance to wall, either LineDist or FloorDist. */
	float GetDistanceToWall() const
	{
		// When the wall distance is set using SetFromSweep, the LineDist value will be reset.
		// However, when SetLineFromTrace is used, there's no guarantee that WallDist is set.
		return bLineTrace ? LineDist : WallDist;
	}

	void SetFromSweep(const FHitResult& inHit, const float inSweepWallDist, const bool bIsClimableWall);
	void SetFromLineTrace(const FHitResult& inHit, const float inSweepWallDist, const float inLineDist, const bool bIsClimableWall);
};

//---------------------------------------------------------------------------------------
/// Saved move for client side prediction, server validation, and replays
//---------------------------------------------------------------------------------------

class OSECORE_API FOSESavedMove_Character : public FSavedMove_Character
{
public:

   typedef FSavedMove_Character Super;

   FOSESavedMove_Character();
   virtual ~FOSESavedMove_Character() { }
   
   /// Clear saved move properties, so it can be re-used
   virtual void Clear() override;
   
   /// Returns a byte containing encoded special movement information (jumping, crouching, etc.)
   virtual uint8 GetCompressedFlags() const override;
   
   /// Returns true if this move can be combined with NewMove for replication without changing any behavior
   virtual bool CanCombineWith(const FSavedMovePtr& NewMove, class ACharacter* Character, float MaxDelta) const override;
   
   /// Returns true if this move is an "important" move that should be sent again if not acked by the server
   virtual bool IsImportantMove(const FSavedMovePtr& lastAckedMove) const override;
   
   /// Set the properties describing the position, etc. of the moved pawn at the start of the move.
   virtual void SetInitialPosition(class ACharacter* character) override;
   
   /// Called to set up this saved move (when initially created) to make a predictive correction
   virtual void SetMoveFor(class ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
   
   /// Called before ClientUpdatePosition uses this SavedMove to make a predictive correction
   virtual void PrepMoveFor(ACharacter* character) override;
   
   /// Set the properties describing the final position, etc. of the moved pawn.
   virtual void PostUpdate(ACharacter* character, EPostUpdateMode postUpdateMode) override;
   
   uint8 bWantsToSprint : 1;
   uint8 bIsSliding : 1;
   uint8 bDistanceConstraintEnabled : 1; // not serialized over wire
   uint8 bExtendingDistanceConstraint : 1;
   uint8 bWantsToContractDistanceConstraint : 1;
   uint8 bWantsToScramble : 1; // Taken care of as a compressed flag
   uint8 bWantsToWallClimb : 1;
   uint8 bWantsToMantle : 1; // Taken care of as a compressed flag
   uint8 bDistanceConstraintUpdate : 1;
   
   EDistanceConstraintDelta DistanceConstraintDelta;
   float DistanceConstraintEndDistance;
   
   float MaxSpeedMultiplier;
   float ExtraGravityScale;
   
   // Mantle state which is needed for server corrections / saved moves
   FOSEMantleState SavedMantleState;
   TWeakObjectPtr<class UAnimMontage> SavedMantleMontage;
   float SavedMantleTimeElapsed;
   float SavedMantleTimeRemaining;
   FVector_NetQuantize SavedMantleStartOffset;
   FVector_NetQuantize SavedMantleStartVelocity;
   bool SavedMantleHadRootMotion = false;
   
   // Ledge state which is needed for server corrections / saved moves
   FOSELedgeState SavedLedgeState;
   TWeakObjectPtr<class UAnimMontage> SavedLedgeMontage;
   float SavedLedgeMountTimeElapsed;
   float SavedLedgeMountTimeRemaining;
   FVector_NetQuantize SavedLedgeMountStartOffset;
   FVector_NetQuantize SavedLedgeMountStartVelocity;

   FOSEDistanceConstraint DistanceConstraint;
};


//---------------------------------------------------------------------------------------
/// Client side prediction data
//---------------------------------------------------------------------------------------

class OSECORE_API FOSENetPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
public:

   typedef FNetworkPredictionData_Client_Character Super;

   FOSENetPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) { }

   virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FOSESavedMove_Character()); }
};


//---------------------------------------------------------------------------------------
/// Server side prediction data
//---------------------------------------------------------------------------------------

class OSECORE_API FOSENetworkPredictionData_Server_Character : public FNetworkPredictionData_Server_Character
{
public:
   using Super = FNetworkPredictionData_Server_Character;

   FOSENetworkPredictionData_Server_Character(const UCharacterMovementComponent& serverMovement)
      : Super(serverMovement)
      , LastReceivedClientDistanceConstraintTimeStamp(0)
   {}

   /// Last timestamp a client move included a distance constraint change
   float LastReceivedClientDistanceConstraintTimeStamp;
};


//---------------------------------------------------------------------------------------
/// Custom character movement class
//---------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSECharacterMovement : public UCharacterMovementComponent
{
   GENERATED_BODY()

public:
   
   // To let us avoid making public accessors for internal state just so that FOSESavedMove_Character::SetMoveFor can copy it
   friend class FOSESavedMove_Character;
   
   UOSECharacterMovement(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
   
   virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
   virtual class FNetworkPredictionData_Server* GetPredictionData_Server() const override;
   
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   
   virtual void PostLoad() override;

   virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
   
   virtual float GetMaxAcceleration() const override;
   
   virtual float GetMaxBrakingDeceleration() const override;
   
   virtual float GetBaseMaxSpeed() const;
   
   virtual float GetMaxSpeed() const override;
   
   virtual float GetGravityZ() const override;
   
   /// Draw debug information for character movement (called with p.VisualizeMovement > 0)
   virtual float VisualizeMovement() const override;

   /// Helper function to get our custom movement type given the current movement mode
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement|OSE")
   virtual ECustomMovementType GetCustomMovementType() const;
   
   /// Helper function to set a custom movement type
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement|OSE")
   virtual void SetCustomMovementType(ECustomMovementType customType);
   
   // from UPawnMovementComponent
   virtual void NotifyBumpedPawn(APawn* bumpedPawn) override;
   
   virtual bool VerifyClientTimeStamp(float timeStamp, FNetworkPredictionData_Server_Character& serverData) override;
   
   // called from owning OSECharacterBase on authority only
   virtual void PossessedBy(AController* newController);
   virtual void UnPossessed();

   virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
   
   // called from owning OSECharacterBase on authority only
   void AuthorityOnLyingDownChanged(const bool isLyingDown);

protected:
#if WITH_EDITOR
   virtual bool CanEditChange(const FProperty* inProperty) const override;
#endif

public:

   /// Overridden to check for a pending mantle or scramble before committing to the jump.
   virtual bool CanAttemptJump() const override;

   /// Override to implement custom jump logic. Do not call directly. Note that you should usually
   /// trigger a jump through Character::Jump() instead.
   ///
   /// \see UOSECharacterMovement::_jumpSettings
   virtual bool DoJump(bool bReplayingMoves, float deltaTime) override;

   /// If the character can jump, applies custom jump physics. This call may directly modify velocity
   /// and can also apply forces and impulses. Returns true if any jump physics were applied.
   virtual bool DoJumpPhysics(const FOSEJumpPhysics& jumpPhysics, bool bUseViewRotation = false);

   /// Returns true if a crouching character can uncrouch and jump
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FOSECrouchJumpSettings& GetCrouchJumpSettings() const { return _crouchJumpSettings; }

   /** Returns true if the character is allowed to crouch in the current state. By default it is allowed when walking or falling, if CanEverCrouch() is true. */
   virtual bool CanCrouchInCurrentState() const override;

protected:

   /// Optional array of jump settings for multiple jumps. The maximum number of jumps is still set on the character.
   ///
   /// If this array is empty, the default JumpZVelocity is used. If this array is not empty, the jump count indexes into this
   /// array to get the jump settings to use. Indices beyond the array range are clamped to the last index.
   ///
   /// \see ACharacter::JumpMaxCount
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Jumping / Falling")
   TArray< FOSEJumpSettings > _jumpSettings;
   
   /// Defines the behavior when attempting a jump while crouched.
   UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetCrouchJumpSettings, Category = "Character Movement: Jumping / Falling")
   FOSECrouchJumpSettings _crouchJumpSettings;

private:
   /// used to cache off our crouching state at the start of movement
   /// updates so that we can send out an event after movement updates are complete
   bool _wasCrouchingBeforeMovementUpdate = false;

protected:

   virtual bool ClientUpdatePositionAfterServerUpdate() override;
   
   FOSECharacterNetworkMoveData* GetCurrentOSENetworkMoveData() const { return static_cast<FOSECharacterNetworkMoveData *>(GetCurrentNetworkMoveData()); }
   FOSECharacterMoveResponseDataContainer& GetOSEMoveResponseDataContainer() const { return static_cast<FOSECharacterMoveResponseDataContainer&>(GetMoveResponseDataContainer()); }
   
   /// Unpack compressed flags from a saved move and set state accordingly.
   virtual void UpdateFromCompressedFlags(uint8 Flags) override;
   
   /// Event notification when client receives correction data from the server, before applying the data. Base implementation logs relevant data and draws debug info if "p.NetShowCorrections" is not equal to 0.
   virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection) override;

   /// Called by UCharacterMovementComponent::VerifyClientTimeStamp() when a client timestamp reset has been detected and is valid.
   virtual void OnClientTimeStampResetDetected() override;
   
   /// Slows towards stop
   virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;
   
   /// Returns the ceiling height (or specified max height) by sweeping the player capsule upwards
   virtual float CheckCeilingClearance(float maxHeight, float radiusShrinkAmount = 0.0f) const;
   
   /// Returns true if we have suitable geometry to scramble directly in front of the character
   virtual bool CheckForwardScrambleClearance(float castDistance, float radiusShrinkAmount = 0.0f) const;
   
   /// Enforce constraints on input given current state. For instance, don't move upwards if walking and looking up.
   virtual FVector ConstrainInputAcceleration(const FVector& inputAcceleration) const override;
   
   /// Handle a blocking impact. Calls ApplyImpactPhysicsForces for the hit, if bEnablePhysicsInteraction is true.
   virtual void HandleImpact(const FHitResult& impact, float timeSlice = 0.0f, const FVector& moveDelta = FVector::ZeroVector) override;
   
   /// Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner.
   virtual void OnMovementModeChanged(EMovementMode previousMovementMode, uint8 previousCustomMode) override;

   virtual void PhysCustom(float deltaTime, int32 iterations) override;
   
   /// Custom movement mode for mantle
   virtual void PhysCustomMantle(float deltaTime, int32 iterations);

   /// Custom movement mode for wall climbing
   virtual void PhysCustomWallClimb(float deltaTime, int32 iterations);
   
   /// Custom movement mode for scrambling
   virtual void PhysCustomScramble(float deltaTime, int32 iterations);
   
   /// Override for detecting physmat changes
   virtual void SetPostLandedPhysics(const FHitResult& Hit) override;

public:

   /// Returns the velocity vector rotated to local space
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual FVector GetLocalVelocity() const;
   
   /// Update the character state in PerformMovement right before doing the actual position change
   virtual void UpdateCharacterStateBeforeMovement(float deltaSeconds) override;
   
   /// Update the character state in PerformMovement after the position change. Some rotation updates happen after this.
   virtual void UpdateCharacterStateAfterMovement(float deltaSeconds) override;

public:
   
   /// The minimum ground speed when sliding. If we fall below this speed, we won't be able to slide anymore.
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual float GetSlidingSpeedMin() const;
   
   /// The maximum ground speed when sliding. This is also the speed we must reach to initiate sliding.
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual float GetSlidingSpeedMax() const;

   /// Returns true if the character is currently sliding
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsSliding() const;
   
   /// Returns true if the character can ever slide
   virtual bool CanEverSlide() const;
   
   /// Returns true if the character is allowed to slide in the current state
   virtual bool CanSlideInCurrentState() const;

protected:
   
   /// Checks for slide conditions, and handles the transition to and from sliding state
   virtual void UpdateSlideState();
   
   /// True if the character can even attempt to slide
   UPROPERTY(Category = "Character Movement: Sliding", EditDefaultsOnly, BlueprintReadOnly)
   bool SlidingEnabled = false;
   
   /// Sliding settings
   UPROPERTY(Category = "Character Movement: Sliding", EditDefaultsOnly, BlueprintReadOnly, meta = (editcondition = "SlidingEnabled"))
   FOSESlidingSettings SlidingSettings;
   
   /// Internal state indicating if we are currently sliding or not
   UPROPERTY(Transient, DuplicateTransient, VisibleInstanceOnly)
   bool bIsSliding;

public:

   /// Returns true if the character is currently scrambling
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsScrambling() const;
   
   /// Returns true if the character can ever scramble
   virtual bool CanEverScramble() const;
   
   /// Returns true if the character can ever jump from a scramble
   virtual bool CanEverScrambleJump() const;
   
   /// Returns true if the character is allowed to scramble in the current state
   virtual bool CanScrambleInCurrentState() const;
   
   /// Returns true if the character is allowed to jump from a scramble in the current state
   virtual bool CanScrambleJumpInCurrentState() const;
   
   /// Sets scrambling state, and trigger OnStartScrambling() on the owner if successful.
   /// In general you should SetWantsToScramble() instead to have the scramble persist during movement, or just use the scramble functions on the owning character.
   /// \param inClientSimulation   True when called when _isScrambling is replicated to non owned clients.
   virtual void StartScrambling(bool inClientSimulation = false);
   
   /// Sets scrambling state, and trigger OnStopScrambling() on the owner if successful.
   /// \param inClientSimulation   True when called when _isScrambling is replicated to non owned clients.
   virtual void StopScrambling(bool inClientSimulation = false);
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetScrambleEnabled() const { return _scrambleEnabled; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetScrambleEnabled(bool inValue) { _scrambleEnabled = inValue; }

   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   const FOSEScrambleSettings& GetDefaultScrambleSettings() const { return _scrambleSettings; }
   const FOSEScrambleSettings& GetCurrentScrambleSettings() const { return _GetScrambleSettings(_scrambleImpactResult); }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetDefaultScrambleSettings(const FOSEScrambleSettings& inValue) { _scrambleSettings = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetWantsToScramble() const { return _wantsToScramble; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetWantsToScramble(bool inValue) { _wantsToScramble = inValue; }

   // Checks to see if we can set bIsSliding based on whether CanSlideInCurrentState() allows it.
   // If it does, calls _SetSliding().
   void _TrySetSliding(bool sliding);

   // Sets bIsSliding and calls OnStartSliding() or OnStopSliding() if necessary.
   void _SetSliding(bool sliding);
   
   /// Default initialization for the elapsed time since valid scramble impact
   static const float DEFAULT_TIME_SINCE_IMPACT;

protected:

   /// Checks for scramble conditions, and handles the transition to and from scrambling state
   virtual void UpdateScrambleState(float deltaSeconds);

   const FOSEScrambleSettings& _GetScrambleSettings(const FHitResult& impact) const;
   
   /// Utility method to determine if a hit result is valid for scrambling
   virtual bool IsValidScrambleHitResult(const FHitResult& impact) const;
   static bool IsValidScrambleHitResult(const FHitResult& impact, const FOSEScrambleSettings& scrambleSettings, const class ACharacter* characterOwner);

private:

   /// True if the character can even attempt to scramble
   UPROPERTY(Category = "Character Movement: Scrambling", EditAnywhere, BlueprintGetter = GetScrambleEnabled, BlueprintSetter = SetScrambleEnabled)
   bool _scrambleEnabled;

   /// Scrambling settings
   UPROPERTY(Category = "Character Movement: Scrambling", EditAnywhere, BlueprintGetter = GetDefaultScrambleSettings, BlueprintSetter = SetDefaultScrambleSettings, meta = (editcondition = "_scrambleEnabled"))
   FOSEScrambleSettings _scrambleSettings;

   /// Scrambling settings
   UPROPERTY(Category = "Character Movement: Scrambling", EditAnywhere, meta = (editcondition = "_scrambleEnabled"))
   TMap<TEnumAsByte<EPhysicalSurface>, FOSEScrambleSettings> _surfaceScrambleSettingOverrides;

   /// If true, try to scramble (or keep scrambling) on next update. If false, try to stop scrambling on next update.
   UPROPERTY(Category = "Character Movement: Scrambling", VisibleInstanceOnly, BlueprintGetter = GetWantsToScramble, BlueprintSetter = SetWantsToScramble)
   uint8 _wantsToScramble : 1;

   /// Internal state of the most recent impact result
   UPROPERTY(Transient, DuplicateTransient)
   FHitResult _scramblePendingResult;

   /// Internal state storing the last valid scramble impact result
   UPROPERTY(Transient, DuplicateTransient)
   FHitResult _scrambleImpactResult;

   /// Internal state used to keep track of how much time elapsed since we had a valid scramble impact
   UPROPERTY(Transient, DuplicateTransient)
   float _scrambleImpactElapsed;

   /// Internal state used to keep track of how much time elapsed since we started scrambling
   UPROPERTY(Transient, DuplicateTransient)
   float _scrambleDurationElapsed;

   /// Internal state indicating if scrambling is blocked (resets when touching the ground)
   UPROPERTY(Transient, DuplicateTransient)
   bool _scrambleIsBlocked;

   /// Internal state storing the ceiling clearance height
   UPROPERTY(Transient, DuplicateTransient)
   float _scrambleClearance;

   /// Internal state storing the forward scramble clearance
   UPROPERTY(Transient, DuplicateTransient)
   bool _scrambleForwardClearance;

   /// Current context used to detect changes in any values for gating delegate firing.
   FOSEMovementMaterialContext _currentMovementMaterialContext;

   /// Cached hit result to use in next update of _currentMovementMaterialContext
   FHitResult _cachedMovementMaterialHitResult;

   float _maxFallingSpeedOverride;

public:

   /// Get the base increase for max movement speed when sprinting
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual float GetSprintSpeedIncrementBase() const { return SprintSpeedIncrementBase; }

   /// Get the additional increase for max movement speed when sprinting forward
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual float GetSprintSpeedIncrementForward() const { return SprintSpeedIncrementForward; }

   /// Returns true if the character is currently sprinting
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsSprinting() const;

   /// Returns true if the character can ever sprint
   virtual bool CanEverSprint() const;

   /// Returns true if the character is allowed to sprint in the current state
   virtual bool CanSprintInCurrentState() const;

   /// Sets sprinting state, and trigger OnStartSprinting() on the owner if successful.
   /// In general you should set bWantsToSprint instead to have the sprint persist during movement, or just use the sprint functions on the owning character.
   /// \param bClientSimulation   True when called when bIsSprinting is replicated to non owned clients
   virtual void StartSprinting(bool bClientSimulation = false);

   /// Sets sprinting state, and trigger OnStopSprinting() on the owner if successful.
   /// \param bClientSimulation   True when called when bIsSprinting is replicated to non owned clients
   virtual void StopSprinting(bool bClientSimulation = false);

   /// If true, try to sprint (or keep sprinting) on next update. If false, try to stop sprinting on next update.
   UPROPERTY(Category = "Character Movement: Walking", VisibleInstanceOnly, BlueprintReadOnly)
   uint8 bWantsToSprint : 1;

   /// Delegate that is fired when the physical material the player is in contact with changes.
   UPROPERTY(BlueprintAssignable, Category = "Pawn|Components|CharacterMovement")
   FOSEOnPhysicalMaterialChanged OnPhysicalMaterialChanged;

protected:

   /// Checks for sprint conditions, and handles the transition to and from sprinting state
   virtual void UpdateSprintState();

   /// Checks for changes to the physics material and fires the delegate as needed.
   virtual void UpdatePhysicalMaterialContext();
   virtual void PopulatePhysicalMaterialContext(FOSEMovementMaterialContext& newContext) const;

   /// Base increase for max movement speed when sprinting
   /// \see UCharacterMovementComponent::MaxWalkSpeed
   UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float SprintSpeedIncrementBase;

   /// Additional increase for max movement speed when sprinting forward
   /// \see UCharacterMovementComponent::MaxWalkSpeed
   UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float SprintSpeedIncrementForward;

   /// If true, we will try to find the distance to the floor beneath us
   UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
   bool ShouldFindFloorWhenFalling = false;

   /// When falling, this is the max distance we will use to try to find the floor beneath us
   UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0", EditCondition = "ShouldFindFloorWhenFalling", EditConditionHides))
   float FallingFindFloorMaxDistance = 3000.0f;

public:

   /// Returns true if the character is currently mantling / vaulting
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsMantling() const;

   /// Returns true if the character can ever mantle / vault
   virtual bool CanEverMantle() const;
   
   /// Returns true if the character is allowed to mantle / vault in the current state
   virtual bool CanMantleInCurrentState() const;
   
   /// Sets mantle / vault state, and trigger OnStartMantle() on the owner if successful.
   /// In general you should SetWantsToMantle() instead to have the mantle persist during movement, or just use the mantle functions on the owning character.
   /// \param bClientSimulation   True when called when AOSECharacterBase::_mantleState is replicated to non owned clients
   virtual void StartMantle(bool isClientSimulation = false);
   
   /// Sets mantle / vault state, and trigger OnStopMantle() on the owner if successful.
   /// \param bClientSimulation   True when called when AOSECharacterBase::_mantleState is replicated to non owned clients
   virtual void StopMantle(bool bClientSimulation = false);
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetMantleEnabled() const { return _mantleEnabled; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetMantleEnabled(bool inValue) { _mantleEnabled = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FOSEMantleSettings& GetMantleSettings() const { return _mantleSettings; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetMantleSettings(const FOSEMantleSettings& inValue) { _mantleSettings = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const TArray<class UOSEMantleAnimSet*>& GetMantleAnimations() const { return _mantleAnimations; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetMantleAnimations(const TArray<class UOSEMantleAnimSet*>& inValue) { _mantleAnimations = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetWantsToMantle() const { return _wantsToMantle || IsWallClimbing(); }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetWantsToMantle(bool inValue) { _wantsToMantle = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual class UAnimMontage* GetMantleMontage() const { return _mantleMontage; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual float GetMantleTimeElapsed() const { return _mantleTimeElapsed; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual float GetMantleTimeRemaining() const { return _mantleTimeRemaining; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FVector& GetMantleStartOffset() const { return _mantleStartOffset; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FVector& GetMantleStartVelocity() const { return _mantleStartVelocity; }

   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetShouldMantle() const;

   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetShouldShimmy() const;
   
   /// Helper for saved moves. Allows us to keep the actual transient state protected
   /// while modified explicitly and with clear purpose.
   virtual void SetMantleTransients(const FOSESavedMove_Character& savedMove);

protected:

   /// Various setter methods for derived classes, which are intentionally not public or BP exposed as they manage internal state
   virtual void SetMantleMontage(class UAnimMontage* inValue) { _mantleMontage = inValue; }
   virtual void SetMantleTimeElapsed(float inValue) { _mantleTimeElapsed = inValue; }
   virtual void SetMantleTimeRemaining(float inValue) { _mantleTimeRemaining = inValue; }
   virtual void SetMantleStartOffset(const FVector& inValue) { _mantleStartOffset = inValue; }
   virtual void SetMantleStartVelocity(const FVector& inValue) { _mantleStartVelocity = inValue; }
   
   /// Given a mantle query, resolves it to a suitable mantle state. Returns true on success, and false on failure
   virtual bool ResolveMantleState(FOSEMantleState& outState, const FOSEMantleQueryResult& inQuery) const;
   
   /// Checks for mantle / vault conditions, and handles the transition to and from mantle / vault state
   virtual void UpdateMantleState();

#pragma region Ledge State Functions
public:

   /// Returns true if the character is currently in ledge state
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsInLedgeState() const;

   /// Returns true if the character is currently in ledge state
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsLedgeMounting() const;
   
   /// Returns true if the character can ever use ledges
   virtual bool CanEverUseLedges() const;
   
   /// Returns true if the character is allowed to use ledges in the current state
   virtual bool CanUseLedgesInCurrentState() const;

   /// Starts ledge state
   /// \param bClientSimulation   True when called when AOSECharacterBase::_ledgeState is replicated to non owned clients
   virtual void StartLedgeState(bool isClientSimulation = false);

   /// Stops ledge state
   /// \param bClientSimulation   True when called when AOSECharacterBase::_ledgeState is replicated to non owned clients
   virtual void StopLedgeState(bool bClientSimulation = false);

   /// Starts mounting state
   /// \param bClientSimulation   True when called when AOSECharacterBase::_ledgeState is replicated to non owned clients
   virtual void StartLedgeMount(bool isClientSimulation = false);

   /// Stops mounting state
   /// \param bClientSimulation   True when called when AOSECharacterBase::_ledgeState is replicated to non owned clients
   virtual void StopLedgeMount(bool bClientSimulation = false);
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetWantsToReleaseLedge() const { return _wantsToReleaseLedge; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetWantsToReleaseLedge(bool inValue) { _wantsToReleaseLedge = inValue; }
   
   /// Returns true if ledge state is enabled and the character is permitted to use ledge state
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetLedgeStateEnabled() const { return _ledgeStateEnabled; }
   
   /// Sets whether or not ledge state is enabled
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetLedgeStateEnabled(bool inValue) { _ledgeStateEnabled = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FOSELedgeSettings& GetLedgeSettings() const { return _ledgeSettings; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetLedgeSettings(const FOSELedgeSettings& inValue) { _ledgeSettings = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const TArray<class UOSELedgeAnimSet*>& GetMountAnimations() const { return _ledgeAnimations; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetMountAnimations(const TArray<class UOSELedgeAnimSet*>& inValue) { _ledgeAnimations = inValue; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual class UAnimMontage* GetLedgeMountMontage() const { return _mountMontage; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual float GetLedgeMountTimeElapsed() const { return _mountTimeElapsed; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual float GetLedgeMountTimeRemaining() const { return _mountTimeRemaining; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FVector& GetLedgeMountStartOffset() const { return _mountStartOffset; }
   
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual const FVector& GetLedgeMountStartVelocity() const { return _mountStartVelocity; }
   
   /// Helper for saved moves. Allows us to keep the actual transient state protected
   /// while modified explicitly and with clear purpose.
   virtual void SetLedgeTransients(const FOSESavedMove_Character& savedMove);

protected:

   /// Various setter methods for derived classes, which are intentionally not public or BP exposed as they manage internal state
   virtual void SetLedgeMountMontage(class UAnimMontage* inValue) { _mountMontage = inValue; }
   virtual void SetLedgeMountTimeElapsed(float inValue) { _mountTimeElapsed = inValue; }
   virtual void SetLedgeMountTimeRemaining(float inValue) { _mountTimeRemaining = inValue; }
   virtual void SetLedgeMountStartOffset(const FVector& inValue) { _mountStartOffset = inValue; }
   virtual void SetLedgeMountStartVelocity(const FVector& inValue) { _mountStartVelocity = inValue; }

   /// Given a ledge query, resolves it to a suitable ledge state. Returns true on success, and false on failure
   virtual bool ResolveLedgeQuery(FOSELedgeMountTarget& outState, const FOSELedgeQueryResult& inQuery) const;
   
   /// Given a ledge query, resolves it to a suitable ledge state. Returns true on success, and false on failure
   virtual bool ResolveLedgeShimmyQuery(FOSELedgeMountTarget& outState, const FOSELedgeQueryResult& inQuery) const;
   
   /// Checks ledge conditions, handles the transition to and from ledge state, and movement in the ledge state
   virtual void UpdateLedgeState();
   
   /// Custom movement mode for ledge
   virtual void PhysCustomLedge(float deltaTime, int32 iterations);
   virtual void PhysCustomLedge_Mount(float deltaTime, int32 iterations);
   
   virtual float VisualizeLedgeState(float HeightOffset) const;
   
   float DrawLedgeState(const FOSELedgeState& ledgeState, float HeightOffset, const FColor DebugColor, const float OffsetPerElement, FVector DebugLocation, const FVector TopOfCapsule) const;

#pragma endregion Ledge State Functions

#pragma region Ledge State Data
protected:

   /// If true, try to release the current ledge a drop. This will only happen succesfully if the character is in a state that permits it.
   UPROPERTY(Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetWantsToReleaseLedge, BlueprintSetter = SetWantsToReleaseLedge)
   bool _wantsToReleaseLedge;
   
   /// True if the character can even attempt to use ledges
   UPROPERTY(Category = "Character Movement: Ledge", EditAnywhere, BlueprintGetter = GetLedgeStateEnabled, BlueprintSetter = SetLedgeStateEnabled)
   bool _ledgeStateEnabled;

   /// Allows the character to shimmy. When false, the character can only ledge grab in place and/or mantle.
   UPROPERTY(Category = "Character Movement: Ledge", EditDefaultsOnly)
   bool _shimmyEnabled;

   /// True if the character can even attempt to use ledges
   UPROPERTY(Category = "Character Movement: Ledge", EditDefaultsOnly)
   bool _ledgeHangEnabled;
   
   /// Ledge query settings
   UPROPERTY(Category = "Character Movement: Ledge", EditAnywhere, BlueprintGetter = GetLedgeSettings, BlueprintSetter = SetLedgeSettings, meta = (editcondition = "_ledgeStateEnabled"))
   FOSELedgeSettings _ledgeSettings;
   
   /// Animation sets used to resolve
   UPROPERTY(Category = "Character Movement: Ledge", EditAnywhere, BlueprintGetter = GetMountAnimations, BlueprintSetter = SetMountAnimations, meta = (editcondition = "_ledgeStateEnabled"))
   TArray<class UOSELedgeAnimSet*> _ledgeAnimations;
   
   UPROPERTY(Category = "Character Movement: Ledge", EditAnywhere, meta = (editcondition = "_ledgeStateEnabled"))
   class UAnimMontage* _rightLedgeShimmy;
   
   UPROPERTY(Category = "Character Movement: Ledge", EditAnywhere, meta = (editcondition = "_ledgeStateEnabled"))
   class UAnimMontage* _leftLedgeShimmy;

   /// Internal state used to store the active mount montage
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetLedgeMountMontage)
   class UAnimMontage* _mountMontage;
   
   /// Internal state used to keep track of how much time elapsed in the mount sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetLedgeMountTimeElapsed)
   float _mountTimeElapsed;
   
   /// Internal state used to keep track of how much time is left in the mount sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetLedgeMountTimeRemaining)
   float _mountTimeRemaining;
   
   /// Internal state to keep track of the mount start offset we apply during the mount sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetLedgeMountStartOffset)
   FVector _mountStartOffset;
   
   /// Internal state to keep track of the mount start velocity when we begin the mount sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Ledge", VisibleInstanceOnly, BlueprintGetter = GetLedgeMountStartVelocity)
   FVector _mountStartVelocity;

   FOSELedgeState _DebugLedgeState;
#pragma endregion Ledge State Data

private:

   /// True if the character can even attempt to mantle / vault
   UPROPERTY(Category = "Character Movement: Mantling", EditAnywhere, BlueprintGetter = GetMantleEnabled, BlueprintSetter = SetMantleEnabled)
   bool _mantleEnabled;

   /// Mantle / vault query settings
   UPROPERTY(Category = "Character Movement: Mantling", EditAnywhere, BlueprintGetter = GetMantleSettings, BlueprintSetter = SetMantleSettings, meta = (editcondition = "_mantleEnabled"))
   FOSEMantleSettings _mantleSettings;
   
   /// Animation sets used to resolve
   UPROPERTY(Category = "Character Movement: Mantling", EditAnywhere, BlueprintGetter = GetMantleAnimations, BlueprintSetter = SetMantleAnimations, meta = (editcondition = "_mantleEnabled"))
   TArray<class UOSEMantleAnimSet*> _mantleAnimations;
   
   // CONSIDER: Should be in mantle settings?
   UPROPERTY(Category = "Character Movement: Mantling", EditDefaultsOnly, meta = (editcondition = "_mantleEnabled"))
   bool _allowMantleFromCrouch = false;
   
   /// If true, try to mantle / vault (or keep mantling) on next update. If false, try to stop mantling on next update.
   UPROPERTY(Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetWantsToMantle, BlueprintSetter = SetWantsToMantle)
   bool _wantsToMantle;

   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly)
   bool _mantleHadRootMotion = false;

   /// Internal state used to store the active mantle montage
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetMantleMontage)
   class UAnimMontage* _mantleMontage;
   
   /// Internal state used to keep track of how much time elapsed in the mantle sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetMantleTimeElapsed)
   float _mantleTimeElapsed;
   
   /// Internal state used to keep track of how much time is left in the mantle sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetMantleTimeRemaining)
   float _mantleTimeRemaining;
   
   /// Internal state to keep track of the mantle start offset we apply during the mantle sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetMantleStartOffset)
   FVector _mantleStartOffset;
   
   /// Internal state to keep track of the mantle start velocity when we begin the mantle sequence
   UPROPERTY(Transient, DuplicateTransient, Category = "Character Movement: Mantling", VisibleInstanceOnly, BlueprintGetter = GetMantleStartVelocity)
   FVector _mantleStartVelocity;

#pragma region Wall Climb Functions
public:   
   /// Sets wall climb state, and trigger OnStartWallClimb() on the owner if successful.
   /// In general you should SetWantsToWallClimb() instead to have the wall climb persist during movement, or just use the wall climb functions on the owning character.
   /// \param bClientSimulation   True when called when AOSECharacterBase::_wallClimbState is replicated to non owned clients
   virtual void StartWallClimb(bool bClientSimulation = false);

   /// Sets wall climb state, and trigger OnStopWallClimb() on the owner if successful.
   /// \param bClientSimulation   True when called when AOSECharacterBase::_wallClimbState is replicated to non owned clients
   virtual void StopWallClimb(bool bClientSimulation = false);

   /// Returns true if the character is currently wall climbing
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   virtual bool IsWallClimbing() const;

   /// Returns true if hit climable
   virtual bool IsClimbableHit(const FHitResult& hitResult) const;

   /// Returns true if the character can ever wall climb
   virtual bool CanEverWallClimb() const;

   /// Returns true if the character is allowed to wall climb in the current state
   virtual bool CanWallClimbInCurrentState(bool bIgnoreWalkableFloor = false) const;

   /// Returns true if the character is allowed to perform a wall climb auto jump
   virtual bool CanWallClimbAutoJump() const;

   /// Updates wall climb state before movement
   virtual void UpdateWallClimbState(float deltaTime);

   /// Returns true if Wall Climb is enabled
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetWallClimbEnabled() const { return _wallClimbEnabled; }
   
   /// Sets whether or not Wall Climb is enabled
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetWallClimbEnabled(bool inValue) { _wallClimbEnabled = inValue; }

   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetWantsToWallClimb() const { return _wantsToWallClimb; }
   
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetWantsToWallClimb(bool inValue) { _wantsToWallClimb = inValue; }

   // Tells us if the vector in question is currently within the bounds of allowing movement during wall climbing
   bool IsVectorWithinWallMovementBounds(const FVector& queryVector) const;

   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   const FOSEWallClimbSettings& GetDefaultWallClimbSettings() const { return _wallClimbSettings; }
   const FOSEWallClimbSettings& GetCurrentWallClimbSettings() const { return _GetWallClimbSettings(_wallClimbImpactResult); }

   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   void SetDefaultWallClimbSettings(const FOSEWallClimbSettings& settings) { _wallClimbSettings = settings; }

   virtual void AdjustWallDist();
   virtual void FindWall(const FVector& capsuleLocation, FFindWallResult& outWallResult, bool bCanUseCachedLocation);
   virtual void ComputeWallDist(const FVector& capsuleLocation, float lineDistance, float sweepDistance, FFindWallResult& outWallResult, float sweepRadius) const;
   virtual bool ComputePerchResult_WallClimb(const float testRadius, const FHitResult& inHit, const float inMaxFloorDist, FFindWallResult& outPerchWallResult) const;
   virtual bool WallSweepTest(struct FHitResult& outHit, const FVector& start, const FVector& end, ECollisionChannel traceChannel, const struct FCollisionShape& collisionShape, const struct FCollisionQueryParams& params, const struct FCollisionResponseParams& responseParam) const;

   /// Sets direction of the wall the character is climbing up
   void SetWallClimbDirection(FVector wallDirection) { _wallDirection = wallDirection; }

   FRotator GetWallAngleNormalRotation() const;
   FVector GetWallAngleNormal() const;

   void SetRotationInputThisFrame(const FRotator& inRotationInput) { _rotationInputThisFrame = inRotationInput; }

protected:

   const FOSEWallClimbSettings& _GetWallClimbSettings(const FHitResult& impact) const;

   virtual void MoveAlongWall(const FVector& inVelocity, float deltaSeconds, FStepDownResult* outStepDownResult = NULL);

   bool IsValidWallClimbHitResult(const FHitResult& impact) const;

   // Is this wall wide enough to fit the character?
   bool IsWallWideEnough(const FHitResult& impact) const;

   FVector ComputeWallMovementDelta(const FVector& delta, const FHitResult& rampHit) const;

   void CheckNegativeCollision(FVector& outVelocity);
   void ResolveNegativeCollisionHit(const FHitResult& hit, FVector& outVelocity, ECharacterRaycastLocation direction);

   void GetCharacterRaycastHit(FHitResult& outHit, ECharacterRaycastLocation direction) const;
   void GetCharacterRaycastInfo(FCharacterRaycastInfo& outRaycastInfo, ECharacterRaycastLocation direction) const;

   bool HasWallClimbClearance();

   void RemoveDirectionFromVector(FVector& outVector, const FVector& Direction);
   void RotateCharacterTowardsWall();
   void RotateCharacterTowardsView();

   void InitiateWallClimbDash(FVector DirectionNormal, bool bAutoWallDash = false);
   void WallClimbDashFinished();

   void ConvertCapsuleToCrouchSize(bool bClientSimulation);
   void RevertCapsuleToNormalSize(bool bClientSimulation);

   bool IsWallDashing() const { return _bIsWallDashing; }

   void ClearAutoTurn();
   float GetAngleBetweenVectors(const FVector& firstVector, const FVector& secondVector) const;
   bool _CanWallClimbOnSurface(const FHitResult& impact) const;

   void ManuallyCanceledWallClimbCooldownFinished();

   FVector GetWallClimbAccelValue();
   FRotator GetWallClimbAutoLookMountDirection(const FVector& currentAccel);

   void InitiateWallClimbAutoTurn(const FRotator& turnTarget);

#pragma endregion Wall Climb Functions

#pragma region Wall Climb Data
public:
   // Default initialization for wall climb movement mode
   static const float MIN_WALL_DIST;
   static const float MAX_WALL_DIST;

   /// Wall climb settings
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere, BlueprintGetter = GetDefaultWallClimbSettings, BlueprintSetter = SetDefaultWallClimbSettings, meta = (editcondition = "_wallClimbEnabled"))
   FOSEWallClimbSettings _wallClimbSettings;

   /// Wall climb settings overrides for certain physical materials
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere, meta = (editcondition = "_wallClimbEnabled"))
   TMap<TEnumAsByte<EPhysicalSurface>, FOSEWallClimbSettings> _surfaceWallClimbSettingOverrides;

   /// If true, then we will check if the wall we are attempting to climb is wide enough to climb on
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere, meta = (EditCondition = "_wallClimbEnabled"))
   bool _checkIfWallIsWideEnoughToClimb = true;
   
   /// If true, then we will only be able to wall climb on the surfaces in _wallClimbableSurfaces
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere, meta = (EditCondition = "_wallClimbEnabled"))
   bool _allowWallClimbOnlyOnSomeSurfaces = false;

   /// If _allowWallClimbOnlyOnSomeSurfaces is true, which surfaces allow us to wall climb on
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere, meta = (EditCondition = "_wallClimbEnabled && _allowWallClimbOnlyOnSomeSurfaces"))
   TArray<TEnumAsByte<EPhysicalSurface>> _wallClimbableSurfaces;

   /** Information about the wall the Character is climbing on (updated only during wall climb movement). */
	UPROPERTY(Category="Character Movement: Wall Climb", VisibleInstanceOnly, BlueprintReadOnly)
   FFindWallResult CurrentWall;

   /**
	* Force the Character in MOVE_WallClimb to do a check for a valid wall even if it hasn't moved. Cleared after next wall check.
	* Normally if bAlwaysCheckWall is false we try to avoid the wall check unless some conditions are met, but this can be used to force the next check to always run.
	*/
	UPROPERTY(Category="Character Movement: Wall Climb", VisibleInstanceOnly, BlueprintReadWrite, AdvancedDisplay)
	uint8 bForceNextWallCheck:1;

   /**
	* Whether we always force wall checks for stationary Characters while wall climbing.
	* Normally wall checks are avoided if possible when not moving, but this can be used to force them if there are use-cases where they are being skipped erroneously
	* (such as objects moving up into the character from below).
	*/
	UPROPERTY(Category="Character Movement: Wall Climb", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint8 bAlwaysCheckWall:1;

protected:
   /// True if the character can even attempt to wall climb
   UPROPERTY(Category = "Character Movement: Wall Climb", EditAnywhere)
   bool _wallClimbEnabled;

private:
   /// If true, try to wall climb. If false, try to stop Wall climbing on next update.
   UPROPERTY(Category = "Character Movement: Wall Climb", VisibleInstanceOnly, BlueprintGetter = GetWantsToWallClimb, BlueprintSetter = SetWantsToWallClimb)
   bool _wantsToWallClimb;

   /// Internal state of the most recent impact result
   UPROPERTY(Transient, DuplicateTransient)
   FHitResult _wallClimbPendingResult;

   /// Internal state storing the last valid wall climb impact result
   UPROPERTY(Transient, DuplicateTransient)
   FHitResult _wallClimbImpactResult;

   /// Direction of wall we are currently climbing
   FVector _wallDirection;

   /// Are we currently performing a wall dash
   bool _bIsWallDashing;

   /// Cleared the floor we were on when we first started wall climbing
   bool _bClearedInitialWallClimbFloor;

   /// Timer responsible for cleaning up data at the end of the wall dash
   FTimerHandle _wallDashTimer;

   /// Local space vector for wall dash direction
   FVector _wallClimbDashLocalAccelNormal;

   /// The rotational input for the camera this frame
   FRotator _rotationInputThisFrame;

   /// Where the camera was at the start of the auto-turn
   FRotator _wallClimbAutoTurnOrigin;

   /// Target for the wall climb auto turn
   FRotator _wallClimbAutoTurnTarget;

   /// Is Wall Climb Camera Auto Turning
   bool _bWallClimbCamAutoTurning;

   /// Did the player manually cancel wall climbing?
   bool _bManuallyCanceledWallClimb;

   /// Timer handle for the manually canceled wall climb cooldown
   FTimerHandle _manuallyCanceledWallClimbCooldownTimer;

#pragma endregion Wall Climb Data

public:
   
   /// Returns true if the character can ever dodge
   UFUNCTION(BlueprintPure, Category = "Character Movement: Dodging")
   virtual bool CanEverDodge() const;
   
   // Returns true if we can currently dodge
   UFUNCTION(BlueprintPure, Category = "Character Movement: Dodging")
   virtual bool CanDodgeInCurrentState() const;
   
   //// Which direction should we dodge in?
   //UFUNCTION(BlueprintPure, Category = "Character Movement: Dodging")
   //virtual FVector GetDodgeDirection() const;

   // Returns a (world-space) dodge vector determined by the character's movement acceleration (i.e. input). outIsStationaryDodge = true if the character was stationary before performing the dodge (in this case, _dodgeDirectionStationary is used)
   UFUNCTION(BlueprintPure, Category = "Character Movement: Dodging")
   virtual FVector GetDodgeDirection(bool& outIsStationaryDodge) const;

   // Takes a (world-space) vector indicating a desired dodge direction, and returns a dodge vector (transformed to align with ground surface as necessary). Used for involuntary dodges not driven by input direction
   FVector MakeDodgeDirectionFromVector(FVector moveDirection) const;
   
   // is dodge enabled?
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   virtual bool GetDodgeEnabled() const { return _dodgeEnabled; }
   
   // set dodge enabled
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   virtual void SetDodgeEnabled(bool inValue) { _dodgeEnabled = inValue; }

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

private:
   
   // Can we ever dodge?
   UPROPERTY(Category = "Character Movement: Dodging", EditAnywhere, BlueprintGetter = GetDodgeEnabled, BlueprintSetter = SetDodgeEnabled)
   bool _dodgeEnabled = true;

   // Minimum input magnitude to trigger a directional dodge (if below this, we fallback to stationaryDodgeDirection passed to GetDodgeDirection())
   UPROPERTY(Category = "Character Movement: Dodging", EditAnywhere, meta = (EditCondition = "_dodgeEnabled", UIMin = "0.0", ClampMin = "0.0"))
   float _dodgeMinimumAccelMagnitude = 0.25f;

   // Direction to dodge (relative to camera-forward) if performed without moving
   UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Dodging", meta = (EditCondition = "_dodgeEnabled"))
   FVector _dodgeDirectionStationary = FVector::BackwardVector;

public:
   
   /// Returns how far to rotate character during the time interval
   virtual FRotator GetDeltaRotation(float deltaTime) const override;
   
   /// Returns true if turn-in-place is enabled.
   ///
   /// \see UOSECharacterMovement::_turnInPlaceEnabled
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   bool GetTurnInPlaceEnabled() const { return _turnInPlaceEnabled; }
   
   /// Enables or disables turn-in-place.
   ///
   /// \see UOSECharacterMovement::_turnInPlaceEnabled
   UFUNCTION(BlueprintSetter, Category = "Pawn|Components|CharacterMovement")
   void SetTurnInPlaceEnabled(bool inValue) { _turnInPlaceEnabled = inValue; }
   
   /// Returns the current turn-in-place settings.
   ///
   /// \see UOSECharacterMovement::_turnInPlaceEnabled
   /// \see UOSECharacterMovement::_turnInPlaceSettings
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   const FOSETurnInPlaceSettings& GetTurnInPlaceSettings() const 
   {
      if (_turnInPlaceSettingsOverride.IsSet())
      {
         return _turnInPlaceSettingsOverride.GetValue();
      }
      return _turnInPlaceSettings; 
   }
   
   /// Returns the current turn-in-place settings.
   ///
   /// \see UOSECharacterMovement::_turnInPlaceEnabled
   /// \see UOSECharacterMovement::_turnInPlaceState
   UFUNCTION(BlueprintGetter, Category = "Pawn|Components|CharacterMovement")
   const FOSETurnInPlaceState& GetTurnInPlaceState() const { return _turnInPlaceState; }
   
   /// Returns true if turn-in-place is currently active.
   ///
   /// \see UOSECharacterMovement::_turnInPlaceEnabled
   /// \see UOSECharacterMovement::_turnInPlaceActive
   UFUNCTION(Category = "Pawn|Components|CharacterMovement")
   bool IsTurnInPlaceActive() const { return GetTurnInPlaceState().IsActive; }

   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   FORCEINLINE void SetTurnInPlaceSettingsOverride(const FOSETurnInPlaceSettings& turnInPlaceSettingsOverride) { _turnInPlaceSettingsOverride = turnInPlaceSettingsOverride; }
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   FORCEINLINE void ClearTurnInPlaceSettingsOverride() { _turnInPlaceSettingsOverride.Reset(); }

protected:
   
   /// Helper method that returns the rotation rate in degrees per second given a linear speed.
   /// Includes variants that use the scaled capsule radius and the current velocity.
   static float GetArcLengthRotationRate(float speed, float radius);
   float GetArcLengthRotationRate(float speed) const;
   
   /// Updates turn-in-place state
   virtual void UpdateTurnInPlace(float deltaSeconds);

private:

   /// Enables turn-in-place modifications to the rotation rate. When bUseControllerDesiredRotation
   /// is enabled, this will modify it within specified parameters (such as angle ranges, velocity, etc).
   /// When using this and bUseControllerDesiredRotation, you will want to make sure that other settings
   /// are cleared, such as bUseControllerRotationYaw on the Character.
   ///
   /// \see UCharacterMovementComponent::bUseControllerDesiredRotation
   /// \see UCharacterMovementComponent::RotationRate
   /// \see APawn::bUseControllerRotationYaw
   UPROPERTY(Category = "Character Movement (Rotation Settings)", EditAnywhere, BlueprintGetter = GetTurnInPlaceEnabled, BlueprintSetter = SetTurnInPlaceEnabled)
   bool _turnInPlaceEnabled = false;
   
   /// Settings to control the turn-in-place behavior
   UPROPERTY(Category = "Character Movement (Rotation Settings)", EditDefaultsOnly, BlueprintGetter = GetTurnInPlaceSettings, meta = (editcondition = "_turnInPlaceEnabled"))
   FOSETurnInPlaceSettings _turnInPlaceSettings;

   /// Override to _turnInPlaceSettings to be assigned at runtime for gameplay purposes
   /// (i.e. a lunge attack with a wind-up, where you'd want the player's character to closely match the camera-forward to visually communicate the lunge direction during the wind-up)
   /// TODO: do remote clients care enough about this to warrant replication?
   TOptional<FOSETurnInPlaceSettings> _turnInPlaceSettingsOverride;
   
   /// Internal run-time state for turn-in-place
   UPROPERTY(Category = "Character Movement (Rotation Settings)", Transient, DuplicateTransient, VisibleInstanceOnly, BlueprintGetter = GetTurnInPlaceState)
   FOSETurnInPlaceState _turnInPlaceState;
   
   /// if true max speed is scaled by _slopesMaxSpeedScale and the Z normal of the ramp.
   UPROPERTY(Category = "Character Movement: Walking", EditAnywhere)
   bool _slopesAffectMaxSpeed = false;
   
   /// the scale in which max speed will be adjusted by. 1 means max speed will be zero to doubled.
   UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, meta = (editcondition = "_slopesAffectMaxSpeed", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
   float _slopeFrictionScale = .25;

protected:

   /// OSE character this movement component belongs to
   UPROPERTY(Transient, DuplicateTransient)
   class AOSECharacterBase* OSECharOwner;

public:

   /// Sets the distance constraint to use. Internally this updates the desired distance constraint.
   /// The value is not replicated directly. Consider using the character method which will call this
   /// and replicate the value to the server and other clients.
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   void SetDistanceConstraint(const FOSEDistanceConstraint& distanceConstraint);
   
   /// Returns the current distance constraint in effect. Note that this may not be the value that
   /// was set, as it can interpolate towards the desired value over time.
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   const FOSEDistanceConstraint& GetDistanceConstraint() const { return _distanceConstraintDesired; }
   
   /// Helper function which simply returns the enabled flag for the distance component
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   bool GetDistanceConstraintEnabled() const { return GetDistanceConstraint().Enabled; }
   
   /// Returns true if the distance constraint is currently affecting the character.
   /// See notes below re: _affectedByDistanceConstraint
   UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
   bool GetAffectedByDistanceConstraint() const { return _affectedByDistanceConstraint; }
   
   void SetExtendingDistanceConstraint(bool value) { _extendingDistanceConstraint = value; }
   bool IsExtendingDistanceConstraint() const { return _extendingDistanceConstraint; }
   void SetContractingDistanceConstraint(bool value) { _wantsToContractDistanceConstraint = value; }
   bool IsContractingDistanceConstraint() const { return _wantsToContractDistanceConstraint; }
   
   /// Checks if we're falling, and if so will give the current distance to the floor,
   /// and the max distance to the floor we've seen for the current jump
   /// Only valid on local players and the server
   UFUNCTION(BlueprintPure, Category = "Pawn|Components|CharacterMovement")
   bool GetDistanceToFloorWhenFalling(float& outFloorDistance, float& outMaxFloorDistanceThisFall) const;

protected:

   virtual FVector GetAirControl(float deltaTime, float tickAirControl, const FVector& fallAcceleration) override;
   virtual void OnMovementUpdated(float deltaSeconds, const FVector& oldLocation, const FVector& oldVelocity) override;

protected:

   /// When swinging/dangling, amount of lateral movement control available to the character.
   /// 0 = no control, 1 = full control at max speed of MaxWalkSpeed.
   UPROPERTY(Category = "Character Movement: Swinging", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float SwingAirControl;

   /// When swinging/dangling, multiplier applied to SwingAirControl when lateral velocity is less than SwingAirControlBoostVelocityThreshold.
   /// Setting this to zero will disable air control boosting. Final result is clamped at 1.
   UPROPERTY(Category = "Character Movement: Swinging", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float SwingAirControlBoostMultiplier;
   
   /// When swinging/dangling, if lateral velocity magnitude is less than this value, SwingAirControl is multiplied by SwingAirControlBoostMultiplier.
   /// Setting this to zero will disable air control boosting.
   UPROPERTY(Category = "Character Movement: Swinging", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float SwingAirControlBoostVelocityThreshold;

private:
   
   void _RelaxDistanceConstraintIfNeeded();
   void _UpdateDistanceConstraintControls(float deltaSeconds);

   bool _FindFloorDistanceWhenFalling(float& outFloorDistance) const;
   
   void _UpdateCustomCharacterStateOnMovementNone();

   /// The desired distance constraint value
   UPROPERTY(Transient)
   FOSEDistanceConstraint _distanceConstraintDesired;
   
   EDistanceConstraintDelta _lastDistanceConstraintDelta;

   /// @TODO: This would likely be better served as a floating point "tension" value, which specifies
   /// how much the distance constraint is affecting movement. This can then be fed into the animation
   /// system in order to blend or choose other visual representations.
   ///
   /// Used for animation etc to know if we're affected by the distance constraint. To be affected
   /// means distance constraint is enabled, a correction to the velocity or position was made due to it, or
   /// there is hysteresis for the value based on a threshold (capsule radius, perhaps?)
   UPROPERTY(Transient)
   uint8 _affectedByDistanceConstraint : 1;
   
   UPROPERTY(Transient)
   uint8 _extendingDistanceConstraint : 1;
   
   UPROPERTY(Transient)
   uint8 _wantsToContractDistanceConstraint : 1;
   
   uint8 _clientResimulateDistanceConstraint : 1;

public:
   
   void SetMaxSpeedMultiplier(float maxSpeed) { _maxSpeedMultiplier = maxSpeed; }
   void SetFrictionMultiplier(float friction) { _frictionMultiplier = FMath::Max(0.0f, friction); }
   void SetBrakingDecelerationMultiplier(float brakingDeceleration) { _brakingDecelerationMultiplier = FMath::Max(0.0f, brakingDeceleration); }
   
   void SetExtraGravityScale(float extraGravityScale) { _extraGravityScale = extraGravityScale; }

   float GetFallingFloorDistance() const { return _fallingFloorDistance; }

   float GetTimeSinceLastSlip() const { return _timeSinceLastImpact; }

protected:
   virtual void _ComputeFrictionAndBrakingDeceleration(float& inOutFriction, float& inOutBrakingDeceleration) const;

   float _maxSpeedMultiplier;
   float _frictionMultiplier = 1.0f;
   float _brakingDecelerationMultiplier = 1.0f;
   float _extraGravityScale = 1.f;

private:
   FOSECharacterNetworkMoveDataContainer _networkMoveDataContainer;
   FOSECharacterMoveResponseDataContainer _responseDataContainer;

private:
   /// Does the authority want to use RVO Avoidance when we're driven by AI?
   bool _authorityDisabledRVOAvoidanceForPlayer = false;

   /// Did we start with CMC RVO enabled?
   bool _authorityRVOWasEnabled = false;

   /// Distance to floor, only updated when falling
   float _fallingFloorDistance = 0.0f;

   /// Max distance to floor seen since we started falling, only updated when falling
   float _fallingMaxFloorDistanceThisFall = 0.0f;

   float _timeSinceLastImpact = 100.0f;

public:
   UFUNCTION(BlueprintCallable, Category = "Character Movement: Vaulting")
   bool FindVaultLedges(FVector& FrontLedge, FVector& FrontLedgeNormal, FVector& BackLedge, FVector& BackLedgeNormal);

   // How far in front of the player to we look for vaultable walls
   UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Vaulting")
   float VaultForwardDistance = 300.0f;

   // How high the front ledge needs to be minimum
   UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Vaulting")
   float VaultFrontLedgeHeightMin = 50.0f;

   // How high the back ledge needs to be minimum
   UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Vaulting")
   float VaultBackLedgeHeightMin = 50.0f;

   // How many downward traces do we want within the distance were looking
   UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Vaulting")
   int VaultDownwardTraceResolution = 8;

private:
   FVector _frontVaultLedge;
   FVector _frontVaultLedgeNormal;

   FVector _backVaultLedge;
   FVector _backVaultLedgeNormal;
};
