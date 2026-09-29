// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"



// ue4

#include "AbilitySystemInterface.h"

#include "GameplayCueInterface.h"

#include "GameplayTagAssetInterface.h"

#include "GameFramework/Character.h"


// ose
#include "OSECharacterUtils.h"
#include "Abilities/Attributes/AttributeBaseInterface.h"
#include "Abilities/Attributes/AttributeBaseSystemInterface.h"
#include "Character/AvatarScaleInterface.h"
#include "Character/OSEFootstepSimulatorProviderInterface.h"
#include "Character/OSETeamInterface.h"

#include "Items/ToolHolderInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Traversal/TraversalInterface.h"
#include "Traversal/Mantle/OSELedgeState.h"
#include "VoiceOver/OSEVoiceOverTriggers.h"

#include "OSECharacterBase.generated.h"
struct FActiveGameplayEffectHandle;
struct FGameplayAbilitySpecHandle;
struct FOnAttributeChangeData;
struct FInputActionValue;
struct FOSEInputContextPriority;
class UAbilitySystemComponent;
class UCombatComponent;
class UCurveFloat;
class UInputMappingContext;
class UInteractMontageMappingAsset;
class UOSECharacterInputActionsAsset;
class UOSEVoiceOverTriggers;
class UPhysicalAnimationComponent;
class USyncedAnimationCharacterMontagesAsset;
class UUserWidget;
class UUtilityAIBehavior;

DECLARE_LOG_CATEGORY_EXTERN(LogOSECharacter, Log, All);

/// How the sprint should behave in response to request/cancel inputs
/// AI should use SprintWhileRequested, while player characters will likely use one of the other two
UENUM()
enum class EOSESprintInputBehavior
{
   /// Requesting a sprint will start a sprint if able, canceling a sprint will stop it
   SprintWhileRequested,

   /// Pressing the sprint button will start sprinting. Even if the button is released, we will continue an existing sprint until it stops
   ContinueSprint,

   /// Pressing the sprint button will start sprinting, and continue even if the button is released
   /// If the button is pressed and released again while sprinting, it will cancel the current sprint
   ContinueSprintAndAllowCancel,

   /// Pressing the sprint button will start sprinting, and continue even if the button is released
   /// If the button is pressed again while sprinting, it will cancel the current sprint
   ContinueSprintAndAllowCancelOnPress
};

UCLASS()
class OSECORE_API AOSECharacterBase
   : public ACharacter
   , public IAbilitySystemInterface
   , public IAttributeBaseInterface
   , public IAttributeBaseSystemInterface
   , public IAvatarScaleInterface
   , public IToolHolderInterface
   , public IToolSetSystemInterface
   , public ITraversalInterface
   , public IGameplayCueInterface
   , public IGameplayTagAssetInterface
   , public IOSETeamInterface
   , public IOSEFootstepSimulatorProviderInterface
{
   GENERATED_BODY()

public:

   /// Sets default values for this character's properties
   AOSECharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

   /// Overridable native event for when play begins for this actor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void Destroyed() override;

   /// Overridable function called whenever this actor is being removed from a level
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   /// Update the character
   virtual void Tick(float DeltaTime) override;

   /// Returns the aim rotation. Use the default if we have a controller; otherwise use the replicated value
   virtual FRotator GetBaseAimRotation() const override;

   /// Returns the view rotation (direction they are looking, normally Controller->ControlRotation).
   /// Use the default if we have a controller; otherwise use the replicated value
   virtual FRotator GetViewRotation() const override;

   /// Called to bind functionality to input
   virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

   /// Called on the actor right before replication occurs
   virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

   /// IAbilitySystemInterface
   virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override final;
   class UOSEAbilitySystemComponent* GetAbilitySystemComponentFromActor() const;

   /// Triggers ability based on input mapping. Usually this happens automatically, but can be
   /// manually triggered like this from code or blueprints for non-player controlled characters.
   /// Positive values of HoldDuration will delay releasing the input, otherwise we'll release it
   /// on the next frame.
   UFUNCTION(BlueprintCallable)
   bool TryActivateAbilityByInputID(EAbilityInputType inputCommand, float holdDuration = 0.0f);

   // Triggers a LocalInputConfirm on the character's Ability System Component, if any.
   UFUNCTION(BlueprintCallable)
   bool TryConfirmAbility(); // @TODO: More fully simulate the Confirm input event?

   /// C++ accessible event that fires when we're possessed by a controller
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPossessedBy, AController*, newController);
   FOnPossessedBy OnPossessedBy;

   virtual void PossessedBy(AController* newController) override;
   virtual void UnPossessed() override;
   virtual void OnRep_Controller() override;
   virtual void Restart() override;
   virtual void EnableInput(APlayerController* playerController) override;
   virtual void DisableInput(APlayerController* playerController) override;

   virtual void OnMovementModeChanged(EMovementMode prevMovementMode, uint8 previousCustomMode = 0) override;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
   virtual bool CanMovementComponentEditCrouchHeight() const { return true; }
#endif

   // Name of the ToolSet Component Name
   static const FName ToolSetComponentName;
   // Name of the Combat Component Name
   static const FName CombatComponentName;
   // Name of the Item Inventory Component Name
   static const FName ItemInventoryComponentName;
public:

   /// Returns the base attribute set
   virtual class UAttributeBaseSet* GetBaseAttributeSet() const;

   // IAttributeBaseSystemInterface
   virtual TScriptInterface< IAttributeBaseInterface > GetBaseAttributeInterface() const override;

   // IAttributeBaseInterface (Health)
   virtual float GetHealth() const override;
   virtual float GetHealthMax() const override;
   virtual float GetHealthPercent() const override;
   virtual float GetHealthRegenRate() const override;

   // IAttributeBaseInterface (Energy)
   virtual float GetEnergy() const override;
   virtual float GetEnergyMax() const override;
   virtual float GetEnergyPercent() const override;
   virtual float GetEnergyRegenRate() const override;

   // IAttributeBaseInterface (Damage)
   virtual float GetHealthDamage() const override;
   virtual float GetAttackDamage() const override;
   virtual float GetAttackDamageMultiplier() const override;
   virtual float GetDamageReductionMultiplier() const override;

   // IAttributeBaseInterface (Movement)
   virtual float GetMovementMaxSpeedMultiplier() const override;
   virtual float GetMovementFrictionMultiplier() const override;
   virtual float GetMovementBrakingDecelerationMultiplier() const override;
   virtual float GetGravityScale() const override;

   // Called after the ability system component has been created, grants initial abilities as needed
   virtual void InitializeAbilities(UOSEAbilitySystemComponent* InComponent, UAttributeBaseSet* InAttributeSet);

   // Called to try and reset ability system to default state before initialization
   virtual void ResetAbilities();

   // Call to register a delegate for when abilities are initialized, if already initialized will run immediately
   void CallOrRegisterAbilitiesInitializedDelegate(const FSimpleMulticastDelegate::FDelegate& initializeDelegate);

   // Call to register a delegate for when abilities are reset, will never call immediately
   void RegisterAbilitiesResetDelegate(const FSimpleMulticastDelegate::FDelegate& resetDelegate);

public:
   UPROPERTY(Transient, BlueprintReadOnly, Category = "Physical Animation")
   UPhysicalAnimationComponent* PhysicalAnimation;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
   TSoftClassPtr<UUserWidget> CharacterHUD;

   UFUNCTION(BlueprintCallable, Category = "Ragdoll")
   virtual bool GetIsRagdolling() const { return _ragdollParams.IsValid(); }

   UFUNCTION(BlueprintCallable, Category = "Ragdoll")
   virtual void SetRagdollingParams(const FOSERagdollParams& params);

   UFUNCTION(BlueprintCallable, Category = "Ragdoll")
   virtual void SetRagdollingParamsAndActivate(const FOSERagdollParams& params);

   UFUNCTION(BlueprintCallable, Category = "Ragdoll")
   virtual void SetRagdollActive(bool active);

   // Damage VO Hooks
   UPROPERTY(EditDefaultsOnly, Category = "Audio")
   UOSEVoiceOverTriggers* VoiceOverTriggers = nullptr;

protected:
   UPROPERTY(Transient, ReplicatedUsing= OnRep_RagdollParams )
   FOSERagdollParams _ragdollParams;

   UFUNCTION()
   void OnRep_RagdollParams();

   void _ApplyRagdollParams();
   void _TickRagdoll();

protected:

   /// The base aim rotation is typically the camera rotation for players. This raw camera
   /// rotation is replicated to remote (non owned) clients
   UPROPERTY(Transient, Replicated)
   FRotator RemoteBaseAimRotation;

   /// The view rotation is typically the controller's control rotation. This view/headlook
   /// rotation is replicated to remote (non owned) clients
   UPROPERTY(Transient, Replicated)
   FRotator RemoteViewRotation;

protected:

   /// IToolHolderInterface
   virtual USceneComponent* GetToolRoot(EMeshPerspective MeshPerspective) const override;

   /// The movement speed modifier that will be applied to the character movement component
   virtual float GetMovementMaxSpeedMultiplierToApply() const { return GetMovementMaxSpeedMultiplier(); }
   virtual float GetMovementFrictionMultiplierToApply() const { return GetMovementFrictionMultiplier(); }
   virtual float GetMovementBrakingDecelerationMultiplierToApply() const { return GetMovementBrakingDecelerationMultiplier(); }

public:

   /// IToolSetSystemInterface
   virtual TScriptInterface<IToolSetInterface> GetToolSetInterface() const override;

public:

   /// ITraversalInterface (crouching)
   /// This method implementation is for convenience and consistency only; overriden (and final)
   /// to pass through to the movement component version only.
   virtual bool IsCrouching() const override final;

   /// ITraversalInterface (sliding)
   virtual bool CanSlide() const override;
   virtual bool IsSliding() const override;


   /// Event called when the character starts sliding
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartSliding();

   virtual void OnStartSliding_Implementation();

   /// Event called when the character stops sliding
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopSliding();
   virtual void OnStopSliding_Implementation();

   /// Event called after movement, including sliding, has been fully resolved, and our sliding state has changed
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnCrouchingStateChanged(bool isCrouching);
   virtual void OnCrouchingStateChanged_Implementation(bool isCrouching) { }



protected:

   virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
   virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

   // Overridden to support jumping while scrambling,
   // and to support jumping while crouched
   virtual bool CanJumpInternal_Implementation() const override;

#pragma region Scramble
public:

   /// ITraversalInterface (scrambling)
   virtual bool CanScramble() const override;
   virtual bool IsScrambling() const override;
   virtual void StartScrambling() override;
   virtual void StopScrambling() override;

   /// Event called when the character starts scrambling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartScrambling(const FHitResult& initialClimbImpact);
   virtual void OnStartScrambling_Implementation(const FHitResult& initialClimbImpact);

   /// Event called when the character jumps from scrambling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnScrambleJump(const FHitResult& lastClimbImpact);
   virtual void OnScrambleJump_Implementation(const FHitResult& lastClimbImpact);

   /// Event called when the character stops scrambling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopScrambling(const FHitResult& lastClimbImpact);
   virtual void OnStopScrambling_Implementation(const FHitResult& lastClimbImpact);

   /// Do not call this function directly. Called by character movement to
   /// specify that this character is currently scrambling.
   void SetIsScrambling(bool inValue) { _isScrambling = inValue; }

protected:
  // Internal counter to keep track of the climb requests. We want to climb
   // only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 _scrambleRequestCounter;

   /// Do not set this value directly. Set by character movement to
   /// specify that this character is currently scrambling.
   UPROPERTY(Transient, ReplicatedUsing = OnRep_IsScrambling)
   uint8 _isScrambling : 1;

   UPROPERTY(EditDefaultsOnly, Category = "OSE|Character")
   FGameplayTagContainer _scrambleSuppressionTags;

   /// Handles scrambling replicated from server
   UFUNCTION()
   void OnRep_IsScrambling();
#pragma endregion Scramble
public:

   /// ITraversalInterface (sprinting)
   virtual bool CanSprint() const override;
   virtual bool IsSprinting() const override;
   virtual void SprintRequest() override;
   virtual void SprintCancel() override;

   /// ITraversalInterface (crouch)
   virtual bool CanCrouch() const override;
   virtual void CrouchRequest() override;
   virtual void CrouchCancel() override;

   /// Event called when the character starts sprinting
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartSprinting();
   virtual void OnStartSprinting_Implementation();

   /// Event called when the character stops sprinting
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopSprinting();
   virtual void OnStopSprinting_Implementation();

   /// Do not call this function directly. Called by character movement to
   /// specify that this character is currently sprinting. Instead, call SprintRequest()
   void SetIsSprinting(bool InValue) { bIsSprinting = InValue; }

protected:

   bool _HasAdditionalSprintCount() const;
   void _SprintToggleStarted();
   void _SprintToggleCompleted();

   // Internal counter to keep track of the sprint requests. We want to sprint
   // only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 SprintRequestCounter;

   // Internal counter to keep track of the crouch requests. We want to croutch
// only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 CrouchRequestCounter;

   /// Internal timer to detect if we've requested a sprint but stayed in place for too long
   /// see StationaryTimeToCancelSprint
   UPROPERTY(Transient, DuplicateTransient)
   float _stationarySprintCancelTimer = 0.0f;


   /// Do not set this value directly. Set by character movement to
   /// specify that this character is currently sprinting.
   UPROPERTY(Transient, ReplicatedUsing = OnRep_IsSprinting)
   uint8 bIsSprinting : 1;

   /// Used to defer a SprintCancel to the next frame
   /// This avoids a situation where we cancel a sprint in the middle of a correction,
   /// which is then potentially stomped because bWantsToSprint is a client-preserved flag
   UPROPERTY(Transient)
   bool _cancelSprintOnNextTick = false;

   /// Modifies SprintRequest() behavior -- players want a sprint request to last until they stop moving, potentially allowing another button press to cancel,
   /// but the AI wants SprintRequest/Cancel to act as a true toggle.
   UPROPERTY()
   EOSESprintInputBehavior _sprintInputBehavior = EOSESprintInputBehavior::ContinueSprintAndAllowCancel;

   /// If we request a sprint with `bAdditionalSprintCount = true` and then cancel the request without actually starting to sprint,
   /// should we cancel the sprint within a grace period
   UPROPERTY()
   bool bSprintCanceledIfStationary = false;

   /// How many seconds after requesting and canceling a sprint (with `bAdditionalSprintCount = true`) do we wait for sprinting to begin
   /// before we cancel the additional request altogether
   UPROPERTY(EditDefaultsOnly, Category = "Traversal", Meta = (EditCondition = "bSprintCanceledIfStationary", EditConditionHides))
   float StationaryTimeToCancelSprint = 0.2f;

   /// Handles sprinting replicated from server
   UFUNCTION()
   void OnRep_IsSprinting();

   void _TickStationarySprintTimer(float deltaTime);

public:
   virtual void ExtendDistanceConstraintRequest();
   virtual void ExtendDistanceConstraintCancel();

   virtual void ContractDistanceConstraintRequest();
   virtual void ContractDistanceConstraintCancel();

   /// Do not call this function directly.
   void SetExtendingDistanceConstraint(bool InValue) { bExtendingDistanceConstraint = InValue; }

   /// Do not call this function directly.
   void SetContractingDistanceConstraint(bool InValue) { bContractingDistanceConstraint = InValue; }

protected:
   // counter of number current requests to release the brake there are
   UPROPERTY(Transient, DuplicateTransient)
   int32 _extendDistanceConstraintCounter;

   UPROPERTY(Transient, DuplicateTransient)
   int32 _contractDistanceConstraintCounter;

   UPROPERTY(Transient, ReplicatedUsing = OnRep_ExtendingDistanceConstraint)
   uint8 bExtendingDistanceConstraint : 1;

   UPROPERTY(Transient, ReplicatedUsing = OnRep_ContractingDistanceConstraint)
   uint8 bContractingDistanceConstraint : 1;

private:
   UFUNCTION()
   void OnRep_ExtendingDistanceConstraint();

   UFUNCTION()
   void OnRep_ContractingDistanceConstraint();

public:

   /// ITraversalInterface (mantling)
   virtual bool CanMantle() const override;
   virtual bool IsMantling() const override;
   virtual void StartMantleAttempt() override;
   virtual void StopMantleAttempt() override;
   virtual void StartReleaseLedge() override;
   virtual void StopReleaseLedge() override;

#pragma region Wall Climb Data
protected:
   // Internal counter to keep track of the climb requests. We want to climb
   // only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 _wallClimbRequestCounter;

   /// Do not set this value directly. Set by character movement to
   /// specify that this character is currently scrambling.
   UPROPERTY(Transient, ReplicatedUsing = OnRep_IsWallClimbing)
   uint8 _isWallClimbing : 1;
#pragma endregion Wall Climb Data

#pragma region Wall Climb Functions
   /// Handles wall climbing replicated from server
   UFUNCTION()
   void OnRep_IsWallClimbing();

public:
   /// ITraversalInterface (Wall Climb)
   virtual bool CanWallClimb() const override;
   virtual bool IsWallClimbing() const override;
   virtual void StartWallClimbing() override;
   virtual void StopWallClimbing() override;

   /// Event called when the character starts Wall Climbing
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartWallClimbing();
   virtual void OnStartWallClimbing_Implementation();

   /// Event called when the character stops Wall Climbing
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopWallClimbing();
   virtual void OnStopWallClimbing_Implementation();

   /// Event called when the character starts Wall Dashing
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartWallDash(bool bAutoWallDash);
   virtual void OnStartWallDash_Implementation(bool bAutoWallDash);

   /// Event called when the character stops Wall Dashing
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopWallDash();
   virtual void OnStopWallDash_Implementation();

   /// Do not call this function directly. Called by character movement to
   /// specify that this character is currently wall climbing.
   void SetIsWallClimbing(bool inValue) { _isWallClimbing = inValue; }
#pragma endregion Wall Climb Functions

   UPROPERTY(EditDefaultsOnly, Category = "Stamina")
   bool bInfiniteStamina = false;

   /// Event called when the character starts Falling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartFalling();
   virtual void OnStartFalling_Implementation();

   /// Event called when the character stops Falling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopFalling();
   virtual void OnStopFalling_Implementation();

   /// Do not call these functions directly! Called by character movement to
   /// update the current mantle state. Instead, call StartMantling()
   void SetIsMantling(bool inValue);
   void SetMantleState(const struct FOSEMantleState& inValue) { _mantleState = inValue; }
   const struct FOSEMantleState& GetMantleState() const { return _mantleState; }

   /// Event called when the character starts mantling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartMantling();
   virtual void OnStartMantling_Implementation();

   /// Event called when the character stops mantling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopMantling();
   virtual void OnStopMantling_Implementation();

#pragma region Ledge Movement Functions
public:
   virtual bool CanUseLedges() const override;
   virtual bool IsOnLedge() const override;
   virtual bool IsLedgeMounting() const;


   /// Do not call these functions directly! Called by character movement to
   /// update the current mantle state. Instead, call StartMantling()
   void SetIsLedgeMode(bool inValue);
   void SetLedgeState(const struct FOSELedgeState& inValue) { _ledgeState = inValue; }
   const struct FOSELedgeState& GetLedgeState() const { return _ledgeState; }
   void SetIsLedgeMounting(bool inValue);

   /// Event called when the character starts mantling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStartLedgeState();
   virtual void OnStartLedgeState_Implementation();

   /// Event called when the character stops mantling
   UFUNCTION(BlueprintNativeEvent, Category = Traversal)
   void OnStopLedgeState();
   virtual void OnStopLedgeState_Implementation();
#pragma endregion Ledge Movement Functions
#pragma region Ledge Movement Data
private:
   // Internal counter to keep track of the ledge release requests. We want to release
   // only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 _ledgeReleaseCounter;

   /// Do not set this value directly! Set by character movement to update the character
   /// ledge state, and for replication to simulated proxies.
   UPROPERTY(Transient, DuplicateTransient, ReplicatedUsing = OnRep_LedgeState)
   struct FOSELedgeState _ledgeState;

   /// Handles ledge state replicated from server. This is only replicated
   /// for simulated proxies. As input is not replicated to simulated proxies,
   /// this is how they initiate the mantle process.
   UFUNCTION()
   void OnRep_LedgeState(const struct FOSELedgeState& prevLedgeState);
#pragma endregion Ledge Movement Data

   // Internal counter to keep track of the mantle requests. We want to mantle
   // only when it is >0
   UPROPERTY(Transient, DuplicateTransient)
   int32 _mantleRequestCounter;

   /// Do not set this value directly! Set by character movement to update the character
   /// mantle state, and for replication to simulated proxies.
   UPROPERTY(Transient, DuplicateTransient, ReplicatedUsing = OnRep_MantleState)
   struct FOSEMantleState _mantleState;

   /// Handles mantling replicated from server. This is only replicated
   /// for simulated proxies. As input is not replicated to simulated proxies,
   /// this is how they initiate the mantle process.
   UFUNCTION()
   void OnRep_MantleState(const struct FOSEMantleState& prevMantleState);

public:

  /// ITraversalInterface (ground speed)
   virtual EOSEGroundSpeed GetGroundSpeedThreshold(float speedValue) const override;
   virtual EOSEGroundSpeed GetGroundSpeed() const override;

public:
   //---------------------------------------------------------------------------------------
   // IGameplayTagAssetInterface
   //---------------------------------------------------------------------------------------

   virtual bool HasMatchingGameplayTag(FGameplayTag tagToCheck) const override;
   virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override;
   virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override;
   virtual void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const override;

protected:

   /// Returns unit movement axis direction in world space.
   /// The default behavior is to convert the view rotation to world space.
   ///
   /// The view rotation is typically the control rotation. The base aim rotation is typically the camera rotation.
   /// The control/view rotation is preferred for local-space movement (what it means to move "forward", "right", "up").
   /// When the control/view rotation is used over the base aim/camera rotation, the movement direction won't change
   /// just because the camera shakes or bounces, for example.
   ///
   /// The control/view rotation will also work well for AI movement. The control/view rotation for AI is expected
   /// to be the headlook (or view) rotation just like it is for players!
   virtual FVector GetMovementAxisForward() const;
   virtual FVector GetMovementAxisRight() const;
   virtual FVector GetMovementAxisUp() const;

   /// Movement actions
   virtual void AddMovementInputForward(float value) { AddMovementInput(GetMovementAxisForward(), value); }
   virtual void AddMovementInputRight(float value) { AddMovementInput(GetMovementAxisRight(), value); }

protected:
   virtual void _OnMoveInput(const FInputActionValue& value);
   virtual void _OnLookInput(const FInputActionValue& value);

protected:
   //---------------------------------------------------------------------------------------
   // Enhanced Input
   //---------------------------------------------------------------------------------------

   // Default input contexts
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   TArray<FOSEInputContextPriority> DefaultInputMappingContexts;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   UOSECharacterInputActionsAsset* InputActionsAsset = nullptr;

   /// from APawn, this is the recommended location to init enhanced input default input context, per docs
   virtual void PawnClientRestart() override;

private:
   UPROPERTY(Transient)
   TArray<UInputMappingContext*> _loadedInputMappingContexts;

public:
   /// Get the default input mapping contexts to query for keys
   const TArray<FOSEInputContextPriority>& GetDefaultInputMappingContexts() const { return DefaultInputMappingContexts; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPawnClientRestart);
   UPROPERTY(BlueprintAssignable)
   FOnPawnClientRestart OnPawnClientRestart;

   // Event called when a local Controller possesses this Pawn on both the server and the client
   // Note that this will be called on local players, as well as AI which are local to the server, so check for newController->IsPlayerController()
   // if you want to filter for players
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Possessed (Local)"))
   void ReceiveLocalPossessed(AController* newController);

   // Event called when a local Controller unpossesses this Pawn on both the server and the client
   // Note that this will be called on local players, as well as AI which are local to the server, so check for oldController->IsPlayerController()
   // if you want to filter for players
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Unpossessed (Local)"))
   void ReceiveLocalUnpossessed(AController* oldController);

public:
   //---------------------------------------------------------------------------------------
   // Character Ready
   //---------------------------------------------------------------------------------------

   /// Broadcast when this character is ready to play locally -- tools, input, etc are all replicated fully.
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterReady, AOSECharacterBase*, character);
   UPROPERTY(BlueprintAssignable, Category = "Character", meta = (DisplayName = "OnLocalCharacterIsReady"))
   FOnCharacterReady OnCharacterReady;

   UFUNCTION(BlueprintPure, meta = (DisplayName = "IsLocalCharacterReady"))
   bool IsCharacterReady() const;

protected:
   // for cpp subclasses
   virtual void _OnIsCharacterReadyChanged(bool isReady) { }

   // for bp subclasses
   UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnLocalCharacterIsReady"))
   void _BPOnIsCharacterReadyChanged(bool isReady);
   void _BPOnIsCharacterReadyChanged_Implementation(bool isReady) { };

   void _TickIsCharacterReady();
   bool _isCharacterReady = false;

public:
   //---------------------------------------------------------------------------------------
   // IOSETeamInterface
   //---------------------------------------------------------------------------------------

   virtual uint8 GetTeam() const override;

public:
   //---------------------------------------------------------------------------------------
   // Combat
   //---------------------------------------------------------------------------------------
   UFUNCTION(BlueprintPure, Category = "Combat")
   UCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
   UCombatComponent* CombatComponent = nullptr;

public:
   //---------------------------------------------------------------------------------------
   // Attribues and Tags
   //---------------------------------------------------------------------------------------

   UFUNCTION(BlueprintNativeEvent, Category = "Health")
   void OnHealthChanged(float newValue, float oldValue);
   virtual void OnHealthChanged_Implementation(float newValue, float oldValue) {};

   UFUNCTION(BlueprintNativeEvent, Category = "Condition")
   void OnConditionGained(const FGameplayTag NewConditionTag);
   virtual void OnConditionGained_Implementation(const FGameplayTag newConditionTag) {};

   UFUNCTION(BlueprintNativeEvent, Category = "Condition")
   void OnConditionStabilized();
   virtual void OnConditionStabilized_Implementation() {};

   bool IsUnconscious() const { return _isUnconscious; }

   UFUNCTION(BlueprintNativeEvent, Category = "Condition")
   void OnUnconsciousChanged(bool isUnconscious);
   virtual void OnUnconsciousChanged_Implementation(bool isUnconscious);

   UFUNCTION(BlueprintNativeEvent, Category = "Carrying")
   void OnCarryingChanged(bool isCarrying);
   virtual void OnCarryingChanged_Implementation(bool isCarrying);
   UFUNCTION(BlueprintPure, Category = "Carrying")
   bool IsCarrying() const { return _isCarrying; }

   DECLARE_EVENT_OneParam(AOSECharacterBase, FOnLyingDownChanged, bool);
   FOnLyingDownChanged OnLyingDown;

   UFUNCTION(BlueprintNativeEvent, Category = "Status")
   void OnLyingDownChanged(bool isLyingDown);
   virtual void OnLyingDownChanged_Implementation(bool isLyingDown);
   UFUNCTION(BlueprintPure, Category = "Status")
   bool IsLyingDown() const { return _isLyingDown; }

   UFUNCTION(BlueprintNativeEvent, Category = "Condition")
   void OnMovementIsInhibitedChanged(bool isMovementInhibited);
   virtual void OnMovementIsInhibitedChanged_Implementation(bool isMovementInhibited);

   // Fired when this character is knocked out by another character (player or NPC)
   // NB: This is based off when we receive HealthDamage, which means we may not have 0 Health, or have the unconscious tag yet
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void AuthorityOnKnockedOutByOtherCharacter(AOSECharacterBase* otherCharacter);
   virtual void AuthorityOnKnockedOutByOtherCharacter_Implementation(AOSECharacterBase* otherCharacter) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void AuthorityOnKnockedOutByNonCharacterSource();
   virtual void AuthorityOnKnockedOutByNonCharacterSource_Implementation() { }

protected:

   virtual void _OnHealthChanged(const FOnAttributeChangeData& data);
   virtual void _OnDamageChanged(const FOnAttributeChangeData& data);

   void _OnConditionTagChanged(const FGameplayTag tag, int32 newTagCount);
   void _OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount);
   void _OnLyingDownTagChanged(const FGameplayTag tag, int32 newTagCount);
   void _OnMovementImpairingTagChanged(const FGameplayTag tag, int32 newTagCount);
   void _OnCarryingTagChanged(const FGameplayTag tag, int32 newTagCount);

   void _RegisterConditionTagEvent(const FGameplayTag& conditionTag);
   void _RegisterLyingDownTagEvent(const FGameplayTag& conditionTag);
   void _RegisterMovementImpairingTagEvent(const FGameplayTag& movementImpairingTag);

   UFUNCTION()
   void OnRep_AbilityComponents();

   /// Called on both client and server when the ability system component has been initialized and default abilities/effects added
   UFUNCTION(BlueprintNativeEvent, Category = "Ability")
   void OnAbilitiesInitialized();
   virtual void OnAbilitiesInitialized_Implementation() {};

   /// Called on both client and server when abilities/effects should be removed from the ability system component, mostly useful for players
   UFUNCTION(BlueprintNativeEvent, Category = "Ability")
   void OnAbilitiesReset();
   virtual void OnAbilitiesReset_Implementation() {};

protected:
   // from ACharacter
   virtual void Landed(const FHitResult& hit) override;

protected:

   /// If this returns true, then the normal character-landed-on-surface logic is skipped (eg. fall damage).
   virtual bool _InterceptLanded(const FHitResult& hit) { return false; }

   /// Delegate called when ability system has initialized
   FSimpleMulticastDelegate OnAbilitiesInitializedDelegate;
   FSimpleMulticastDelegate OnAbilitiesResetDelegate;

   /// This will be true if it created the ability system component in the C++ constructor, false if it was created later and needs replication fixups
   UPROPERTY(BlueprintReadOnly, Category = "Ability")
   bool bCreatedStaticAbilityComponents = false;

   /// If true, the initial ability system initialization has happened
   UPROPERTY(Transient, BlueprintReadOnly, Category = "Ability")
   bool bAreAbilitiesInitialized = false;

   /// Ability system component, this is allocated after creation based on bUseExternalAbilitiesComponent
   UPROPERTY(ReplicatedUsing=OnRep_AbilityComponents, Transient)
   class UOSEAbilitySystemComponent* AbilitySystemComponent;

   /// Base ability attribute set
   UPROPERTY(ReplicatedUsing=OnRep_AbilityComponents, Transient)
   class UAttributeBaseSet* BaseAttributeSet;

   /// Initial abilities
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TArray< class UOSEGameplayAbilitySet* > InitialAbilitySets;

   /// Handles to granted abilities
   TArray< FGameplayAbilitySpecHandle > GrantedInitialAbilities;

   /// Initial gameplay effects
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TArray< class UOSEGameplayEffectSet* > InitialEffectSets;

   /// Handles to granted effects
   TArray< FActiveGameplayEffectHandle > GrantedInitialEffects;

   /// Tool set component
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools")
   class UToolSetComponent* ToolSetComponent;

   FDelegateHandle _onCarryingChangeDelegateHandle;

   // Are we carrying a body right now?
   bool _isCarrying = false;

   // Are we lying down right now?
   bool _isLyingDown = false;

   // Are we unconscious right now?
   bool _isUnconscious = false;

   // Are we inhibiting movement right now?
   bool _isMovementInhibited = false;

   struct FTagBindingInfo
   {
      FGameplayTag Tag;
      FDelegateHandle DelegateHandle;
   };
   TArray<FTagBindingInfo> _conditionChangeDelegateHandles;
   TArray<FTagBindingInfo> _lyingDownDelegateHandles;
   TArray<FTagBindingInfo> _movementImpairingDelegateHandles;
   FDelegateHandle _unconsciousDelegateHandle;

   // Avatar scale is a normalized value representing the size of the avatar relative to a "normal" six foot human
   // This value is used in adjusting animation selection
   UPROPERTY(EditDefaultsOnly, Category=Character)
   float _avatarScale;

public:
   virtual float GetAvatarScale() const override final { return _avatarScale; }

   /// Sets the distance constraint locally on the movement component and
   /// ensures it is replicated.
   UFUNCTION(BlueprintCallable, Category = "Character|OSE")
   void SetDistanceConstraint(FOSEDistanceConstraint distanceConstraint);

   // Do not call this directly
   void SetDistanceConstraintRaw(const FOSEDistanceConstraint& distanceConstraint) { _distanceConstraint = distanceConstraint; }

private:
   /// Applies the value to the movement component
   void _UpdateDistanceConstraint();

   void _SetContractingDistanceConstraint(bool newValue);
   void _SetExtendingDistanceConstraint(bool newValue);

   UFUNCTION()
   void _OnRep_DistanceConstraint();

   // When the health damage attribute is incremented on authority, check if we're knocked out
   // If so, look for the instigator to find out who killed us
   void _AuthorityCheckForDamageKnockout(const FOnAttributeChangeData& data);

   /// Replicated. When the value is set, sets the desired distance constraint on the movement component.
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_DistanceConstraint)
   FOSEDistanceConstraint _distanceConstraint;

protected:
   AActor* _GetInstigatorForAttributeChange(const FOnAttributeChangeData& data);

public:
   // Get the synced animation montages for this character
   UFUNCTION(BlueprintPure)
   USyncedAnimationCharacterMontagesAsset* GetSyncedAnimationCharacterMontagesAsset() const { return _syncedAnimationCharacterMontageAsset; }

protected:
   /// Synced character animation montage definitions for this character
   /// These montages will be validated in the editor to match the skeleton for this character
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animations")
   USyncedAnimationCharacterMontagesAsset* _syncedAnimationCharacterMontageAsset;

protected:
   //---------------------------------------------------------------------------------------
   // Falling Damage
   //---------------------------------------------------------------------------------------

   /// Event called when the character takes falling damage
   UFUNCTION(BlueprintNativeEvent, Category = "Falling Damage")
   void _OnFallingDamageTaken(const FHitResult& landingHit, float damageTakenHealthMaxPct, float damageTaken);
   virtual void _OnFallingDamageTaken_Implementation(const FHitResult& landingHit, float damageTakenHealthMaxPct, float damageTaken) { }

   /// Event called when the character lands and sends along some of our interpreted data.
   /// normalizedFallingHeight is a 0-1 value between the min fall height and death fall height which
   /// is useful for something like playing audio sfx depending on height without looking at velocity
   UFUNCTION(BlueprintNativeEvent, Category = "Falling Damage")
   void _OnLandedWithData(const FHitResult& landingHit, float normalizedFallingHeight);
   virtual void _OnLandedWithData_Implementation(const FHitResult& landingHit, float normalizedFallingHeight) { }

   /// From what height should we consider this a fall at all?
   UPROPERTY(EditDefaultsOnly, Category = "Falling Damage", meta = (ClampMin = "0", UIMin = "0"))
   float _minFallHeight = 10.0f;

   /// From what approximate height should we start taking damage?
   UPROPERTY(EditDefaultsOnly, Category = "Falling Damage", meta = (ClampMin = "0", UIMin = "0"))
   float _safeFallHeight = 550.0f;

   /// From what approximate height should we die?
   UPROPERTY(EditDefaultsOnly, Category = "Falling Damage", meta = (ClampMin = "0", UIMin = "0"))
   float _deathFallHeight = 1200.0f;

   /// Scale the amount of max player health we take between the safe fall height and the death height
   UPROPERTY(EditDefaultsOnly, Category = "Falling Damage")
   UCurveFloat* _fallingDamageCurve = nullptr;

public:
   //---------------------------------------------------------------------------------------
   // Bumping
   //---------------------------------------------------------------------------------------

   /// Event called when this character bumps into another
   UFUNCTION(BlueprintNativeEvent, Category = "Bumping")
   void OnBumpedInto(AOSECharacterBase* otherCharacter, const FHitResult& impact);
   virtual void OnBumpedInto_Implementation(AOSECharacterBase* otherCharacter, const FHitResult& impact);

   /// Event called when this character bumps into another
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBumpedInto, AOSECharacterBase*, otherCharacter, const FHitResult&, impact);
   UPROPERTY(BlueprintAssignable)
   FOnBumpedInto OnBumpedIntoEvent;

   /// Event called when another character bumps into us
   UFUNCTION(BlueprintNativeEvent, Category = "Bumping")
   void OnBumpedBy(AOSECharacterBase* otherCharacter, const FHitResult& impact);
   virtual void OnBumpedBy_Implementation(AOSECharacterBase* otherCharacter, const FHitResult& impact);

   /// Event called when another character bumps into us
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBumpedBy, AOSECharacterBase*, otherCharacter, const FHitResult&, impact);
   UPROPERTY(BlueprintAssignable)
   FOnBumpedBy OnBumpedByEvent;

public:
   //---------------------------------------------------------------------------------------
   // Footsteps
   //---------------------------------------------------------------------------------------

   //  IOSEFootstepSimulatorProviderInterface
   virtual UOSEFootstepSimulatorComponent* GetFootstepComponent() const override;

private:
   UPROPERTY(EditDefaultsOnly)
   class UOSEFootstepSimulatorComponent* _footstepComponent = nullptr;
};
