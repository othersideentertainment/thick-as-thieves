// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OSEPlayerController.generated.h"

enum class ECommonInputType : uint8;
class UOSEAimAssistComponent;
class UOSEPerformanceTestComponent;
class UOSEPingSystemComponent;

enum class EOSEInputHardwareType : uint8;

//--------------------------------------------------------------------------------------------------
/// Explicit player camera modes we support. The camera is not limited to this. It merely manages
/// the behavior we want for certain camera modes, in a simple and consistent way.
//--------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EPlayerCameraMode : uint8
{
   /// No explicit camera mode. Behavior of the view targets is unchanged.
   Default,

   /// First person camera mode. The view target is expected to be the possessed pawn.
   /// If no pawn is possessed, the player controller is used as the view target.
   FirstPerson,

   /// Third person camera mode. The view target is expected to be a separate actor.
   /// If no pawn is possessed, the player controller is used as the view target.
   ThirdPerson,
};


//--------------------------------------------------------------------------------------------------
/// Structure defining the settings used for a specific player camera mode. For example, the actor
/// class to use to spawn a view target, and the transition parameters.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FPlayerCameraSettings
{
   GENERATED_BODY()

public:

   FPlayerCameraSettings() { }

   FPlayerCameraSettings(
      const FViewTargetTransitionParams& InParams,
      const TSubclassOf<class ACameraActor>& InClass = nullptr)
      : TransitionParams(InParams), CameraClass(InClass) { }

   /// Transition parameters to use when switching view targets in this mode
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
   FViewTargetTransitionParams TransitionParams;

   /// The optional actor class to spawn for a camera. It is assumed for
   /// any spawned actor that the owner will be the player controller
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
   TSubclassOf<class ACameraActor> CameraClass;
};

//--------------------------------------------------------------------------------------------------
/// Our player controller version. Handles specialized camera modes, as well as pawn updating
/// based on rotation, etc.
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API AOSEPlayerController : public APlayerController
{
   GENERATED_BODY()

public:

   AOSEPlayerController(const FObjectInitializer& objectInitializer);

   /// Static utility function. Returns the OSE-specific player controller
   /// at the specified player index, or nullptr if the controller is not an AOSEPlayerController.
   UFUNCTION(BlueprintPure, Category = "Player Controller|OSE", meta = (WorldContext = "contextObj"))
   static AOSEPlayerController* GetOSEPlayerController(const UObject* contextObj, int index);

   /// Static utility function. Returns the OSE-specific player controller if and only if
   /// it is locally controlled. Thus, dedicated servers will return nullptr and you can rely
   /// on local-only behavior.
   UFUNCTION(BlueprintPure, Category = "Player Controller|OSE", meta = (WorldContext = "contextObj"))
   static AOSEPlayerController* GetLocalOSEPlayerController(const UObject* contextObj);

   /// Called after the actor's components have been initialized, only during gameplay and some editor previews.
   virtual void PostInitializeComponents() override;

   /// Overridable native event for when play begins for this actor
   virtual void BeginPlay() override;

   /// Overridable function called whenever this actor is being removed from a level
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   /// Processes player input (immediately after PlayerInput gets ticked) and calls UpdateRotation().
   /// PlayerTick is only called if the PlayerController has a PlayerInput object.
   /// Therefore, it will only be called for locally controlled PlayerControllers.
   virtual void PlayerTick(float deltaTime) override;

   /// Update the camera manager; this is called after all actors have been ticked.
   virtual void UpdateCameraManager(float deltaTime) override;

   /// Set the control rotation. The RootComponent's rotation will also be updated to match it if RootComponent->bAbsoluteRotation is true.
   virtual void SetControlRotation(const FRotator& newRotation) override;

   /// Updates the rotation of player, based on ControlRotation after RotationInput has been applied.
   /// This may then be modified by the PlayerCamera, and is passed to Pawn->FaceRotation().
   virtual void UpdateRotation(float deltaTime) override;

   /// Enables cheats within the game
   virtual void EnableCheats() override;

   /// Objects that should seamless travel along w/ the player controller
   virtual void GetSeamlessTravelActorList(bool toEntry, TArray<class AActor*>& actorList) override;
   
   /// Called to notify the server when the client has loaded a new world via seamless traveling
   virtual void NotifyLoadedWorld(FName worldPackageName, bool finalDest) override;

   // Public access to the ping system
   UFUNCTION(BlueprintPure)
   UOSEPingSystemComponent* GetPingSystem() const { return PingSystem; }

   // Public access to aim assist
   UFUNCTION(BlueprintPure)
   UOSEAimAssistComponent* GetAimAssist() const { return AimAssist; }
   
   /// Disable or Enable local user input from activating abilities
   void SetIgnoreAbilityInput(bool newIgnore);
   bool IsAbilityInputIgnored() const { return _abilityInputIgnoreCount > 0; }

protected:

   /// Overriding so we can reset interpolated data and adjust camera mode if needed
   virtual void OnPossess(APawn* inPawn) override;

   /// Overriding so we can reset camera mode if needed
   virtual void OnUnPossess() override;

public:
   virtual void SetPlayer(UPlayer* player) override;

   /// Setter for Pawn. Normally should only be used internally when possessing/unpossessing a Pawn.
   /// Overridden to call the OnPawnChanged delegate
   virtual void SetPawn(APawn* inPawn) override;

   /// Overridden to call the OnPawnChanged delegate
   virtual void OnRep_Pawn() override;

   /// Broadcasts when the pawn changes
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPawnChanged, APawn*, newPawn);
   UPROPERTY(BlueprintAssignable, Category = "Player Controller")
   FOnPawnChanged OnPawnChanged;

public:

   /// Returns the OSE-specific version of the player state
   UFUNCTION(BlueprintPure, Category = "Player State|OSE")
   AOSEPlayerState* GetOSEPlayerState() const;

   /// Called on the server when we create our player state.
   /// Overriden to call the OnPlayerStateChanged delegate
   virtual void InitPlayerState() override;

   /// Called on clients when we replicate our player state.
   /// Overriden to call the OnPlayerStateChanged delegate
   virtual void OnRep_PlayerState() override;

   /// Broadcasts when the player state changes
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, APlayerState*, newPlayerState);
   UPROPERTY(BlueprintAssignable, Category = "Player State")
   FOnPlayerStateChanged OnPlayerStateChanged;

public:

   /// Returns the current player camera mode for the player controller. This is not the same
   /// as the APlayerController implementaton of Get/SetCameraMode, which passes through to the camera
   /// manager 'style' variable an FName indicating the camera manager mode.
   UFUNCTION(BlueprintCallable, Category = "Camera|OSE")
   virtual EPlayerCameraMode GetPlayerCameraMode() const { return _playerCameraMode; }

   /// @brief Sets the current player camera mode to use for the player controller
   /// @param inMode The new mode to set
   /// @return Returns the previous camera mode
   UFUNCTION(BlueprintCallable, Category = "Camera|OSE")
   virtual EPlayerCameraMode SetPlayerCameraMode(const EPlayerCameraMode inMode);

   /// @brief Resets the current player camera mode.
   /// @return Returns the previous camera mode.
   UFUNCTION(BlueprintCallable, Category = "Camera|OSE")
   virtual EPlayerCameraMode ResetPlayerCameraMode() { return SetPlayerCameraMode(EPlayerCameraMode::Default); }

   /// Updates the view target as needed for the specified player camera mode. This method has
   /// no effect if the player controller isn't locally controlled, or if the camera mode is the default.
   virtual void TickPlayerCameraMode(float deltaTime);

   /// Directly adds extra rotation input to the player controller.
   /// This is reduced over time at a user-specified rate.
   /// \see _extraRotationInput
   /// \see _extraRotationRate
   UFUNCTION(BlueprintCallable, Category = "Camera|OSE")
   virtual void AddExtraRotationInput(const FRotator& deltaRotation) { _extraRotationInput += deltaRotation.GetNormalized(); }

   /// Given a desired control rotation, calculates the extra rotation input needed to arrive there.
   /// As extra rotation input is stateless and applied over time, this does not guarantee the control
   /// rotation will arrive there (due to user input, etc). However, it does ensure that existing
   /// rotation input is taken into account.
   UFUNCTION(BlueprintCallable, Category = "Camera|OSE")
   virtual FRotator CalculateExtraRotationInput(const FRotator& desiredControlRotation) const;

protected:

   /// First person mode camera settings
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerController)
   FPlayerCameraSettings _cameraSettings1P;

   /// Third person mode camera settings
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerController)
   FPlayerCameraSettings _cameraSettings3P;

   /// The rate of change for extra rotation input.
   /// Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
   /// \see _extraRotationInput
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerController)
   float _extraRotationRate;

protected:

   /// Returns the suggested view target for the given player camera mode
   AActor* GetSuggestedViewTarget(const EPlayerCameraMode inMode);
   AActor* GetSuggestedViewTarget() { return GetSuggestedViewTarget(GetPlayerCameraMode()); }

   /// Spawns the camera actor for specified settings
   class ACameraActor* SpawnCameraActor(const FPlayerCameraSettings& inSettings);

   /// The current camera mode
   UPROPERTY(Transient)
   EPlayerCameraMode _playerCameraMode = EPlayerCameraMode::Default;

   /// Spawned actor for third person mode
   UPROPERTY(Transient)
   class ACameraActor* _cameraActor3P = nullptr;

   /// Extra rotation input applied to the base class rotation input for local controllers.
   /// This differs from rotation input in that it interpolates to zero at a user-defined rate.
   /// This allows stateless camera / aiming modifications that persist over time.
   /// \see _extraRotationRate
   UPROPERTY(Transient)
   FRotator _extraRotationInput = FRotator::ZeroRotator;

public:

   /// Returns the most recent input device hardware type
   UFUNCTION(BlueprintPure, Category = "Player Controller|OSE")
   EOSEInputHardwareType GetCurrentInputHardwareType() const { return _inputHardwareType; }
   
   /// Broadcasts when the input device hardware changes
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputHardwareTypeChanged, EOSEInputHardwareType, newInputHardwareType);
   UPROPERTY(BlueprintAssignable, Category = "Player Controller")
   FOnInputHardwareTypeChanged OnInputHardwareTypeChanged;

   /// Used by our enhanced input mouse smoother after it consumes the number of samples in this frame
   void ResetMouseNumSamples() { _mouseNumSamples = 0; }
   int GetMouseNumSamples() const { return _mouseNumSamples; }

protected:
   // from APlayerController
   virtual bool InputKey(const FInputKeyParams& params) override;

   virtual void OnInputMethodChanged(ECommonInputType inputType);

private:
   EOSEInputHardwareType _inputHardwareType;

   /// keep track of the mouse input samples between frames
   int _mouseNumSamples = 0;

   int32 _abilityInputIgnoreCount = 0;

protected:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UOSEPingSystemComponent* PingSystem = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UOSEAimAssistComponent* AimAssist = nullptr;

private:
   UPROPERTY(Transient)
   UOSEPerformanceTestComponent* _performanceTestComponent = nullptr;

private:
   // using the same trick as AController to keep track of the old pawn in OnRep for later use.
   // we'd use theirs but it's private, and this is easier than an engine mod
   TWeakObjectPtr<APawn> _oldPawn;
};
