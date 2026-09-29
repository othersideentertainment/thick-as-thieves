// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// todo: sort includes by project

#include "Character/OSECharacterBase.h"
#include "Character/OSECharacterMovement.h"
#include "Character/OSEFootstepSimulatorComponent.h"
#include "VoiceOver/OSEVoiceOverTriggers.h"

#include "Combat/CombatComponent.h"

#include "Items/ItemFunctionLibrary.h"
#include "Items/ToolSetComponent.h"

#include "Online/OSEGameState.h"
#include "Player/OSEPlayerState.h"
#include "Player/OSEPlayerStats.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/OSEEnhancedInputPriority.h"
#include "Input/OSEInputSettings.h"
#include "InputMappingContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEGameplayAbilitySet.h"
#include "Abilities/OSESyncedAnimations.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Abilities/Effects/OSEGameplayEffectSet.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "GameplayEffectExtension.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "OSECoreCollision.h"
#include "OSECommon.h"
#include "OSEProjectSettings.h"

#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Engine/LocalPlayer.h"

//ue includes
#include "Misc/DataValidation.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECharacterBase)

DEFINE_LOG_CATEGORY(LogOSECharacter);

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_WallClimbing_Dashing, "Status.WallClimbing.Dashing");

namespace CharacterHelpers
{
   static void RemoveMovementModeTag(const UOSEProjectSettings& settings, UAbilitySystemComponent* asc, EMovementMode mode, uint8 customMode)
   {
      FGameplayTag removedTag = settings.GetTagForMovementMode(mode, customMode);
      if (removedTag.IsValid())
      {
         asc->RemoveLooseGameplayTag(removedTag);
      }
   }


   static void AddMovementModeTag(const UOSEProjectSettings& settings, UAbilitySystemComponent* asc, EMovementMode mode, uint8 customMode)
   {
      FGameplayTag addedTag = settings.GetTagForMovementMode(mode, customMode);
      if (addedTag.IsValid())
      {
         asc->AddLooseGameplayTag(addedTag);
      }
   }
}


const FName AOSECharacterBase::ToolSetComponentName(TEXT("InventoryComponent"));
const FName AOSECharacterBase::CombatComponentName(TEXT("CombatComponent"));
const FName AOSECharacterBase::ItemInventoryComponentName(TEXT("ItemInventoryComponent"));

// Sets default values
AOSECharacterBase::AOSECharacterBase(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer.SetDefaultSubobjectClass<UOSECharacterMovement>(ACharacter::CharacterMovementComponentName))
   , _scrambleRequestCounter(0)
   , _isScrambling(false)
   , SprintRequestCounter(0)
   , bIsSprinting(false)
   , _mantleRequestCounter(0)
   , _avatarScale(1)
{
   // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true;

   // Return the material in movement hit results for all characters. This is needed for audio to play correct sound effects
   // based on the physical material the character is moving on.
   GetCapsuleComponent()->bReturnMaterialOnMove = true;

   // Create ability system component, and set it to be explicitly replicated
   AbilitySystemComponent = CreateOptionalDefaultSubobject<UOSEAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

   if (AbilitySystemComponent)
   {
      bCreatedStaticAbilityComponents = true;
      AbilitySystemComponent->SetIsReplicated(true);
      AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
   }

   // Create the attribute set, this replicates by default. Adding it as a subobject of the owning actor
   // of an AbilitySystemComponent automatically registers the AttributeSet with the AbilitySystemComponent
   BaseAttributeSet = CreateOptionalDefaultSubobject<UAttributeBaseSet>(TEXT("BaseAttributeSet"));

   // Create tool set component
   ToolSetComponent = CreateDefaultSubobject<UToolSetComponent>(AOSECharacterBase::ToolSetComponentName);

   // Create physical animation component
   PhysicalAnimation = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));

   // Create Combat component
   CombatComponent = CreateDefaultSubobject<UCombatComponent>(AOSECharacterBase::CombatComponentName);

   // Create the Footstep component
   _footstepComponent = CreateDefaultSubobject<UOSEFootstepSimulatorComponent>(TEXT("FootstepComponent"));
}

void AOSECharacterBase::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   // OSEEnhancedInputComponent loads the project default versions of input mapping contexts, but for any character-specific
   // mapping contexts assigned to us we have we need to load them ourselves (since they are soft ptrs in the struct to share w/ project settings)
   for (const FOSEInputContextPriority& context : DefaultInputMappingContexts)
   {
      _loadedInputMappingContexts.AddUnique(context.Context.LoadSynchronous());
   }
}

// Overridable native event for when play begins for this actor
void AOSECharacterBase::BeginPlay()
{
   Super::BeginPlay();

   if (bCreatedStaticAbilityComponents)
   {
      InitializeAbilities(AbilitySystemComponent, BaseAttributeSet);
   }
}

void AOSECharacterBase::Destroyed()
{
   // if we are being destroyed, and are inhibiting movement on our controller, free that state up now
   // NOTE: OnUnpossess() happens further down in the stack and I am intentionally not putting this there -- we can
   // potentially be unpossessed but want to maintain the movement restriction(s)
   if (_isMovementInhibited)
   {
      _isMovementInhibited = false;
      OnMovementIsInhibitedChanged(_isMovementInhibited);
   }

   UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
   if (ASC)
   {
      // Clear our abilities, this is needed because the component may exist on another actor
      ResetAbilities();
   }

   Super::Destroyed();
}

// Overridable function called whenever this actor is being removed from a level
void AOSECharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   Super::EndPlay(EndPlayReason);
}

void AOSECharacterBase::Restart()
{
   Super::Restart();

   if (AbilitySystemComponent != nullptr)
   {
      AbilitySystemComponent->RefreshAbilityActorInfo();
   }
}

void AOSECharacterBase::EnableInput(APlayerController* playerController)
{
   Super::EnableInput(playerController);
}

void AOSECharacterBase::DisableInput(APlayerController* playerController)
{
   Super::DisableInput(playerController);

   // While input is disabled we no longer receive SprintRequest() and SprintCancel()... so in the case where:
   // - Player holds shift to sprint
   // - We disable input (like in an enemy takedown ability)
   // - Player releases sprint
   // - We re-enable input
   // If the shift button is lifted while input was disabled, we miss the event and don't have a way to get our state synced back up.
   // This may not be a real bug in the shipping title because we may not have sprint-required abilties that also disable input, but we do right now.
   // So for the moment we'll just turn sprint off when input is disabled
   SprintCancel();
}

void AOSECharacterBase::OnMovementModeChanged(EMovementMode prevMovementMode, uint8 previousCustomMode)
{
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

      CharacterHelpers::RemoveMovementModeTag(settings, asc, prevMovementMode, previousCustomMode);
      const UCharacterMovementComponent* cmc = GetCharacterMovement();
      CharacterHelpers::AddMovementModeTag(settings, asc, cmc->MovementMode, cmc->CustomMovementMode);
   }

   Super::OnMovementModeChanged(prevMovementMode, previousCustomMode);
}

#if WITH_EDITOR
EDataValidationResult AOSECharacterBase::IsDataValid(class FDataValidationContext& Context) const 
{
   EDataValidationResult result = Super::IsDataValid(Context);

   // validate that the animations belong to our skeleton
   if (_syncedAnimationCharacterMontageAsset)
   {
      _syncedAnimationCharacterMontageAsset->ValidateForCharacter(this, Context);
      if (Context.GetNumWarnings() || Context.GetNumErrors() )
      {
         result = EDataValidationResult::Invalid;
      }
   }
   return result;
}
#endif

// Update the character
void AOSECharacterBase::Tick(float deltaTime)
{
   if (_cancelSprintOnNextTick)
   {
      SprintCancel();
      _cancelSprintOnNextTick = false;
   }

   Super::Tick(deltaTime);
   _TickRagdoll();

   if (GetLocalRole() != ROLE_SimulatedProxy)
   {
      _TickStationarySprintTimer(deltaTime);
   }

   if(IsLocallyControlled())
   {
      _TickIsCharacterReady();
      
      // Could register an attribute change listener, but this is simpler, and marginally
      // less overhead for non-locally-controlled actors. Could move if computation becomes
      // nontrivial.
      if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
      {
         movementComp->SetMaxSpeedMultiplier(GetMovementMaxSpeedMultiplierToApply());
         movementComp->SetExtraGravityScale(GetGravityScale());
      }
   }
   else if (GetLocalRole() == ROLE_SimulatedProxy)
   {
      // Since this is based on the same value based on the client, this could diverge more
      // in timing, but it avoids replicating a separate value. If ticking this becomes expensive,
      // consider using a listener.
      if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
      {
         movementComp->SetExtraGravityScale(GetGravityScale());
      }
   }

   // Always set friction and braking deceleration - we want this applied on everything
   if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->SetFrictionMultiplier(GetMovementFrictionMultiplierToApply());
      movementComp->SetBrakingDecelerationMultiplier(GetMovementBrakingDecelerationMultiplierToApply());
   }
}

// Called to bind functionality to input
void AOSECharacterBase::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
   Super::SetupPlayerInputComponent(playerInputComponent);

   // Bind ability events, this may also happen in initialize abilities
   if (AbilitySystemComponent != nullptr)
   {
      AbilitySystemComponent->BindToInputComponent(playerInputComponent);
   }

   const UOSEInputDeveloperSettings& inputSettings = UOSEInputDeveloperSettings::Get();
   UOSECharacterInputActionsAsset* inputActionsAsset = InputActionsAsset ? InputActionsAsset : inputSettings.DefaultCharacterInputActionsAsset.Get();
   UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(playerInputComponent);
   if (enhancedInputComponent && inputActionsAsset)
   {
      // Move/look
      enhancedInputComponent->BindAction(inputActionsAsset->Move, ETriggerEvent::Triggered, this, &AOSECharacterBase::_OnMoveInput);
      enhancedInputComponent->BindAction(inputActionsAsset->LookMouseKeyboard, ETriggerEvent::Triggered, this, &AOSECharacterBase::_OnLookInput);
      enhancedInputComponent->BindAction(inputActionsAsset->LookGamepad, ETriggerEvent::Triggered, this, &AOSECharacterBase::_OnLookInput);

      // Bind sprint events
      // NB: Run is for Keyboard/Mouse, and RunToggle is for Gamepad. For now, we want these to have the same UX, but we can differentiate them later
      enhancedInputComponent->BindAction(inputActionsAsset->Run, ETriggerEvent::Started, this, &AOSECharacterBase::_SprintToggleStarted);
      enhancedInputComponent->BindAction(inputActionsAsset->Run, ETriggerEvent::Completed, this, &AOSECharacterBase::_SprintToggleCompleted);
      enhancedInputComponent->BindAction(inputActionsAsset->RunToggle, ETriggerEvent::Started, this, &AOSECharacterBase::_SprintToggleStarted);
      enhancedInputComponent->BindAction(inputActionsAsset->RunToggle, ETriggerEvent::Completed, this, &AOSECharacterBase::_SprintToggleCompleted);

      // Setup some bound action values so that other systems can query their state without binding for callbacks
      enhancedInputComponent->BindActionValue(inputActionsAsset->Move);
      enhancedInputComponent->BindActionValue(inputActionsAsset->LookMouseKeyboard);
      enhancedInputComponent->BindActionValue(inputActionsAsset->LookGamepad);
   }
}


//---------------------------------------------------------------------------------------
// IAbilitySystemInterface
//---------------------------------------------------------------------------------------

// Returns the ability system component to use for this actor. It may live on another actor, such as a Pawn using the PlayerState's component
UAbilitySystemComponent* AOSECharacterBase::GetAbilitySystemComponent() const
{
   return AbilitySystemComponent;
}

UOSEAbilitySystemComponent* AOSECharacterBase::GetAbilitySystemComponentFromActor() const
{
   return Cast<UOSEAbilitySystemComponent>(GetAbilitySystemComponent());
}

// Triggers ability based on input mapping. Usually this happens automatically, but can be
// manually triggered like this from code or blueprints for non-player controlled characters.
bool AOSECharacterBase::TryActivateAbilityByInputID(EAbilityInputType inputCommand, float holdDuration /* = 0.0f*/)
{
   if (UOSEAbilitySystemComponent* asc = GetAbilitySystemComponentFromActor())
   {
      return asc->TryActivateAbilityByInputID<>(inputCommand, holdDuration);
   }

   // No ability system
   return false;
}

bool AOSECharacterBase::TryConfirmAbility()
{
   if (UOSEAbilitySystemComponent* asc = GetAbilitySystemComponentFromActor())
   {
      asc->LocalInputConfirm();
      return true;
   }

   // No ability system
   return false;
}

//---------------------------------------------------------------------------------------
// IAttributeBaseInterface
//---------------------------------------------------------------------------------------

// Returns the base attribute set
UAttributeBaseSet* AOSECharacterBase::GetBaseAttributeSet() const
{
   return BaseAttributeSet;
}

// IAttributeBaseSystemInterface
TScriptInterface< IAttributeBaseInterface > AOSECharacterBase::GetBaseAttributeInterface() const
{
   return GetBaseAttributeSet();
}

// IAttributeBaseInterface (Health)
float AOSECharacterBase::GetHealth() const { return BaseAttributeSet?BaseAttributeSet->GetHealth() : 0.0f; }
float AOSECharacterBase::GetHealthMax() const { return BaseAttributeSet?BaseAttributeSet->GetHealthMax() : 1.0f; }
float AOSECharacterBase::GetHealthPercent() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthPercent() : 0.0f; }
float AOSECharacterBase::GetHealthRegenRate() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthRegenRate() : 0.0f; }

// IAttributeBaseInterface (Energy)
float AOSECharacterBase::GetEnergy() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergy() : 0.0f; }
float AOSECharacterBase::GetEnergyMax() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyMax() : 1.0f; }
float AOSECharacterBase::GetEnergyPercent() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyPercent() : 0.0f; }
float AOSECharacterBase::GetEnergyRegenRate() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyRegenRate() : 0.0f; }

// IAttributeBaseInterface (Damage)
float AOSECharacterBase::GetHealthDamage() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthDamage() : 0.0f; }
float AOSECharacterBase::GetAttackDamage() const { return BaseAttributeSet ? BaseAttributeSet->GetAttackDamage() : 0.0f; }
float AOSECharacterBase::GetAttackDamageMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetAttackDamageMultiplier() : 0.0f; }
float AOSECharacterBase::GetDamageReductionMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetDamageReductionMultiplier() : 0.0f; }

// IAttributeBaseInterface (Movement)
float AOSECharacterBase::GetMovementMaxSpeedMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementMaxSpeedMultiplier() : 1.0f;  }
float AOSECharacterBase::GetMovementFrictionMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementFrictionMultiplier() : 1.0f;  }
float AOSECharacterBase::GetMovementBrakingDecelerationMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementBrakingDecelerationMultiplier() : 1.0f;  }
float AOSECharacterBase::GetGravityScale() const { return BaseAttributeSet ? BaseAttributeSet->GetGravityScale() : 1.0f; }

void AOSECharacterBase::PossessedBy(AController* newController)
{
   AController* const oldController = Controller;
   const bool isNewController = newController != oldController;

   Super::PossessedBy(newController);

   // Update avatar
   if (AbilitySystemComponent != nullptr)
   {
      AbilitySystemComponent->SetAvatarActor(this);
   }

   if (isNewController && newController && newController->IsLocalController())
   {
      // this is the server version of this event, see AOSEPlayerController::OnRep_Pawn() for the client version
      ReceiveLocalPossessed(newController);
   }

   if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->PossessedBy(newController);
   }

   OnPossessedBy.Broadcast(newController);
}

void AOSECharacterBase::UnPossessed()
{
   AController* const oldController = Controller;

   Super::UnPossessed();

   if (oldController && oldController->IsLocalController())
   {
      // this is the server version of this event, see AOSEPlayerController::OnRep_Pawn() for the client version
      ReceiveLocalUnpossessed(oldController);
   }

   if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->UnPossessed();
   }
}

void AOSECharacterBase::OnRep_Controller()
{
   Super::OnRep_Controller();

   // Our controller changed, must update ActorInfo on AbilitySystemComponent
   if (AbilitySystemComponent != nullptr)
   {
      // Check to see whether RefreshAbilityActorInfo() would have set
      // a null PlayerController before allowing it. There's a mess of
      // logic in there, but in this case it boils down to just GetController()
      // on this actor.
      if (Cast<APlayerController>(GetController()))
      {
         AbilitySystemComponent->RefreshAbilityActorInfo();
      }
   }
}


//-------------------------------------------------------------------------------------------------
// Movement/facing
//-------------------------------------------------------------------------------------------------

// Returns the aim/camera rotation. Use the default if we have a controller; otherwise use the replicated value
FRotator AOSECharacterBase::GetBaseAimRotation() const
{
   if (GetController() != nullptr)
      return Super::GetBaseAimRotation();
   else
      return RemoteBaseAimRotation;
}

// Returns the view rotation (direction they are looking, normally Controller->ControlRotation).
// Use the default if we have a controller; otherwise use the replicated value
FRotator AOSECharacterBase::GetViewRotation() const
{
   if (GetController() != nullptr)
      return Super::GetViewRotation();
   else
      return RemoteViewRotation;
}

// Returns unit movement axis direction in world space.
FVector AOSECharacterBase::GetMovementAxisForward() const
{
   FVector Axis = GetViewRotation().RotateVector(FVector::ForwardVector);

   // The character remains vertical by default if moving on the ground
   // or falling. Adding a conditional check here helps ensure the other
   // movement modes support full motion
   if (const UCharacterMovementComponent* movementComponent = GetCharacterMovement())
   {
      if (movementComponent->ShouldRemainVertical())
      {
         Axis.Z = 0.0f;
      }
   }

   return Axis.GetSafeNormal();
}

// Returns unit movement axis direction in world space
FVector AOSECharacterBase::GetMovementAxisRight() const
{
   FVector Axis = GetViewRotation().RotateVector(FVector::RightVector);

   // The character remains vertical by default if moving on the ground
   // or falling. Adding a conditional check here helps ensure the other
   // movement modes support full motion
   if (const UCharacterMovementComponent* movementComponent = GetCharacterMovement())
   {
      if (movementComponent->ShouldRemainVertical())
      {
         Axis.Z = 0.0f;
      }
   }

   return Axis.GetSafeNormal();
}

// Returns unit movement axis direction in world space
FVector AOSECharacterBase::GetMovementAxisUp() const
{
   FVector Axis = GetViewRotation().RotateVector(FVector::UpVector);
   return Axis.GetSafeNormal();
}

void AOSECharacterBase::_OnMoveInput(const FInputActionValue& value)
{
   // right
   const float& rightInput = value[0];
   AddMovementInputRight(rightInput);
   
   // fwd
   const float& fwdInput = value[1];
   AddMovementInputForward(fwdInput);
}

void AOSECharacterBase::_OnLookInput(const FInputActionValue& value)
{
   // yaw
   const float& yawInput = value[0];
   AddControllerYawInput(yawInput);

   // pitch
   const float& pitchInput = value[1];
   AddControllerPitchInput(pitchInput);
}

//---------------------------------------------------------------------------------------
// Enhanced Input
//---------------------------------------------------------------------------------------

void AOSECharacterBase::PawnClientRestart()
{
   Super::PawnClientRestart();

   if (APlayerController* pc = Cast<APlayerController>(GetController()))
   {
      // Get the Enhanced Input Local Player Subsystem from the Local Player related to our Player Controller.
      if (UEnhancedInputLocalPlayerSubsystem* enhancedInputSubsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
      {
         // PawnClientRestart can run more than once in an Actor's lifetime, so start by clearing out any leftover mappings.
         enhancedInputSubsys->ClearAllMappings();

         const UOSEInputDeveloperSettings& inputSettings = UOSEInputDeveloperSettings::Get();
         const TArray<FOSEInputContextPriority>& defaultInputMappingContexts = 
            DefaultInputMappingContexts.Num() > 0 ? DefaultInputMappingContexts : inputSettings.DefaultCharacterInputMappingContexts;

         // Add each mapping context, along with their priority values. Higher values outprioritize lower values.
         for (const FOSEInputContextPriority& context : defaultInputMappingContexts)
         {
            FModifyContextOptions Opts = {};
            Opts.bNotifyUserSettings = true;
            enhancedInputSubsys->AddMappingContext(context.Context.Get(), static_cast<int32>(context.Priority), Opts);
         }

         // Rebuild immediately so we can start querying keys in the UI
         FModifyContextOptions opts;
         opts.bForceImmediately = true;
         opts.bIgnoreAllPressedKeysUntilRelease = true;
         enhancedInputSubsys->RequestRebuildControlMappings(opts);
      }
   }

   // As the pawn has potentially changed and a lot comes along with this, reset this flag
   // to force us back into ready checking to make sure all the bits required are correctly
   // in place (see _TickIsCharacterReady).
   _isCharacterReady = false;

   OnPawnClientRestart.Broadcast();
}

//---------------------------------------------------------------------------------------
// Character Ready
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::IsCharacterReady() const
{
   if (IsLocallyControlled())
   {
      return _isCharacterReady;
   }
   
   // ASSUMPTION: remote characters are never ready, which matches behavior in ToolSet/Tool components
   return false;
}

void AOSECharacterBase::_TickIsCharacterReady()
{
   if (IsLocallyControlled() && !_isCharacterReady)
   {
      // requires a player controller
      APlayerController* pc = Cast<APlayerController>(GetController());
      if (!pc)
         return;

      // requires a player state
      APlayerState* ps = GetPlayerState();
      if (!ps)
         return;
      
      // requires tool set to be replicated and ready
      if (!ToolSetComponent->IsReady())
         return;

      // requires ability system component
      UOSEAbilitySystemComponent* asc = GetAbilitySystemComponentFromActor();
      if (!asc)
         return;

      // requires initial abilities granted
      for (UOSEGameplayAbilitySet* abilitySet : InitialAbilitySets)
      {
         if (!abilitySet)
            continue;

         if (!abilitySet->IsReady(asc, this))
            return;
      }

      // requires initial input setup if we have an enhanced input subsystem
      if (UEnhancedInputLocalPlayerSubsystem* enhancedInputSubsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
      {
         for (const FOSEInputContextPriority& context : DefaultInputMappingContexts)
         {
            if (!enhancedInputSubsys->HasMappingContext(context.Context.Get()))
               return;
         }
      }

      // passed all our checks, we are ready!
      _isCharacterReady = true;

      // cpp subclasses
      _OnIsCharacterReadyChanged(_isCharacterReady);

      // bp subclasses
      _BPOnIsCharacterReadyChanged(_isCharacterReady);

      // external bindings
      OnCharacterReady.Broadcast(this);
   }
}

//---------------------------------------------------------------------------------------
// IOSETeamInterface
//---------------------------------------------------------------------------------------

uint8 AOSECharacterBase::GetTeam() const
{
   // default impl knows how to fetch the team from the player state
   if (const IOSETeamInterface* ps = GetPlayerState<IOSETeamInterface>())
   {
      return ps->GetTeam();
   }
   return IOSETeamInterface::kInvalidTeam;
}

//---------------------------------------------------------------------------------------
// Ragdoll
//---------------------------------------------------------------------------------------

void AOSECharacterBase::SetRagdollingParams(const FOSERagdollParams& params)
{
   _ragdollParams = params;
   #if UE_WITH_IRIS   
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, _ragdollParams, this);
   #endif
}

void AOSECharacterBase::SetRagdollingParamsAndActivate(const FOSERagdollParams& params)
{
   SetRagdollingParams(params);
   SetRagdollActive(true);
}

void AOSECharacterBase::SetRagdollActive(bool active)
{
   if (HasAuthority() && active && _ragdollParams.Duration >= 0)
   {
      _ragdollParams.ServerEndTime = GetWorld()->GetTimeSeconds() + _ragdollParams.Duration;
   }

   _ragdollParams.BoneSimulationParams.Active = active;
   _ApplyRagdollParams();
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, _ragdollParams, this);
   #endif
}

void AOSECharacterBase::OnRep_RagdollParams()
{
   _ApplyRagdollParams();
}

void AOSECharacterBase::_ApplyRagdollParams()
{
   _ragdollParams.ApplyProperties(PhysicalAnimation);
   _ragdollParams.ApplyEnabled(GetMesh());
   #if UE_WITH_IRIS   
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, _ragdollParams, this);
   #endif
}

void AOSECharacterBase::_TickRagdoll()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSECharacterBase_TickRagdoll);

   // if we're ragdolling w/ a duration....
   if (_ragdollParams.BoneSimulationParams.Active &&
      _ragdollParams.Duration >= 0 &&
       _ragdollParams.ServerEndTime >= 0)
   {
      // require gs for server time
      AOSEGameState* gs = AOSEGameState::GetOSEGameState(this);
      if (!gs)
         return;

      const float serverTimeNow = gs->GetServerWorldTimeSeconds();

      // ramp the physics vs authored anim blend weight according to the defined curve
      float blendWeight = 1.0f;
      if (_ragdollParams.BlendWeightOverTime)
      {
         const float serverStartTime = _ragdollParams.ServerEndTime - _ragdollParams.Duration;
         float pctComplete = FMath::GetMappedRangeValueClamped(FVector2D(serverStartTime, _ragdollParams.ServerEndTime), FVector2D(0.0f, 1.0f), serverTimeNow);
         blendWeight = _ragdollParams.BlendWeightOverTime->GetFloatValue(pctComplete);
      }
      
      // apply blend weight
      _ragdollParams.SetBlendWeight(GetMesh(), blendWeight);
      #if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, _ragdollParams, this);
	  #endif
      // turn it off when it's done in server-time
      if (serverTimeNow >= _ragdollParams.ServerEndTime)
      {
         SetRagdollActive(false);
      }
   }
}


//---------------------------------------------------------------------------------------
// IToolHolderInterface
//---------------------------------------------------------------------------------------

USceneComponent* AOSECharacterBase::GetToolRoot(EMeshPerspective MeshPerspective) const
{
   return GetMesh();
}


//---------------------------------------------------------------------------------------
// IToolSetSystemInterface
//---------------------------------------------------------------------------------------

TScriptInterface<IToolSetInterface> AOSECharacterBase::GetToolSetInterface() const
{
   check(ToolSetComponent != nullptr);
   return ToolSetComponent;
}


//---------------------------------------------------------------------------------------
// ITraversalInterface (crouching)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::IsCrouching() const
{
   if (const UCharacterMovementComponent* movementComponent = GetCharacterMovement())
   {
      return movementComponent->IsCrouching();
   }

   return false;
}


//---------------------------------------------------------------------------------------
// ITraversalInterface (sliding)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanSlide() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->CanEverSlide() && !IsCarrying();
   }

   return false;
}

bool AOSECharacterBase::IsSliding() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->IsSliding();
   }

   return false;
}

// Event called when the character starts sliding
void AOSECharacterBase::OnStartSliding_Implementation()
{
   // stow our current tool during the slide
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, true);

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusSlidingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->AddLooseGameplayTag(settings.StatusSlidingTag);
      }
   }
}

// Event called when the character stops sliding
void AOSECharacterBase::OnStopSliding_Implementation()
{
   // un-stow our current tool when the slide is complete
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, false);

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusSlidingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->RemoveLooseGameplayTag(settings.StatusSlidingTag);
      }
   }
}


//---------------------------------------------------------------------------------------
// ITraversalInterface (climbing)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanScramble() const
{
   UOSECharacterMovement* charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if(!charMoveComp)
   {
      return false;
   }
   if (!charMoveComp->CanEverScramble() || IsCarrying())
   {
      return false;
   }

   const UAbilitySystemComponent* asc = GetAbilitySystemComponent();
   if(!asc)
   {
      return false;
   }

   return !asc->HasAnyMatchingGameplayTags(_scrambleSuppressionTags);;
}

bool AOSECharacterBase::IsScrambling() const
{
   return _isScrambling;
}

void AOSECharacterBase::StartScrambling()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(_scrambleRequestCounter >= 0);
   ++_scrambleRequestCounter;

   if (auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      charMoveComp->SetWantsToScramble(_scrambleRequestCounter > 0);
   }
}

void AOSECharacterBase::StopScrambling()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   _scrambleRequestCounter = FMath::Max(0, _scrambleRequestCounter - 1);

   if (auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      charMoveComp->SetWantsToScramble(_scrambleRequestCounter > 0);
   }
}

void AOSECharacterBase::OnRep_IsScrambling()
{
   if (auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      if (_isScrambling)
      {
         _scrambleRequestCounter = 1;
         charMoveComp->SetWantsToScramble(true);
         charMoveComp->StartScrambling(true);
      }
      else
      {
         _scrambleRequestCounter = 0;
         charMoveComp->SetWantsToScramble(false);
         charMoveComp->StopScrambling(true);
      }

      charMoveComp->bNetworkUpdateReceived = true;
   }
}

void AOSECharacterBase::OnStartScrambling_Implementation(const FHitResult& initialClimbImpact)
{
   // stow our current tool during the climb
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, true);

   if (auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      // Uncrouch when we start climbing
      charMoveComp->bWantsToCrouch = false;

      // Rotate the controller to face the wall
      if (auto playerController = Cast<AOSEPlayerController>(GetController()))
      {
         const FVector wallDirection = (-initialClimbImpact.Normal).GetSafeNormal2D();
         const FRotator wallRotation = wallDirection.ToOrientationRotator();

         const FOSEScrambleSettings& settings = charMoveComp->GetCurrentScrambleSettings();
         FRotator nextRotation = playerController->GetControlRotation().GetNormalized();
         nextRotation.Pitch = FMath::Max(settings.MaxCameraPitch, nextRotation.Pitch);
         nextRotation.Yaw = wallRotation.Yaw;

         const FRotator deltaRotation = playerController->CalculateExtraRotationInput(nextRotation);
         playerController->AddExtraRotationInput(deltaRotation);
      }
   }
}

void AOSECharacterBase::OnScrambleJump_Implementation(const FHitResult& lastClimbImpact)
{
   // Rotate the controller to face away from the wall
   if (auto playerController = Cast<AOSEPlayerController>(GetController()))
   {
      const FVector wallDirection = lastClimbImpact.Normal.GetSafeNormal2D();
      const FRotator wallRotation = wallDirection.ToOrientationRotator();

      const FRotator deltaRotation = playerController->CalculateExtraRotationInput(wallRotation);
      playerController->AddExtraRotationInput(deltaRotation);
   }
}

void AOSECharacterBase::OnStopScrambling_Implementation(const FHitResult& /*lastClimbImpact*/)
{
   // un-stow our current tool when the climb is complete
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, false);
}

void AOSECharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
   Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

   // Cancel sprinting on crouch iff the character isn't going to start a slide
   auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (charMoveComp && !charMoveComp->CanSlideInCurrentState())
   {
      while (SprintRequestCounter > 0)
      {
         SprintCancel();
      }
   }

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusCrouchingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->AddLooseGameplayTag(settings.StatusCrouchingTag);
      }
   }
}

void AOSECharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
   Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusCrouchingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->RemoveLooseGameplayTag(settings.StatusCrouchingTag);
      }
   }
}

bool AOSECharacterBase::CanJumpInternal_Implementation() const
{
   if (auto charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      // Check if the movement component allows a jump right now
      if (!charMoveComp->CanAttemptJump())
         return false;

      if (charMoveComp->CanEverScrambleJump() && IsScrambling())
      {
         // We're climbing and can jump from a climb. Rely on the movement
         // component's previous validation via CanAttemptJump above.
         return true;
      }

      if (charMoveComp->IsCrouching() && !charMoveComp->GetCrouchJumpSettings().AllowJumpWhileCrouched)
      {
         // We're crouched and can't jump while crouched
         return false;
      }

      // NOTE: The following block was copied from ACharacter::CanJumpInternal_Implementation().
      //       Code style left intact for ease in future comparison.
      {
         // Ensure JumpHoldTime and JumpCount are valid.
         if (!bWasJumping || GetJumpMaxHoldTime() <= 0.0f)
         {
            if (JumpCurrentCount == 0 && charMoveComp->IsFalling())
            {
               return (JumpCurrentCount + 1 < JumpMaxCount);
            }
            else
            {
               return (JumpCurrentCount < JumpMaxCount);
            }
         }
         else
         {
            // Only consider JumpKeyHoldTime as long as:
            // A) The jump limit hasn't been met OR
            // B) The jump limit has been met AND we were already jumping
            const bool bJumpKeyHeld = (bPressedJump && JumpKeyHoldTime < GetJumpMaxHoldTime());
            return bJumpKeyHeld &&
               ((JumpCurrentCount < JumpMaxCount) || (bWasJumping && JumpCurrentCount == JumpMaxCount));
         }
      }
   }

   // Fall back to base implementation, which checks the jump count, movement mode, etc
   return Super::CanJumpInternal_Implementation();
}


//---------------------------------------------------------------------------------------
// ITraversalInterface (sprinting)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanSprint() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->CanEverSprint() && !IsCarrying() && !IsWallClimbing();
   }

   return false;
}

bool AOSECharacterBase::IsSprinting() const
{
   return bIsSprinting;
}

void AOSECharacterBase::SprintRequest()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(SprintRequestCounter >= 0);
   ++SprintRequestCounter;
   
   // For the first request, add an additional counter. This ensures
   // that holding the sprint key down will not remove our request.
   if (_HasAdditionalSprintCount())
   {
      if (SprintRequestCounter == 1)
         ++SprintRequestCounter;
   }

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->bWantsToSprint = (SprintRequestCounter > 0);
   }
}

void AOSECharacterBase::SprintCancel()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   SprintRequestCounter = FMath::Max(0, SprintRequestCounter - 1);

   // If we're letting go of the sprint key, start a timer to check if we begin executing the sprint within a certain time
   // If not, then we want to cancel the additional sprint request
   if (_HasAdditionalSprintCount() && bSprintCanceledIfStationary && SprintRequestCounter == 1)
   {
      _stationarySprintCancelTimer = StationaryTimeToCancelSprint;
   }

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->bWantsToSprint = (SprintRequestCounter > 0);
   }
}

//---------------------------------------------------------------------------------------
// ITraversalInterface (crouching)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanCrouch() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->CanEverCrouch();
   }

   return false;
}

void AOSECharacterBase::CrouchRequest()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(CrouchRequestCounter >= 0);
   ++CrouchRequestCounter;

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->bWantsToCrouch = (CrouchRequestCounter > 0);
   }
}

void AOSECharacterBase::CrouchCancel()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   CrouchRequestCounter = FMath::Max(0, CrouchRequestCounter - 1);

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->bWantsToCrouch = (CrouchRequestCounter > 0);
   }
}

void AOSECharacterBase::_SprintToggleStarted()
{
   // If we are already sprinting, check if we should interpret another tap or press of the button as a cancel
   if (IsSprinting() &&
      ((_sprintInputBehavior == EOSESprintInputBehavior::ContinueSprintAndAllowCancel) || (_sprintInputBehavior == EOSESprintInputBehavior::ContinueSprintAndAllowCancelOnPress))
      )
   {
      if (_sprintInputBehavior == EOSESprintInputBehavior::ContinueSprintAndAllowCancelOnPress)
      {
         // Cancel as soon a the button is pressed
         SprintCancel();
      }
      else
      {
         // Do nothing, wait for button release to cancel
      }
   }
   else
   {
      SprintRequest();
   }
}

void AOSECharacterBase::_SprintToggleCompleted()
{
   // Whether we are canceling an in-progress sprint, stopping an explicit request, or 
   // we should cancel the sprint. If we are canceling an in-progress sprint, then we will still have our sprint count be 1,
   // so this cancel will stop our in-progress sprint
   SprintCancel();
}

void AOSECharacterBase::_TickStationarySprintTimer(float deltaTime)
{
   if (bSprintCanceledIfStationary && _stationarySprintCancelTimer > 0.0f)
   {
      // If we still have one remaining request (the extra request from _HasAdditionalSprintCount)
      if (SprintRequestCounter == 1)
      {
         // If we haven't started sprinting, decrement the timer
         if (!IsSprinting())
         {
            _stationarySprintCancelTimer -= deltaTime;

            // We didn't start the sprint within the required time, so cancel the sprint
            if (_stationarySprintCancelTimer <= 0.0f)
            {
               SprintCancel();
            }
         }
         else
         {
            // We started actually sprinting, stop the timer and don't cancel
            _stationarySprintCancelTimer = 0.0f;
         }
      }
      else
      {
         // Something else affected the request count, meaning our timer is no longer valid
         _stationarySprintCancelTimer = 0.0f;
      }
   }
}

// Handles sprinting replicated from server
void AOSECharacterBase::OnRep_IsSprinting()
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      if (bIsSprinting)
      {
         SprintRequestCounter = _HasAdditionalSprintCount() ? 2 : 1;
         MovementComp->bWantsToSprint = true;
         MovementComp->StartSprinting(true);
      }
      else
      {
         SprintRequestCounter = 0;
         MovementComp->bWantsToSprint = false;
         MovementComp->StopSprinting(true);
      }

      MovementComp->bNetworkUpdateReceived = true;
   }
}

// Event called when the character starts sprinting
void AOSECharacterBase::OnStartSprinting_Implementation()
{
   if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
   {
      // Uncrouch when we start sprinting
      movementComponent->bWantsToCrouch = false;
   }

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusSprintingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->AddLooseGameplayTag(settings.StatusSprintingTag);
      }
   }
}

// Event called when the character stops sprinting
void AOSECharacterBase::OnStopSprinting_Implementation()
{
   // Cancel our last sprint request once we stop sprinting. We stop
   // sprinting once we are no longer able to sprint, which is usually
   // due to the velocity criteria (i.e. we stopped moving forward)
   if (_HasAdditionalSprintCount() && SprintRequestCounter == 1)
   {
      // Defer canceling the sprint to the next tick: This function is potentially called in the middle of a server correction
      // in which case we will potentially undo our correction to bWantsToSprint at the end of ClientUpdatePositionAfterServerUpdate
      // To avoid that, we defer our cancel to the next frame, which ensures that the update sticks
      //
      // NB: This can potentially be called multiple times in a frame due to a combination of:
      //  - _stationarySprintCancelTimer allowing a grace period after releasing the sprint key that still allows sprinting
      //  - updates from the server potentially starting/stopping sprinting multiple times within a frame
      // In those cases, since our SprintRequestCounter will still be 1, it's okay since we'll still issue the final SprintCancel on the next tick
      // Since SprintRequest will only add an additional sprint request if SprintRequestCounter is 1, we will not have a situation where our sprint requests go beyond what's expected
      // So the above situation is rare, but should not break any invariants we have about sprinting
      _cancelSprintOnNextTick = true;
   }

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (settings.StatusSprintingTag.IsValid())
   {
      if (bAreAbilitiesInitialized)
      {
         GetAbilitySystemComponent()->RemoveLooseGameplayTag(settings.StatusSprintingTag);
      }
   }
}

bool AOSECharacterBase::_HasAdditionalSprintCount() const
{
   switch (_sprintInputBehavior)
   {
      case EOSESprintInputBehavior::SprintWhileRequested:
         return false;
      case EOSESprintInputBehavior::ContinueSprint:
         return true;
      case EOSESprintInputBehavior::ContinueSprintAndAllowCancel:
         return true;
      case EOSESprintInputBehavior::ContinueSprintAndAllowCancelOnPress:
         return true;
      default:
         checkNoEntry();
         return false;
   }
}


//---------------------------------------------------------------------------------------
// ITraversalInterface (mantling)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanMantle() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->CanEverMantle() && !IsCarrying();
   }

   return false;
}

bool AOSECharacterBase::IsMantling() const
{
   return GetMantleState().IsMantling || GetLedgeState().IsMounting;
}

void AOSECharacterBase::SetIsMantling(bool inValue)
{
   FOSEMantleState state = GetMantleState();
   state.IsMantling = inValue;
   SetMantleState(state);
}

void AOSECharacterBase::StartMantleAttempt()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(_mantleRequestCounter >= 0);
   ++_mantleRequestCounter;

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetWantsToMantle(_mantleRequestCounter > 0);
   }
}

void AOSECharacterBase::StopMantleAttempt()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   _mantleRequestCounter = FMath::Max(0, _mantleRequestCounter - 1);

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetWantsToMantle(_mantleRequestCounter > 0);
   }
}

void AOSECharacterBase::StartReleaseLedge()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(_ledgeReleaseCounter >= 0);
   ++_ledgeReleaseCounter;

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetWantsToReleaseLedge(_ledgeReleaseCounter > 0);
   }
}

void AOSECharacterBase::StopReleaseLedge()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   _ledgeReleaseCounter = FMath::Max(0, _ledgeReleaseCounter - 1);

   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetWantsToReleaseLedge(_ledgeReleaseCounter > 0);
   }
}

void AOSECharacterBase::OnStartMantling_Implementation()
{
   // stow our current tool during the mantle
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, true);

   // Rotate the controller to face the surface
   if (auto playerController = Cast<AOSEPlayerController>(GetController()))
   {
      const FOSEMantleState& mantleState = GetMantleState();
      check(mantleState.IsValid == true);

      const FRotator wallRotation = mantleState.StartDirection.Rotation();
      const FRotator deltaRotation = playerController->CalculateExtraRotationInput(wallRotation);
      playerController->AddExtraRotationInput(deltaRotation);
   }
}

void AOSECharacterBase::OnStopMantling_Implementation()
{
   // unstow our current tool after the mantle is complete
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, false);
}

// Handles mantling replicated from server. This is only replicated
// for simulated proxies. As input is not replicated to simulated proxies,
// this is how they initiate the mantle process.
void AOSECharacterBase::OnRep_MantleState(const struct FOSEMantleState& prevMantleState)
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      if (prevMantleState.IsMantling != _mantleState.IsMantling)
      {
         if (_mantleState.IsMantling)
         {
            _mantleRequestCounter = 1;
            MovementComp->SetWantsToMantle(true);
            MovementComp->StartMantle(true);
         }
         else
         {
            _mantleRequestCounter = 0;
            MovementComp->SetWantsToMantle(false);
            MovementComp->StopMantle(true);
         }
      }

      MovementComp->bNetworkUpdateReceived = true;
   }
}

//---------------------------------------------------------------------------------------
// ITraversalInterface (Ledge state)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanUseLedges() const
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      return MovementComp->CanEverUseLedges() && !IsCarrying();
   }

   return false;
}
bool AOSECharacterBase::IsOnLedge() const
{
   return GetLedgeState().IsLedgeStateActive;
}

void AOSECharacterBase::SetIsLedgeMode(bool inValue)
{
   check(!IsLedgeMounting());

   FOSELedgeState state = GetLedgeState();
   state.IsLedgeStateActive = inValue;
   SetLedgeState(state);
}

bool AOSECharacterBase::IsLedgeMounting() const
{
   return GetLedgeState().IsMounting;
}
void AOSECharacterBase::SetIsLedgeMounting(bool inValue)
{
   FOSELedgeState state = GetLedgeState();
   state.IsMounting = inValue;
   SetLedgeState(state);
}
void AOSECharacterBase::OnStartLedgeState_Implementation()
{
   // stow our current tool during the mantle
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, true);

   // Rotate the controller to face the surface
   if (auto playerController = Cast<AOSEPlayerController>(GetController()))
   {
      const FOSELedgeState& ledgeState = GetLedgeState();

      const FRotator wallRotation = ledgeState.MountTarget.StartDirection.Rotation();
      const FRotator deltaRotation = playerController->CalculateExtraRotationInput(wallRotation);
      playerController->AddExtraRotationInput(deltaRotation);
   }

   if (UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
      if (settings.StatusLedgeClimbTag.IsValid() && bAreAbilitiesInitialized)
      {
         abilitySystem->SetLooseGameplayTagCount(settings.StatusLedgeClimbTag, 1);
      }
   }
}

void AOSECharacterBase::OnStopLedgeState_Implementation()
{
   // unstow our current tool after the mantle is complete
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, false);

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent();
   if (settings.StatusLedgeClimbTag.IsValid() && IsValid(abilitySystem) && bAreAbilitiesInitialized)
   {
      abilitySystem->SetLooseGameplayTagCount(settings.StatusLedgeClimbTag, 0);
   }
}

// Handles ledge state replicated from server. This is only replicated
// for simulated proxies. As input is not replicated to simulated proxies,
// this is how they initiate the mantle process.
void AOSECharacterBase::OnRep_LedgeState(const struct FOSELedgeState& prevLedgeState)
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      if (prevLedgeState.IsLedgeStateActive != _ledgeState.IsLedgeStateActive)
      {
         if (_ledgeState.IsLedgeStateActive)
         {
            MovementComp->StartLedgeState(true);
         }
         else
         {
            MovementComp->StopLedgeState(true);
         }
      }
      if (prevLedgeState.IsMounting != _ledgeState.IsMounting)
      {
         if (_ledgeState.IsMounting)
         {
            _mantleRequestCounter = 1;
            MovementComp->SetWantsToMantle(true);
            MovementComp->StartLedgeMount(true);
         }
         else
         {
            _mantleRequestCounter = 0;
            MovementComp->SetWantsToMantle(false);
            MovementComp->StopLedgeMount(true);
         }
      }
      MovementComp->bNetworkUpdateReceived = true;
   }
}

//---------------------------------------------------------------------------------------
// ITraversalInterface (Wall Climb)
//---------------------------------------------------------------------------------------

bool AOSECharacterBase::CanWallClimb() const
{
   UOSECharacterMovement* movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (IsValid(movementComp))
   {
      return movementComp->CanEverWallClimb() && !IsCarrying() && !IsOnLedge();
   }

   return false;
}

bool AOSECharacterBase::IsWallClimbing() const
{
   return _isWallClimbing;
}

void AOSECharacterBase::StartWallClimbing()
{
   // Make sure we didn't go negative; if we did someone is likely
   // poking at this value incorrectly
   check(_wallClimbRequestCounter >= 0);
   ++_wallClimbRequestCounter;

   UOSECharacterMovement* charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (IsValid(charMoveComp))
   {
      charMoveComp->SetWantsToWallClimb(_wallClimbRequestCounter > 0);
   }
}

void AOSECharacterBase::StopWallClimbing()
{
   // Decrement the counter, but clamp it at zero. The cancel may happen
   // more frequently than the request, so must be able to handle this case.
   _wallClimbRequestCounter = FMath::Max(0, _wallClimbRequestCounter - 1);

   UOSECharacterMovement* charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (IsValid(charMoveComp))
   {
      charMoveComp->SetWantsToWallClimb(_wallClimbRequestCounter > 0);
   }
}

void AOSECharacterBase::OnRep_IsWallClimbing()
{
   UOSECharacterMovement* charMoveComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (IsValid(charMoveComp))
   {
      if (_isWallClimbing)
      {
         _wallClimbRequestCounter = 1;
         charMoveComp->SetWantsToWallClimb(true);
         charMoveComp->StartWallClimb(true);
      }
      else
      {
         _wallClimbRequestCounter = 0;
         charMoveComp->SetWantsToWallClimb(false);
         charMoveComp->StartWallClimb(true);
      }

      charMoveComp->bNetworkUpdateReceived = true;
   }
}

void AOSECharacterBase::OnStartWallClimbing_Implementation()
{
   if (UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
      if (settings.StatusWallClimbTag.IsValid() && bAreAbilitiesInitialized)
      {
         abilitySystem->SetLooseGameplayTagCount(settings.StatusWallClimbTag, 1);
      }
   }
}

void AOSECharacterBase::OnStopWallClimbing_Implementation()
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent();
   if (settings.StatusWallClimbTag.IsValid() && IsValid(abilitySystem) && bAreAbilitiesInitialized)
   {
      abilitySystem->SetLooseGameplayTagCount(settings.StatusWallClimbTag, 0);
   }
}

void AOSECharacterBase::OnStartWallDash_Implementation(bool bAutoWallDash)
{
   UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent();
   if (IsValid(abilitySystem) && bAreAbilitiesInitialized)
   {
      abilitySystem->AddLooseGameplayTag(TAG_Status_WallClimbing_Dashing);
   }
}

void AOSECharacterBase::OnStopWallDash_Implementation()
{
   UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent();
   if (IsValid(abilitySystem) && bAreAbilitiesInitialized)
   {
      abilitySystem->RemoveLooseGameplayTag(TAG_Status_WallClimbing_Dashing);
   }
}

//---------------------------------------------------------------------------------------
// ITraversalInterface (ground speed)
//---------------------------------------------------------------------------------------

EOSEGroundSpeed AOSECharacterBase::GetGroundSpeedThreshold(float speedValue) const
{
   UOSECharacterMovement* movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement());
   if (!movementComp || (!movementComp->IsMovingOnGround() && !movementComp->IsFalling()))
      return EOSEGroundSpeed::Invalid;

   // invalid input
   if (speedValue < 0.0f)
      return EOSEGroundSpeed::Invalid;

   // stopped if we're just about at 0
   if (FMath::Abs(speedValue) < KINDA_SMALL_NUMBER)
   {
      return EOSEGroundSpeed::Stopped;
   }

   // need to provide a little buffer here, things like walk speed can oscillate between 399.99 and
   // 400.0f which would flicker between states, so leave a little leeway for it to not be perfectly >=
   const float kSpeedBuffer = 0.1f;

   const float maxWalkSpeed = movementComp->MaxWalkSpeed - kSpeedBuffer;
   const float sprintSpeed = maxWalkSpeed + movementComp->GetSprintSpeedIncrementBase() - kSpeedBuffer;
   const float sprintSpeedFwd = sprintSpeed + movementComp->GetSprintSpeedIncrementForward() - kSpeedBuffer;
   if (speedValue < maxWalkSpeed)
   {
      return EOSEGroundSpeed::Slow;
   }
   else if (speedValue >= maxWalkSpeed && speedValue < sprintSpeed)
   {
      return EOSEGroundSpeed::Walk;
   }
   else if (speedValue >= sprintSpeed && speedValue < sprintSpeedFwd)
   {
      return EOSEGroundSpeed::Sprint;
   }
   else if (speedValue >= sprintSpeedFwd)
   {
      return EOSEGroundSpeed::Full;
   }

   // should have covered everything above...?
   unimplemented();
   return EOSEGroundSpeed::Invalid;
}

EOSEGroundSpeed AOSECharacterBase::GetGroundSpeed() const
{
   const float currentSpeed = GetVelocity().Size();
   return GetGroundSpeedThreshold(currentSpeed);
}

bool AOSECharacterBase::HasMatchingGameplayTag(FGameplayTag tagToCheck) const
{
    if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
    {
        return asc->HasMatchingGameplayTag(tagToCheck);
    }
    return false;
}

bool AOSECharacterBase::HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const
{
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      return asc->HasAllMatchingGameplayTags(tagContainer);
   }
   return false;
}

bool AOSECharacterBase::HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const
{
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      return asc->HasAnyMatchingGameplayTags(tagContainer);
   }
   return false;
}

void AOSECharacterBase::GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const
{
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      return asc->GetOwnedGameplayTags(tagContainer);
   }
}


//-------------------------------------------------------------------------------------------------
// Replication
//-------------------------------------------------------------------------------------------------

void AOSECharacterBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   
   // Not Push Based regardless of Iris
   // Replicate some state to simulated proxies only
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _isScrambling, COND_SimulatedOnly);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _isWallClimbing, COND_SimulatedOnly);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, bIsSprinting, COND_SimulatedOnly);
	
   // Replicate mantling state to simulated proxies only
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _mantleState, COND_SimulatedOnly);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _ledgeState, COND_SimulatedOnly);
   
   // Push Based if using Iris
   #if UE_WITH_IRIS
   // Everyone except local owner; flag change is locally instigated
   FDoRepLifetimeParams SkipParams;
   SkipParams.bIsPushBased = true;
   SkipParams.Condition = COND_SkipOwner;
   
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, RemoteBaseAimRotation, SkipParams);
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, RemoteViewRotation, SkipParams);
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, _ragdollParams, SkipParams);

   // Assumed to be locally instigated; replicated to everyone else.
   // The movement component can be set directly if client prediction is correctly handled.
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, _distanceConstraint, SkipParams);
   #else
   DOREPLIFETIME_CONDITION(AOSECharacterBase, RemoteBaseAimRotation, COND_SkipOwner);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, RemoteViewRotation, COND_SkipOwner);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _ragdollParams, COND_SkipOwner);

   // Assumed to be locally instigated; replicated to everyone else.
   // The movement component can be set directly if client prediction is correctly handled.
   DOREPLIFETIME_CONDITION(AOSECharacterBase, _distanceConstraint, COND_SkipOwner);
   #endif

   
   #if UE_WITH_IRIS
   FDoRepLifetimeParams SimParams;
   SimParams.bIsPushBased = true;
   SimParams.Condition = COND_SimulatedOnly;
   
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, bExtendingDistanceConstraint, SimParams);
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, bContractingDistanceConstraint, SimParams);
   #else
   DOREPLIFETIME_CONDITION(AOSECharacterBase, bExtendingDistanceConstraint, COND_SimulatedOnly);
   DOREPLIFETIME_CONDITION(AOSECharacterBase, bContractingDistanceConstraint, COND_SimulatedOnly);   
   #endif

   
   #if UE_WITH_IRIS
   FDoRepLifetimeParams DefaultParams;
   DefaultParams.bIsPushBased = true;
   #endif
   if (!bCreatedStaticAbilityComponents)
   {
      #if UE_WITH_IRIS
      DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, AbilitySystemComponent, DefaultParams);
      DOREPLIFETIME_WITH_PARAMS_FAST(AOSECharacterBase, BaseAttributeSet, DefaultParams);
	  #else
	  DOREPLIFETIME(AOSECharacterBase, AbilitySystemComponent);
      DOREPLIFETIME(AOSECharacterBase, BaseAttributeSet);
	  #endif
   }
   else
   {
      DISABLE_REPLICATED_PROPERTY(AOSECharacterBase, AbilitySystemComponent);
      DISABLE_REPLICATED_PROPERTY(AOSECharacterBase, BaseAttributeSet);
   }

}

void AOSECharacterBase::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
   Super::PreReplication(ChangedPropertyTracker);

   // The server is responsible for replicating the base aim rotation to everyone else.
   // The locally controlled autonomous proxy won't need it, but any simulated proxies
   // will need it since they won't have a controller.
   if (GetLocalRole() == ROLE_Authority && GetController() != nullptr)
   {
      RemoteBaseAimRotation = GetBaseAimRotation();
      RemoteViewRotation = GetViewRotation();
	  #if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, RemoteBaseAimRotation, this);
      MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, RemoteViewRotation, this);
	  #endif
   }
}

void AOSECharacterBase::_OnHealthChanged(const FOnAttributeChangeData& data)
{
   OnHealthChanged(data.NewValue, data.OldValue);
}

void AOSECharacterBase::_OnDamageChanged(const FOnAttributeChangeData& data)
{
   // remove these gameplay effects whenever we take damage
   if (data.NewValue > 0.f && data.OldValue <= 0.f)
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
      if (settings.EffectTagsToRemoveOnTakingDamage.IsValid())
         AbilitySystemComponent->RemoveActiveEffectsWithTags(settings.EffectTagsToRemoveOnTakingDamage);

      if (VoiceOverTriggers && data.GEModData)
      {
         VoiceOverTriggers->FireDamageVO(this, data.GEModData->EffectSpec);
      }
   }

   if (HasAuthority())
   {
      _AuthorityCheckForDamageKnockout(data);
   }
}

void AOSECharacterBase::_OnConditionTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

   if (tag != settings.ConditionBaseTag && newTagCount == 1)
   {
      OnConditionGained(tag);
   }
   else if (tag == settings.ConditionBaseTag && newTagCount == 0)
   {
      OnConditionStabilized();
   }
}

void AOSECharacterBase::OnUnconsciousChanged_Implementation(bool isUnconscious)
{

}

void AOSECharacterBase::_OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   const bool newIsUnconscious = newTagCount >= 1;
   if (newIsUnconscious != _isUnconscious)
   {
      _isUnconscious = newIsUnconscious;
      OnUnconsciousChanged(_isUnconscious);
   }
}

void AOSECharacterBase::_OnLyingDownTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   bool isLyingDown = (newTagCount > 0) || AbilitySystemComponent->HasAnyMatchingGameplayTags(settings.LyingDownTags);
   if (isLyingDown != _isLyingDown)
   {
      _isLyingDown = isLyingDown;
      OnLyingDownChanged(_isLyingDown);
      OnLyingDown.Broadcast(_isLyingDown);

      if (HasAuthority())
      {
         if (UOSECharacterMovement* movementComponent = Cast<UOSECharacterMovement>(GetCharacterMovement()))
         {
            movementComponent->AuthorityOnLyingDownChanged(_isLyingDown);
         }
      }
   }
}

void AOSECharacterBase::OnLyingDownChanged_Implementation(bool isLyingDown)
{
}

void AOSECharacterBase::_OnMovementImpairingTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   bool isMovementInhibited = (newTagCount > 0) || AbilitySystemComponent->HasAnyMatchingGameplayTags(settings.MovementImpairingTags);
   if (isMovementInhibited != _isMovementInhibited)
   {
      _isMovementInhibited = isMovementInhibited;
      OnMovementIsInhibitedChanged(_isMovementInhibited);
   }
}

void AOSECharacterBase::OnMovementIsInhibitedChanged_Implementation(bool isMovementInhibited)
{
   if (AController* controller = GetController())
   {
      if (isMovementInhibited)
      {
         controller->SetIgnoreMoveInput(true);
      }
      else
      {
         controller->SetIgnoreMoveInput(false);
      }
   }
}

void AOSECharacterBase::_OnCarryingTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   bool newIsCarrying = newTagCount > 0;
   if (newIsCarrying != _isCarrying)
   {
      _isCarrying = newIsCarrying;
      OnCarryingChanged(_isCarrying);
   }
}

void AOSECharacterBase::OnCarryingChanged_Implementation(bool isCarrying)
{
   // stow/unstow when carrying changes -- no weapons out while carrying bodies.
   UOSEItemFunctionLibrary::StowCurrentToolForAvatar(this, isCarrying);
}

void AOSECharacterBase::_RegisterConditionTagEvent(const FGameplayTag& conditionTag)
{
   check(AbilitySystemComponent);
   
   FDelegateHandle delegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(conditionTag).AddUObject(this, &AOSECharacterBase::_OnConditionTagChanged);
   _conditionChangeDelegateHandles.Add(FTagBindingInfo{ conditionTag, delegateHandle });
}

void AOSECharacterBase::_RegisterLyingDownTagEvent(const FGameplayTag& lyingDownTag)
{
   check(AbilitySystemComponent);

   FDelegateHandle delegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(lyingDownTag).AddUObject(this, &AOSECharacterBase::_OnLyingDownTagChanged);
   _lyingDownDelegateHandles.Add(FTagBindingInfo{ lyingDownTag, delegateHandle });
}

void AOSECharacterBase::_RegisterMovementImpairingTagEvent(const FGameplayTag& movementImpairingTag)
{
   check(AbilitySystemComponent);

   FDelegateHandle delegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(movementImpairingTag).AddUObject(this, &AOSECharacterBase::_OnMovementImpairingTagChanged);
   _movementImpairingDelegateHandles.Add(FTagBindingInfo{ movementImpairingTag, delegateHandle });
}

void AOSECharacterBase::OnRep_AbilityComponents()
{
   // These two objects are not guaranteed to replicate in order, wait for both
   if (AbilitySystemComponent && BaseAttributeSet)
   {
      InitializeAbilities(AbilitySystemComponent, BaseAttributeSet);
   }
}

void AOSECharacterBase::InitializeAbilities(UOSEAbilitySystemComponent* InComponent, UAttributeBaseSet* InAttributeSet)
{
   if (bAreAbilitiesInitialized)
   {
      return;
   }

   if (!AbilitySystemComponent)
   {
      AbilitySystemComponent = InComponent;
   }
   
   if (!BaseAttributeSet)
   {
      BaseAttributeSet = InAttributeSet;
   }

   AbilitySystemComponent->SetAvatarActor(this);

   if (InputComponent)
   {
      // If we already have an input component, this got skipped
      AbilitySystemComponent->BindToInputComponent(InputComponent);
   }

   // @TODO: Do we actually want to grant abilities on the client? They normally get replicated down

   // Grant initial abilities
   for (const UOSEGameplayAbilitySet* AbilitySet : InitialAbilitySets)
   {
      if (AbilitySet != nullptr)
      {
         GrantedInitialAbilities.Append(AbilitySet->GiveAbilities(this));
      }
   }

   // Apply initial effects
   for (const UOSEGameplayEffectSet* EffectSet : InitialEffectSets)
   {
      if (EffectSet != nullptr)
      {
         GrantedInitialEffects.Append(EffectSet->ApplyEffects(this));
      }
   }

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthAttribute()).AddUObject(this, &AOSECharacterBase::_OnHealthChanged);
      asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthDamageAttribute()).AddUObject(this, &AOSECharacterBase::_OnDamageChanged);
      _onCarryingChangeDelegateHandle = asc->RegisterGameplayTagEvent(settings.StatusCarryingTag).AddUObject(this, &AOSECharacterBase::_OnCarryingTagChanged);

      // generically respond to conditions
      for (const FGameplayTag& tag : settings.ConditionTags)
      {
         _RegisterConditionTagEvent(tag);
      }

      // specifically respond to conditions
      _unconsciousDelegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).AddUObject(this, &AOSECharacterBase::_OnIsUnconsciousTagChanged);

      // movement impairing tags
      for (const FGameplayTag& tag : settings.MovementImpairingTags)
      {
         _RegisterMovementImpairingTagEvent(tag);
      }

      // lying down tags
      for (const FGameplayTag& tag : settings.LyingDownTags)
      {
         _RegisterLyingDownTagEvent(tag);
      }

      // N.B. There's a possibility that the ASC replicates after we replicate the crouching status
      // In that case, we need to explicitly check here if we're crouching so we can apply the tag
      if (IsCrouching())
      {
         if (settings.StatusCrouchingTag.IsValid())
         {
            asc->AddLooseGameplayTag(settings.StatusCrouchingTag);
         }
      }

      // Same as above for sprinting
      if (IsSprinting())
      {
         if (settings.StatusSprintingTag.IsValid())
         {
            asc->AddLooseGameplayTag(settings.StatusSprintingTag);
         }
      }

      // Same as above for sliding
      if (IsSliding())
      {
         if (settings.StatusSlidingTag.IsValid())
         {
            asc->AddLooseGameplayTag(settings.StatusSlidingTag);
         }
      }

      if (bInfiniteStamina)
      {
         asc->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("Cheat.InfiniteStamina"));
      }

      // make sure initial movement mode is set, if any
      const UCharacterMovementComponent* cmc = GetCharacterMovement();
      CharacterHelpers::AddMovementModeTag(settings, asc, cmc->MovementMode, cmc->CustomMovementMode);
   }

   bAreAbilitiesInitialized = true;

   OnAbilitiesInitialized();

   OnAbilitiesInitializedDelegate.Broadcast();

   ForceNetUpdate();
}

void AOSECharacterBase::ResetAbilities()
{
   if (bAreAbilitiesInitialized)
   {
      UAbilitySystemComponent* asc = GetAbilitySystemComponent();
      if (asc)
      {
         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

         // remove movement mode tag, if any
         if (const UCharacterMovementComponent* cmc = GetCharacterMovement())
         {
            CharacterHelpers::RemoveMovementModeTag(settings, asc, cmc->MovementMode, cmc->CustomMovementMode);
         }

         asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthAttribute()).RemoveAll(this);
         asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthDamageAttribute()).RemoveAll(this);
         asc->UnregisterGameplayTagEvent(_onCarryingChangeDelegateHandle, settings.StatusCarryingTag);

         for (FTagBindingInfo& conditionChangeInfo : _conditionChangeDelegateHandles)
         {
            asc->UnregisterGameplayTagEvent(conditionChangeInfo.DelegateHandle, conditionChangeInfo.Tag);
         }
         _conditionChangeDelegateHandles.Reset();

         asc->UnregisterGameplayTagEvent(_unconsciousDelegateHandle, settings.ConditionUnconsciousTag);

         for (FTagBindingInfo& movementImpairingInfo : _movementImpairingDelegateHandles)
         {
            asc->UnregisterGameplayTagEvent(movementImpairingInfo.DelegateHandle, movementImpairingInfo.Tag);
         }
         _movementImpairingDelegateHandles.Reset();

         for (FTagBindingInfo& binding : _lyingDownDelegateHandles)
         {
            asc->UnregisterGameplayTagEvent(binding.DelegateHandle, binding.Tag);
         }
         _lyingDownDelegateHandles.Reset();

         // NOTE: This may not work properly for Instant effects
         for (FActiveGameplayEffectHandle EffectHandle : GrantedInitialEffects)
         {
            asc->RemoveActiveGameplayEffect(EffectHandle);
         }

         for (FGameplayAbilitySpecHandle AbilityHandle : GrantedInitialAbilities)
         {
            asc->ClearAbility(AbilityHandle);
         }

         if (settings.StatusCrouchingTag.IsValid())
         {
            asc->RemoveLooseGameplayTag(settings.StatusCrouchingTag);
         }

         if (settings.StatusSprintingTag.IsValid())
         {
            asc->RemoveLooseGameplayTag(settings.StatusSprintingTag);
         }

         if (settings.StatusSlidingTag.IsValid())
         {
            asc->RemoveLooseGameplayTag(settings.StatusSprintingTag);
         }

         GrantedInitialAbilities.Reset();
         GrantedInitialEffects.Reset();

         OnAbilitiesReset();
         OnAbilitiesResetDelegate.Broadcast();

         bAreAbilitiesInitialized = false;
      }

      // Clear both delegates, will need to be registered again
      OnAbilitiesResetDelegate.Clear();
      OnAbilitiesInitializedDelegate.Clear();
   }
}

void AOSECharacterBase::CallOrRegisterAbilitiesInitializedDelegate(const FSimpleMulticastDelegate::FDelegate& initializeDelegate)
{
   if (bAreAbilitiesInitialized)
   {
      initializeDelegate.Execute();
   }
   else 
   {
      OnAbilitiesInitializedDelegate.Add(initializeDelegate);
   }
}

void AOSECharacterBase::RegisterAbilitiesResetDelegate(const FSimpleMulticastDelegate::FDelegate& resetDelegate)
{
   OnAbilitiesResetDelegate.Add(resetDelegate);
}

//---------------------------------------------------------------------------------------
// Fall Damage
//---------------------------------------------------------------------------------------

void AOSECharacterBase::Landed(const FHitResult& hit)
{
   Super::Landed(hit);

   // Allow circumventing normal fall damage
   if (_InterceptLanded(hit))
   {
      return;
   }

   if (_fallingDamageCurve)
   {
      // This simplifies a lot assuming Z direction gravity, but it's a useful abstraction
      // to maintain as a reference for other applications (impact damage, changing gravity, etc)
      
      // GetGravityZ is scaled by gravity scale, so under lower gravity we'd take damage more easily and under higher
      // gravity scale we'd take damage less easily. This is undesirable, so we use DefaultGravityZ here so that damage
      // calculations are always computed against the world gravity.
      const FVector gravityVector(0.0f, 0.0f, UPhysicsSettings::Get()->DefaultGravityZ);

      FVector gravityDirection;
      float gravityLength;
      gravityVector.ToDirectionAndLength(gravityDirection, gravityLength);

      // Projecting the velocity to gravity direction (for -Z case, it's just the Z component negated).
      // Also, don't allow the projected velocity to go negative... if we were somehow "landing" with
      // a velocity in the opposite direction of gravity ¯\_(ツ)_/¯
      const FVector velocityVector = GetVelocity();
      const FVector velocityProjected = gravityDirection * FMath::Max(0.0f, velocityVector | gravityDirection);

      // Project the velocity to the impact normal. This is the speed of impact relative to the surface.
      // Value is expected to be negative, we negate the impact normal and clamp to get a positive (or zero) value
      const float impactSpeed = FMath::Max(0.0f, velocityProjected | (-hit.Normal));

      // Assumes instaneous velocity when falling is the square root of 2*d*g, where
      // d is the distance travelled. Squaring both sides gives us v^2 = 2*d*g, which is
      // a useful value to COMPARE against...
      const float minFallSpeedSqr = 2.0f * _minFallHeight * gravityLength;
      if ((impactSpeed * impactSpeed) > minFallSpeedSqr)
      {
         // ...however, when we use it in calculations or to look up values our result will not be accurate.
         // Thus, we need to use the actual LINEAR values in any calculations for consistent results.
         const float safeFallSpeed = FMath::Sqrt(2.0f * _safeFallHeight * gravityLength);
         const float deathFallSpeed = FMath::Sqrt(2.0f * _deathFallHeight * gravityLength);

         // take falling damage
         {
            float pctOfMaxFallingDamage = FMath::GetRangePct(safeFallSpeed, deathFallSpeed, impactSpeed);
            if (pctOfMaxFallingDamage > 0.0f)
            {
               // how much damage should we take?
               float pctOfMaxHealthToLose = _fallingDamageCurve->GetFloatValue(pctOfMaxFallingDamage);
               float damageTaken = GetHealthMax() * pctOfMaxHealthToLose;
               if (damageTaken > 0.0f)
               {
                  // apply dmg taken
                  const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
                  settings.FallingDamageEffect.ApplyEffectWithMagnitude(AbilitySystemComponent, damageTaken);

                  // let our bp subclasses do anything else they'd like to do once we take dmg, self-stuns to drive animation state or whatever!
                  _OnFallingDamageTaken(hit, pctOfMaxHealthToLose, damageTaken);
               }
            }
         }

         // send along an event that we fell for things like audio sfx; uses minFallVelocity to weed out tiny falls that shouldn't count
         {
            const float pctOfMaxfall = FMath::GetRangePct(FMath::Sqrt(minFallSpeedSqr), deathFallSpeed, impactSpeed);
            _OnLandedWithData(hit, pctOfMaxfall);
         }
      }
   }
}

//---------------------------------------------------------------------------------------
// Falling
//---------------------------------------------------------------------------------------

void AOSECharacterBase::OnStartFalling_Implementation()
{
   if (UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
      if (settings.StatusFallingTag.IsValid() && bAreAbilitiesInitialized)
      {
         abilitySystem->SetLooseGameplayTagCount(settings.StatusFallingTag, 1);
      }
   }
}

void AOSECharacterBase::OnStopFalling_Implementation()
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   UAbilitySystemComponent* abilitySystem = GetAbilitySystemComponent();
   if (settings.StatusFallingTag.IsValid() && IsValid(abilitySystem) && bAreAbilitiesInitialized)
   {
      abilitySystem->SetLooseGameplayTagCount(settings.StatusFallingTag, 0);
   }
}

//---------------------------------------------------------------------------------------
// Distance constraints
//---------------------------------------------------------------------------------------

void AOSECharacterBase::SetDistanceConstraint(FOSEDistanceConstraint distanceConstraint)
{
   _distanceConstraint = distanceConstraint;
   
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, _distanceConstraint, this);
   #endif
   
   // Value was set on locally controlled pawn.
   // Make sure the value is updated on the movement component.
   _UpdateDistanceConstraint();
}

void AOSECharacterBase::_OnRep_DistanceConstraint()
{
   // Value was replicated from server to simulated proxy.
   // Make sure the value is updated on the movement component.
   _UpdateDistanceConstraint();
}

void AOSECharacterBase::_UpdateDistanceConstraint()
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetDistanceConstraint(_distanceConstraint);
   }
}

void AOSECharacterBase::_SetContractingDistanceConstraint(bool newValue)
{
   bContractingDistanceConstraint = newValue;
   
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, bContractingDistanceConstraint, this);
   #endif
   
   if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->SetContractingDistanceConstraint(newValue);
   }
}

void AOSECharacterBase::_SetExtendingDistanceConstraint(bool newValue)
{
   bExtendingDistanceConstraint = newValue;
   
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSECharacterBase, bExtendingDistanceConstraint, this);
   #endif
   
   if (auto movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->SetExtendingDistanceConstraint(newValue);
   }
}

void AOSECharacterBase::ExtendDistanceConstraintRequest()
{
   check(_extendDistanceConstraintCounter >= 0);
   ++_extendDistanceConstraintCounter;
   UE_LOG(LogOSECharacter, Verbose, TEXT("ExtendDistanceConstraintRequest: count = %d"), _extendDistanceConstraintCounter);

   if (_extendDistanceConstraintCounter == 1)
   {
      _SetExtendingDistanceConstraint(true);
   }
}

void AOSECharacterBase::ExtendDistanceConstraintCancel()
{
   _extendDistanceConstraintCounter = FMath::Max(0, _extendDistanceConstraintCounter - 1);
   UE_LOG(LogOSECharacter, Verbose, TEXT("ExtendDistanceConstraintCancel: count = %d"), _extendDistanceConstraintCounter);

   if (_extendDistanceConstraintCounter <= 0)
   {
      _SetExtendingDistanceConstraint(false);
   }
}

void AOSECharacterBase::ContractDistanceConstraintRequest()
{
   check(_contractDistanceConstraintCounter >= 0);
   ++_contractDistanceConstraintCounter;
   UE_LOG(LogOSECharacter, Verbose, TEXT("ContractDistanceConstraintRequest: count = %d"), _contractDistanceConstraintCounter);

   if (_contractDistanceConstraintCounter == 1)
   {
      _SetContractingDistanceConstraint(true);
   }
}

void AOSECharacterBase::ContractDistanceConstraintCancel()
{
   _contractDistanceConstraintCounter = FMath::Max(0, _contractDistanceConstraintCounter - 1);
   UE_LOG(LogOSECharacter, Verbose, TEXT("ContractDistanceConstraintCancel: count = %d"), _contractDistanceConstraintCounter);

   if (_contractDistanceConstraintCounter <= 0)
   {
      _SetContractingDistanceConstraint(false);
   }
}

void AOSECharacterBase::OnRep_ExtendingDistanceConstraint()
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetExtendingDistanceConstraint(bExtendingDistanceConstraint);
      MovementComp->bNetworkUpdateReceived = true;
   }
}

void AOSECharacterBase::OnRep_ContractingDistanceConstraint()
{
   if (auto MovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      MovementComp->SetContractingDistanceConstraint(bContractingDistanceConstraint);
      MovementComp->bNetworkUpdateReceived = true;
   }
}

void AOSECharacterBase::OnBumpedInto_Implementation(AOSECharacterBase* otherCharacter, const FHitResult& impact)
{
   check(otherCharacter);
   OnBumpedIntoEvent.Broadcast(otherCharacter, impact);
}

void AOSECharacterBase::OnBumpedBy_Implementation(AOSECharacterBase* otherCharacter, const FHitResult& impact)
{
   check(otherCharacter);
   OnBumpedByEvent.Broadcast(otherCharacter, impact);
}

UOSEFootstepSimulatorComponent* AOSECharacterBase::GetFootstepComponent() const 
{ 
   return _footstepComponent; 
}

void AOSECharacterBase::_AuthorityCheckForDamageKnockout(const FOnAttributeChangeData& data)
{
   check(HasAuthority());

   check(data.Attribute == UAttributeBaseSet::GetHealthDamageAttribute());

   // Only check for knockout if HealthDamage is increasing
   if (data.NewValue > data.OldValue)
   {
      const float currentHealthDamage = data.NewValue;
      const float currentHealth = GetHealth();

      AActor* instigator = _GetInstigatorForAttributeChange(data);

      // If our health damage exceeds our current health, then we've been knocked out
      if (currentHealthDamage >= currentHealth)
      {
         // We want to consistently call either AuthorityOnKnockedOutByOtherCharacter or AuthorityOnKnockedOutByNonCharacterSource
         // so that downstream code can be sure that if our health reaches 0, one of them will be called
         bool knockedOutByOtherCharacter = false;
         if (instigator != nullptr)
         {
            AActor* ultimateInstigator = UOSECommon::FindUltimateInstigator(instigator);
            if (AOSECharacterBase* ultimateInstigatingCharacter = UOSECommon::GetPawn<AOSECharacterBase>(ultimateInstigator))
            {
               UE_LOG(LogOSECharacter, Log, TEXT("Character '%s' knocked out by '%s'"),
                  *GetName(),
                  *ultimateInstigatingCharacter->GetName());

               // Don't consider self-knockouts, e.g. from fall damage
               if (ultimateInstigatingCharacter != this)
               {
                  knockedOutByOtherCharacter = true;
                  AuthorityOnKnockedOutByOtherCharacter(ultimateInstigatingCharacter);
               }
            }
            else
            {
               UE_LOG(LogOSECharacter, Log, TEXT("Could not find player instigator for health damage (direct instigator is '%s')"), *instigator->GetName());
            }
         }
         else
         {
            // null-check here, but if it's null we'd hit the ensure in _GetInstigatorForAttributeChange
            if (data.GEModData != nullptr)
            {
               UE_LOG(LogOSECharacter, Warning, TEXT("No instigator actor for health damage: effect = '%s' ability = '%s'"),
                  *GetNameSafe(data.GEModData->EffectSpec.Def),
                  *GetNameSafe(data.GEModData->EffectSpec.GetEffectContext().GetAbility()));
            }
            else
            {
               UE_LOG(LogOSECharacter, Error, TEXT("No instigator actor for health damage on '%s' due to no effect context"), *GetName());
            }
         }

         // If we didn't find a character source for the KO, call this instead
         if (!knockedOutByOtherCharacter)
         {
            AuthorityOnKnockedOutByNonCharacterSource();
         }
      }
   }
}

AActor* AOSECharacterBase::_GetInstigatorForAttributeChange(const FOnAttributeChangeData& data)
{
   ensureMsgf(data.GEModData != nullptr,
      TEXT("Got attribute change to '%s' on '%s' callback w/o a gameplay effect context. ")
      TEXT("Attribute was either increased directly, or we batched gameplay effects in an unsupported way (see UE-109007)"),
      *data.Attribute.GetName(),
      *GetName());

   if (data.GEModData != nullptr)
   {
      FGameplayEffectContextHandle damageEffectContext = data.GEModData->EffectSpec.GetEffectContext();
      return damageEffectContext.GetInstigator();
   }
   
   return nullptr;
}
