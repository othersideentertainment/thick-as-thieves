// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/OSEPlayerController.h"

// OSE
#include "OSECommon.h"
#include "Camera/PlayerCameraAnim.h"
#include "Camera/PlayerCameraInterface.h"
#include "Camera/OSEPlayerCameraManager.h"
#include "Camera/OSECameraActor3P.h"
#include "Character/OSECharacterBase.h"
#include "GameFramework/OSECheatManager.h"
#include "Graphics/Performance/OSEPerformanceTestComponent.h"
#include "Input/OSEInputFunctionLibrary.h"
#include "Online/OSEGameState.h"
#include "PingSystem/OSEPingSystemComponent.h"
#include "Player/OSEPlayerState.h"
#include "Player/AimAssist/OSEAimAssistComponent.h"
#include "Character/OSECharacterMovement.h"

// UE4
#include "CommonInputSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CineCameraActor.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerController)


AOSEPlayerController::AOSEPlayerController(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   PlayerCameraManagerClass = AOSEPlayerCameraManager::StaticClass();
   CheatClass = UOSECheatManager::StaticClass();
   PingSystem = CreateDefaultSubobject<UOSEPingSystemComponent>(TEXT("PingSystem"));
   AimAssist = CreateDefaultSubobject<UOSEAimAssistComponent>(TEXT("AimAssist"));

   // Attach to the pawn by default when possessing. It's super helpful to be able to have
   // the controller attached to the pawn, as we can use it for accurate location checks
   // and to make debugging issues easier.
   bAttachToPawn = true;

   // Default the damping rate for extra rotation input to match the default smooth
   // rotation value from the base player controller class.
   _extraRotationRate = SmoothTargetViewRotationSpeed;

   // Set up first person defaults
   {
      FViewTargetTransitionParams params;
      params.BlendTime = 0.10f;
      params.BlendFunction = VTBlend_EaseOut;
      params.bLockOutgoing = false;
      _cameraSettings1P = FPlayerCameraSettings(params);
   }

   // Set up third person defaults
   {
      FViewTargetTransitionParams params;
      params.BlendTime = 0.25f;
      params.BlendFunction = VTBlend_EaseIn;
      params.bLockOutgoing = true;
      _cameraSettings3P = FPlayerCameraSettings(params, AOSECameraActor3P::StaticClass());
   }
}

// static
AOSEPlayerController* AOSEPlayerController::GetOSEPlayerController(const UObject* contextObj, int index)
{
   return UOSECommon::GetPlayerControllerAtIndex<AOSEPlayerController>(contextObj, index);
}

// static
AOSEPlayerController* AOSEPlayerController::GetLocalOSEPlayerController(const UObject* contextObj)
{
   return UOSECommon::GetLocalPlayerController<AOSEPlayerController>(contextObj);
}

void AOSEPlayerController::PostInitializeComponents()
{
   Super::PostInitializeComponents();

#if OSE_ALLOW_PERFTEST
   // only init the perf test component if we're actually running perf test
   // and don't do it at all in a shipping build
   if (UOSEPerformanceTestComponent::IsRunningPerformanceTest())
   {
      _performanceTestComponent = NewObject<UOSEPerformanceTestComponent>(this, TEXT("PerformanceTestComponent"));
      _performanceTestComponent->RegisterComponent();
   }
#endif
}

// Overridable native event for when play begins for this actor
void AOSEPlayerController::BeginPlay()
{
   Super::BeginPlay();

   _cameraActor3P = SpawnCameraActor(_cameraSettings3P);
}

// Overridable function called whenever this actor is being removed from a level
void AOSEPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (_cameraActor3P != nullptr)
   {
      _cameraActor3P->RemoveTickPrerequisiteActor(this);
      GetWorld()->DestroyActor(_cameraActor3P, false, false);
      _cameraActor3P = nullptr;
   }

   Super::EndPlay(endPlayReason);
}

// Spawns the camera actor for specified settings
ACameraActor* AOSEPlayerController::SpawnCameraActor(const FPlayerCameraSettings& inSettings)
{
   // Local only
   if (!IsLocalPlayerController())
      return nullptr;

   // It's optional
   UClass* const camClass = inSettings.CameraClass;
   if (camClass == nullptr)
      return nullptr;

   FActorSpawnParameters spawnParams;
   spawnParams.Owner = this;
   spawnParams.Instigator = nullptr;
   spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   spawnParams.ObjectFlags |= RF_Transient;

   auto spawnedCam = GetWorld()->SpawnActor<ACameraActor>(camClass, GetFocalLocation(), GetControlRotation(), spawnParams);
   if (spawnedCam != nullptr)
   {
      spawnedCam->AddTickPrerequisiteActor(this);
   }

   return spawnedCam;
}

// Returns the suggested view target for the given player camera mode
AActor* AOSEPlayerController::GetSuggestedViewTarget(const EPlayerCameraMode inMode)
{
   if (inMode == EPlayerCameraMode::ThirdPerson)
   {
      // This mode uses a special actor
      if (_cameraActor3P != nullptr)
         return _cameraActor3P;
   }

   // Fallback to the pawn or spectator we're controlling
   APawn* controlledPawn = GetPawnOrSpectator();
   if (controlledPawn != nullptr)
      return controlledPawn;

   // If not valid fallback to the player controller itself
   return this;
}

// Sets the current camera mode to use for the player controller.
// Returns the previous camera mode.
EPlayerCameraMode AOSEPlayerController::SetPlayerCameraMode(const EPlayerCameraMode newMode)
{
   const EPlayerCameraMode oldMode = GetPlayerCameraMode();
   if (newMode != oldMode)
   {
      _playerCameraMode = newMode;

      if (newMode == EPlayerCameraMode::Default)
      {
         AutoManageActiveCameraTarget(GetSuggestedViewTarget(newMode));
      }
   }
   return oldMode;
}

// Processes player input (immediately after PlayerInput gets ticked) and calls UpdateRotation().
// PlayerTick is only called if the PlayerController has a PlayerInput object.
// Therefore, it will only be called for locally controlled PlayerControllers.
void AOSEPlayerController::PlayerTick(float deltaTime)
{
   // Update extra rotation input after player input
   if (IsLocalPlayerController())
   {
      // The delta is how much we're interpolating to zero. The delta is
      // removed from extra rotation input and applied to actual rotation input.
      const FRotator newRotation = FMath::RInterpTo(_extraRotationInput, FRotator::ZeroRotator, deltaTime, _extraRotationRate);
      const FRotator deltaRotation = (_extraRotationInput - newRotation).GetNormalized();
      _extraRotationInput -= deltaRotation;
      RotationInput += deltaRotation;
   }

   Super::PlayerTick(deltaTime);

   TickPlayerCameraMode(deltaTime);
}

FRotator AOSEPlayerController::CalculateExtraRotationInput(const FRotator& desiredControlRotation) const
{
   const FRotator prevRotation = (GetControlRotation() + _extraRotationInput + RotationInput).GetNormalized();
   const FRotator nextRotation = desiredControlRotation.GetNormalized();
   return (nextRotation - prevRotation).GetNormalized();
}

// Update the camera manager; this is called after all actors have been ticked.
void AOSEPlayerController::UpdateCameraManager(float deltaTime)
{
   // Always update the camera actor location right before updating the camera manager.
   // This ensures that the location used is the most up-to-date in case the pawn
   // moved during the tick. 
   // if the camera is attached, then setting its world position here is redundant, and also causes camera pops
   if (_cameraActor3P != nullptr && _cameraActor3P->GetAttachParentActor() == nullptr)
   {
      _cameraActor3P->SetActorLocationAndRotation(GetFocalLocation(), GetControlRotation());
   }

   Super::UpdateCameraManager(deltaTime);
}

void AOSEPlayerController::SetIgnoreAbilityInput(bool newIgnore)
{
   const int32 delta = newIgnore ? 1 : -1;
   _abilityInputIgnoreCount = FMath::Max(0, _abilityInputIgnoreCount + delta);
}

void AOSEPlayerController::OnPossess(APawn* inPawn)
{
   _extraRotationInput = FRotator::ZeroRotator;

   Super::OnPossess(inPawn);

   if (IsLocalPlayerController())
   {
      const APawn* const controlledPawn = GetPawnOrSpectator();
      if ((controlledPawn != nullptr) && (controlledPawn == GetViewTarget()))
      {
         // If we possess a pawn without an active camera component, default to third person.
         // Otherwise, the camera is likely to be in the wrong location, and the mesh is likely
         // to intersect with the camera.
         if (!controlledPawn->HasActiveCameraComponent())
         {
            SetPlayerCameraMode(EPlayerCameraMode::ThirdPerson);
         }
      }
   }
}

void AOSEPlayerController::OnUnPossess()
{
   if (IsLocalPlayerController())
   {
      const APawn* const controlledPawn = GetPawnOrSpectator();
      if ((controlledPawn != nullptr) && (GetPlayerCameraMode() == EPlayerCameraMode::ThirdPerson))
      {
         // If we unpossess a pawn without an active camera component, and we have explicitly
         // set the mode to third person, then restore the mode to the default value. This is to
         // undo the corresponding logic found in OnPossess()
         if (!controlledPawn->HasActiveCameraComponent())
         {
            SetPlayerCameraMode(EPlayerCameraMode::Default);
         }
      }
   }

   Super::OnUnPossess();
}

// Updates the view target as needed for the specified player camera mode. This method has
// no effect if the player controller isn't locally controlled, or if the camera mode is the default.
void AOSEPlayerController::TickPlayerCameraMode(float deltaTime)
{
   if (!IsLocalPlayerController())
      return;

   // if we are running perf test don't mess w/ the camera
   if (_performanceTestComponent)
      return;

   bool notifyCameraCut = false;

   // We always need to update the camera actor in case we switch to it
   if (_cameraActor3P != nullptr)
   {
      // We use the instigator of the camera to compare our pawn.
      // When this value changes, we trigger a camera cut.
      APawn* const currPawn = _cameraActor3P->GetInstigator();
      APawn* const nextPawn = GetPawnOrSpectator();
      if (nextPawn != currPawn)
      {
         // Only trigger the cut when one of the values is null
         if ((nextPawn == nullptr) || (currPawn == nullptr))
         {
            notifyCameraCut = true;
         }

         _cameraActor3P->SetInstigator(nextPawn);
      }

      // Update and attach to the appropriate parent actor. It is possible to use the
      // controlled pawn or the controller itself as the parent actor.
      // When this value changes, we trigger a camera cut.
      AActor* const currParentActor = _cameraActor3P->GetAttachParentActor();
      AActor* const nextParentActor = this;
      if (nextParentActor != currParentActor)
      {
         notifyCameraCut = true;

         if (currParentActor != nullptr)
         {
            _cameraActor3P->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
         }

         if (nextParentActor != nullptr)
         {
            _cameraActor3P->AttachToActor(nextParentActor, FAttachmentTransformRules::KeepWorldTransform);
         }
      }

      // Always set to the focal location and the control rotation, if the camera is not attached to another actor
      if (_cameraActor3P->GetAttachParentActor() != nullptr)
      {
         _cameraActor3P->SetActorLocationAndRotation(GetFocalLocation(), GetControlRotation());
      }
    
   }

   // If we're not overriding the camera mode, then we don't want to modify any view target settings
   const EPlayerCameraMode currCameraMode = GetPlayerCameraMode();
   if (currCameraMode != EPlayerCameraMode::Default)
   {
      // Compare the view target and change it if needed.
      // When this value changes, we trigger a camera cut.
      AActor* const currViewTarget = GetViewTarget();
      AActor* const nextViewTarget = GetSuggestedViewTarget();

      // external view is not owned by the controller or pawn (eg a cinematic or similar view override)
      const bool isExternalView = currViewTarget && currViewTarget != this && currViewTarget != _cameraActor3P && currViewTarget != GetPawnOrSpectator();
      if (!isExternalView && nextViewTarget != currViewTarget)
      {
         notifyCameraCut = true;

         FViewTargetTransitionParams params;
         if (currCameraMode == EPlayerCameraMode::FirstPerson)
            params = _cameraSettings1P.TransitionParams;
         else if (currCameraMode == EPlayerCameraMode::ThirdPerson)
            params = _cameraSettings3P.TransitionParams;
         else
         {
            unimplemented();
         }

         SetViewTarget(nextViewTarget, params);
      }
   }

   if (_cameraActor3P != nullptr)
   {
      // Trigger the camera cut notification if needed. We do this last so that any
      // code that might run has the most up-to-date position, rotation, instigator,
      // and attachment info. Camera components call the camera actor version of
      // NotifyCameraCut(), so we start with components.
      if (notifyCameraCut)
      {
         _cameraActor3P->GetCameraComponent()->NotifyCameraCut();
      }
   }
}

// Set the control rotation. The RootComponent's rotation will also be updated to match it if RootComponent->bAbsoluteRotation is true.
void AOSEPlayerController::SetControlRotation(const FRotator& newRotation)
{
   Super::SetControlRotation(newRotation);

   // Make sure the camera actors stay in sync with the control rotation
   if (IsLocalPlayerController() && (_cameraActor3P != nullptr))
   {
      _cameraActor3P->SetActorRotation(GetControlRotation());
   }
}

// Updates the rotation of player, based on ControlRotation after RotationInput has been applied.
// This may then be modified by the PlayerCamera, and is passed to Pawn->FaceRotation().
void AOSEPlayerController::UpdateRotation(float deltaTime)
{
   AOSECharacterBase* characterOSE = Cast<AOSECharacterBase>(GetPawn());
   UOSECharacterMovement* moveComp = IsValid(characterOSE) ? Cast<UOSECharacterMovement>(characterOSE->GetMovementComponent()) : NULL;
   if (IsValid(moveComp))
   {
      moveComp->SetRotationInputThisFrame(RotationInput);
   }

   if (IsLocalPlayerController())
   {
      // Aim assist
      if (AimAssist != nullptr)
      {
         const FRotator aimAssistDelta = AimAssist->ProcessAimAssist(*this, RotationInput, deltaTime);
         RotationInput += aimAssistDelta;
      }
   }

   Super::UpdateRotation(deltaTime);
}

// Enables cheats within the game
void AOSEPlayerController::EnableCheats()
{
#   if (OSE_CHEATS_FORCE_ON)
   {
      // Force add cheats if desired
      AddCheats(true);
   }
#   elif (OSE_CHEATS_ENABLED)
   {
      // Fall back to default behavior. This adds
      // cheats if we're not in shipping or test
      Super::EnableCheats();
   }
#   else
   {
      // Cheats are not enabled; do nothing
   }
#   endif
}

void AOSEPlayerController::GetSeamlessTravelActorList(bool toEntry, TArray<class AActor*>& actorList)
{
   Super::GetSeamlessTravelActorList(toEntry, actorList);
   actorList.AddUnique(_cameraActor3P);
}

void AOSEPlayerController::NotifyLoadedWorld(FName worldPackageName, bool finalDest)
{
   // @TODO: Revaluate if these assumptions still hold true!
   
   // Intentionally not calling Super:: 
   // - It changes the view target to the pc instead of leaving it on the pawn like we want it
   // - It attempts to move the pc to a player start, which we don't want to
   //Super::NotifyLoadedWorld(worldPackageName, finalDest);
}

void AOSEPlayerController::SetPlayer(UPlayer* player)
{
   if (UCommonInputSubsystem* inputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
   {
      inputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
   }

   Super::SetPlayer(player);

   if (UCommonInputSubsystem* inputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
   {
      inputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::OnInputMethodChanged);
      OnInputMethodChanged(inputSubsystem->GetCurrentInputType());
   }
}

void AOSEPlayerController::SetPawn(APawn* inPawn)
{
   const bool broadcastMsg = inPawn != GetPawn();

   Super::SetPawn(inPawn);

   if (broadcastMsg)
   {
      OnPawnChanged.Broadcast(GetPawn());
   }
}

void AOSEPlayerController::OnRep_Pawn()
{
   if (IsLocalController())
   {
      if (AOSECharacterBase* oldCharacter = Cast<AOSECharacterBase>(_oldPawn.Get()))
      {
         // this is the client version of this event, see AOSECharacterBase::UnPossessed() for the server version
         oldCharacter->ReceiveLocalUnpossessed(this);
      }
   }

   Super::OnRep_Pawn();

   _oldPawn = GetPawn();

   // need special handling for this since the Pawn variable is already set
   // assumes that the pawn actually changed, which is probably fine
   OnPawnChanged.Broadcast(GetPawn());

   if (IsLocalController())
   {
      if (AOSECharacterBase* inCharacter = Cast<AOSECharacterBase>(GetPawn()))
      {
         // this is the client version of this event, see AOSECharacterBase::PossessedBy() for the server version
         inCharacter->ReceiveLocalPossessed(this);
      }
   }
}

void AOSEPlayerController::InitPlayerState()
{
   APlayerState* oldPS = PlayerState;

   Super::InitPlayerState();

   if (PlayerState != oldPS)
   {
      OnPlayerStateChanged.Broadcast(PlayerState);
   }
}

void AOSEPlayerController::OnRep_PlayerState()
{
   Super::OnRep_PlayerState();
   OnPlayerStateChanged.Broadcast(PlayerState);
}

AOSEPlayerState* AOSEPlayerController::GetOSEPlayerState() const
{
   return GetPlayerState<AOSEPlayerState>();
}

bool AOSEPlayerController::InputKey(const FInputKeyParams& params)
{
   bool result = Super::InputKey(params);

   // count up mouse samples to be used with our custom enhanced input mouse smoothing modifier
   // ASSUMPTION: MouseX and MouseY have an identical # of samples so we only have to count one of them
   if (params.Key == EKeys::MouseX)
   {
      _mouseNumSamples += params.NumSamples;
   }

   return result;
}

void AOSEPlayerController::OnInputMethodChanged(ECommonInputType inputType)
{
   if (inputType == ECommonInputType::MouseAndKeyboard)
   {
      _inputHardwareType = EOSEInputHardwareType::KeyboardMouse;
   }
   else if (inputType == ECommonInputType::Gamepad)
   {
      _inputHardwareType = EOSEInputHardwareType::Gamepad;
   }

   OnInputHardwareTypeChanged.Broadcast(_inputHardwareType);
}
