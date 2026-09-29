// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATSwingingDoor.h"

// tat
#include "AI/TATAIController.h"
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/Traits/TATAITraits.h"
#include "Audio/TATAudioPortalComponent.h"
#include "Breakables/TATBreakableComponent.h"
#include "Developer/TATProjectSettings.h"
#include "Interactables/SwingingDoorNavLinkComponent.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Interactables/TATSecurityLockdownComponent.h"
#include "Interactables/TATSwingingDoorPivotComponent.h"
#include "Variation/TATSpawnerComponent.h"
#include "AI/Target/TATTargetingGroups.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue4
#include "AkAudioEvent.h"
#include "AkGameplayStatics.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "VisualLogger/VisualLogger.h"
#include "Components/BillboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSwingingDoor)

DEFINE_LOG_CATEGORY_STATIC(LogTATSwingingDoor, Log, All);

#define LOCTEXT_NAMESPACE "ATATSwingingDoor"

UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_BreakDownDoor_Events_StartBreakingDownDoor, "Ability.BreakDownDoor.Events.StartBreakingDownDoor");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_BreakDownDoor_Events_DoBreakDownDoor, "Ability.BreakDownDoor.Events.DoBreakDownDoor");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Object_Broken_Door, "AI.Object.Broken.Door")

namespace DoorHelpers
{
   static ACharacter* GetCharacterFromPathingAgent(UObject* pathingAgent)
   {
      if (auto pathComp = Cast<UPathFollowingComponent>(pathingAgent))
      {
         AActor* pathOwner = pathComp->GetOwner();
         AController* controllerOwner = Cast<AController>(pathOwner);
         if (controllerOwner)
         {
            return controllerOwner->GetPawn<ACharacter>();
         }
      }
      return nullptr;
   }

   static bool MatchDirection(EDoorLockDirection direction, bool isInFront)
   {
      const EDoorLockDirection mask = isInFront ? EDoorLockDirection::LockFrontOnly : EDoorLockDirection::LockBackOnly;
      return (static_cast<uint8>(direction) & static_cast<uint8>(mask)) != 0;
   }

   static bool MatchDirection(EDoorOpenDirection direction, bool isInFront)
   {
      const EDoorOpenDirection mask = isInFront ? EDoorOpenDirection::OpenFrontOnly : EDoorOpenDirection::OpenBackOnly;
      return (static_cast<uint8>(direction) & static_cast<uint8>(mask)) != 0;
   }
 
   static TConstArrayView<ESwingingDoorPosition> GetAllowedSwingOpenPositions(EDoorOpenDirection allowedOpenFromDirection)
   {
      // NOTE: since EDoorOpenDirection (as used in FTATSwingingDoorAllowedDirections::AllowedOpenDirection) indicates a relative location a door 
      // can be opened FROM (rather than opened IN), we return an ESwingingDoorPosition opposing the provided EDoorOpenDirection (if not Both)
      static constexpr ESwingingDoorPosition openBothDirections[] { ESwingingDoorPosition::OpenFront, ESwingingDoorPosition::OpenBack };
      static constexpr ESwingingDoorPosition openFromFrontOnly[] { ESwingingDoorPosition::OpenBack };
      static constexpr ESwingingDoorPosition openFromBackOnly[] { ESwingingDoorPosition::OpenFront };

      switch (allowedOpenFromDirection)
      {
      case EDoorOpenDirection::Both:
         return openBothDirections;
      case EDoorOpenDirection::OpenFrontOnly:
         return openFromFrontOnly;
      case EDoorOpenDirection::OpenBackOnly:
         return openFromBackOnly;

      // Should not be called with None!
      default:
         checkNoEntry();
         return TConstArrayView<ESwingingDoorPosition>{};
      }
   }
}

namespace DoorCVars
{
   static int32 InterpolateAudioPortals = 0;
   static FAutoConsoleVariableRef CVarInterpolateAudioPortals(
      TEXT("TAT.Door.InterpolateAudioPortals"),
      InterpolateAudioPortals,
      TEXT("Whether to interpolate the obstruction/occlusion of door audio portals when opening/closing (default 0)"),
      ECVF_Default);
}

// Sets default values
ATATSwingingDoor::ATATSwingingDoor()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true;

   // start dormant
   // TODO: We want this to be DORM_Initial, but are running into a replication issue where if a property is changed in the level instance,
   // and then changed back to its CDO default sometime later, FlushNetDormancy does not properly replicate the change to clients
   NetDormancy = DORM_DormantAll;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
   RootComponent->bEditableWhenInherited = true;
   RootComponent->Mobility = EComponentMobility::Static;

   _pivot = CreateDefaultSubobject<USceneComponent, UTATSwingingDoorPivotComponent>(TEXT("Pivot"));
   _pivot->SetupAttachment(RootComponent);
   _pivot->ComponentTags.Add(TEXT("Interact"));
   _pivot->ComponentTags.Add(UTATProjectSettings::Get().NavModifiableDynamicComponentTag);
   _pivot->ComponentTags.Add(UTATProjectSettings::Get().NavModifiableStaticComponentTag);

   _navLinkFront = CreateDefaultSubobject<USwingingDoorNavLinkComponent>(TEXT("NavLinkFront"));
   _navLinkFront->SetLinkDirection(ENavLinkDirection::RightToLeft);

   _navLinkBack = CreateDefaultSubobject<USwingingDoorNavLinkComponent>(TEXT("NavLinkBack"));
   _navLinkBack->SetLinkDirection(ENavLinkDirection::LeftToRight);

   // mission spawning disabled by default, let the default locked bool do it's job.  only drive the locked
   // state for the mission if we set this up to be driven by it.
   _lockedSpawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("VariationSpawner"));
   _lockedSpawnerComponent->SetSpawnType(ETATSpawnChanceType::Disabled);

   // Spawner disabled (i.e. spawns closed) by default, can be configured on an instance/subclass basis to potentially spawn open
   _openSpawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("OpenSpawner"));
   _openSpawnerComponent->SetSpawnType(ETATSpawnChanceType::Disabled);

   _securityLockdownComponent = CreateDefaultSubobject<UTATSecurityLockdownComponent>(TEXT("SecurityLockdown"));

   _navLinkOwnerComponent = CreateDefaultSubobject<UTATNavLinkOwnerComponent>(TEXT("NavLinkOwnerComponent"));
   
   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));

   _portalBoxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalBoxTrigger"));
   _portalBoxTrigger->SetupAttachment(RootComponent);
   _portalBoxTrigger->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   _portalBoxTrigger->Mobility = EComponentMobility::Static;

   _portalComponent = CreateDefaultSubobject<UTATAudioPortalComponent>(TEXT("PortalComponent"));
   _portalComponent->SetupAttachment(_portalBoxTrigger);
   _portalComponent->InitialState = AkAcousticPortalState::Closed;
   _portalComponent->Mobility = EComponentMobility::Static;

   _visualBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("VisualBounds"));
   _visualBounds->SetupAttachment(RootComponent);
   _visualBounds->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   _visualBounds->ShapeColor = FColor::Purple;
   _visualBounds->SetLineThickness(1.5f);
   _visualBounds->SetBoxExtent(FVector(10.0f, 30.0f, 30.0f));
   _visualBounds->Mobility = EComponentMobility::Static;

   _incorrectStateActionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent_IncorrectObjectState>(TEXT("IncorrectStateActionNode"));
   _incorrectStateActionNodeComponent->SetupAttachment(RootComponent);
   _incorrectStateActionNodeComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
   if (GIsEditor && !IsRunningCommandlet())
   {
      // Actor viewport visualization of where ak events will be posted (_audioEventPostLocation)
      _akEventPostLocationEditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("DebugAkEventPostLocation"));
      _akEventPostLocationEditorBillboard->SetupAttachment(RootComponent);
      _akEventPostLocationEditorBillboard->Mobility = EComponentMobility::Static;
      _akEventPostLocationEditorBillboard->bHiddenInGame = true;
      _akEventPostLocationEditorBillboard->SetIsVisualizationComponent(true);

      _akEventPostLocationEditorBillboard->bIsScreenSizeScaled = true;
      _akEventPostLocationEditorBillboard->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
      _akEventPostLocationEditorBillboard->SetUsingAbsoluteScale(true);

      ConstructorHelpers::FObjectFinderOptional<UTexture2D> debugDisplayTexture(TEXT("/Engine/EditorResources/AudioIcons/S_AudioComponent"));
      if (UTexture2D* texture = debugDisplayTexture.Get())
      {
         _akEventPostLocationEditorBillboard->Sprite = texture;
      }
      else
      {
         UE_LOG(LogTATSwingingDoor, Error, TEXT("Failed to find texture with which to display _debugDrawAkEventPostLocation!"))
      }

      _RefreshAkPostLocationEditorPreview();
   }
#endif // WITH_EDITORONLY_DATA

   _swingAxis = EAxis::Z;
   _closedAngle = 0;
   _openBackAngle = 90;
   _openFrontAngle = -90;
   _secondaryAngleOffset = 180;
   _swingDuration = 0.4f;
   _brokenSwingSpeedMultiplier = 2;
   _frontDirection = FVector(0, 1, 0);

   _openPrompt = LOCTEXT("OpenPrompt", "Open");
   _closePrompt = LOCTEXT("ClosePrompt", "Close");
}

void ATATSwingingDoor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATSwingingDoor, _state);
   DOREPLIFETIME(ATATSwingingDoor, _locked);
   DOREPLIFETIME(ATATSwingingDoor, _lockedDown);
   DOREPLIFETIME(ATATSwingingDoor, _lockpickCurrentTrack);
   DOREPLIFETIME(ATATSwingingDoor, _lockDirection);
}

void ATATSwingingDoor::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);
   
   SnapTo(_state.Position);
}

void ATATSwingingDoor::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   _interactionGates.Initialize(this);
   _lockConfig.InitializeAlternateLock(this);

   // if this was set to anything other than disabled wait for a spawn callback to decide to lock/unlock
   if (_lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled)
   {
      _lockedSpawnerComponent->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATSwingingDoor::_AuthorityOnLockedSpawnerSpawn);
      _lockedSpawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATSwingingDoor::_AuthorityOnLockedSpawnerNotSpawn);
   }
   if (_openSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled)
   {
      _openSpawnerComponent->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerSpawn);
      _openSpawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerNotSpawn);
   }
   if(HasAuthority())
   {
      _navLinkOwnerComponent->RegisterNavLink(_navLinkBack);
      _navLinkOwnerComponent->RegisterNavLink(_navLinkFront);
   }

   if (HasAuthority() && _incorrectStateActionNodeComponent != nullptr)
   {
      // Incorrect state component will pay attention to whether this actor is broken or not.
      // If broken, the object will be marked as not in an incorrect state as it's a state 
      // the AI can't do anything about
      _incorrectStateActionNodeComponent->AssignBreakableComponent(_breakableComponent);

      // Helps prevent AI from considering an opened door in an "incorrect" state if the door
      // was opened by an AI with the intention of navigating through it.
      _incorrectStateActionNodeComponent->AssignNavLinkOwnerComponent(_navLinkOwnerComponent);

      // Incorrect state component will enable/disable the stimuli source component based on
      // whether the object is in a correct/incorrect state. If in incorrect state, it will be 
      // visible. If in a correct state, it will not be visible.
      _incorrectStateActionNodeComponent->AssignStimuliSourceComponent(_perceptionStimuliSource);

      // If this door's initial open/close state isn't going to change, set the initial state
      // now on the component.
      if (_openSpawnerComponent->GetSpawnType() == ETATSpawnChanceType::Disabled)
      {
         _UpdateIncorrectStateInitialState();
      }
   }

   _RefreshPivotComponentOffsets();
}

void ATATSwingingDoor::PostRegisterAllComponents()
{
   Super::PostRegisterAllComponents();
   
#if WITH_EDITOR
   if (_akEventPostLocationEditorBillboard)
   {
      // Only visible in BP editor viewport
      _akEventPostLocationEditorBillboard->SetVisibility(GetWorld()->IsPreviewWorld() && GetWorld()->IsEditorWorld());
      _RefreshAkPostLocationEditorPreview();
   }
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void ATATSwingingDoor::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propName = propertyChangedEvent.GetMemberPropertyName();
   if (propName == GET_MEMBER_NAME_CHECKED(ThisClass, _audioEventPostLocation))
   {
      _RefreshAkPostLocationEditorPreview();
   }
   else if (propName == GET_MEMBER_NAME_CHECKED(ThisClass, _state))
   {
      _RefreshPivotComponentOffsets();
   }
}

void ATATSwingingDoor::PostLoad()
{
   Super::PostLoad();

   // TODO: remove this after uses we care about are re-saved
   if(_relockDirection_DEPRECATED != EDoorLockDirection::Both && _normalAllowedDirections.RelockDirection == EDoorLockDirection::Both)
   {
      _normalAllowedDirections.RelockDirection = _relockDirection_DEPRECATED;
      _relockDirection_DEPRECATED = EDoorLockDirection::Both;
   }
}
#endif // WITH_EDITOR

FVector ATATSwingingDoor::GetNavAgentLocation() const
{
   // Get the bottom of the center of mass of the pivot component and its attachments
   return _pivotBounds.Origin - FVector::UpVector * _pivotBounds.BoxExtent.Z;
}

void ATATSwingingDoor::GetMoveGoalReachTest(const AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const
{
   GoalOffset = FVector::ZeroVector;
   GoalRadius = FMath::Max(_pivotBounds.BoxExtent.X, _pivotBounds.BoxExtent.Y);
   GoalHalfHeight = _pivotBounds.BoxExtent.Z;
}

void ATATSwingingDoor::GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const
{
   if(_breakableComponent && _breakableComponent->IsBroken() && !_IsDoorWarded())
   {
      tagContainer.AddTag(TAG_AI_Object_Broken_Door);
   }
}

FGameplayTag ATATSwingingDoor::GetUtilityAITargetingGroup() const
{
   if (!_IsDoorWarded())
   {
      if (_breakableComponent && _breakableComponent->IsBroken())
      {
         return TAG_AI_TargetingGroup_SmartObject_BrokenObject;
      }
      // This doesn't explicitly mean that the object is in an incorrect state,
      // just that it should join the set of objects that should be checked for
      // an incorrect state. Given that it is only made visible when broken or
      // in an incorrect state, it also shouldn't be checked for it until it
      // is actually necessary.
      return TAG_AI_TargetingGroup_SmartObject_IncorrectState;
   }
   return FGameplayTag::EmptyTag;
}

bool ATATSwingingDoor::CanBeSeenFrom(const FVector& observerLocation,
   FVector& outSeenLocation,
   int32& numberOfLoSChecksPerformed,
   float& outSightStrength,
   const AActor* ignoreActor,
   const bool* wasVisible,
   int32* userData) const
{
   const FBox actorBounds = _portalComponent->Bounds.GetBox();;
   return UTATPerceptionFunctionLibrary::CanActorBoundingBoxBeSeenFromLocation(
      this,
      actorBounds,
      observerLocation,
      outSeenLocation,
      numberOfLoSChecksPerformed,
      outSightStrength,
      ignoreActor
   );
}

bool ATATSwingingDoor::CanActivateWard_Implementation() const
{
   return !_IsDoorWarded();
}

void ATATSwingingDoor::OnWardActivated_Implementation(AActor* wardActor, APawn* instigator)
{
   ensure(wardActor != nullptr);

   if (_currentWard.Get() != nullptr)
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("OnWardActivated: Door already has existing ward '%s'. Replacing with new ward '%s'"),
         *GetNameSafe(_currentWard.Get()), *GetNameSafe(wardActor));
   }

   _currentWard = wardActor;

   // Open the door when warded so players can see what's on the other side
   if (HasAuthority() && _state.Position == ESwingingDoorPosition::Closed && !IsLocked())
   {
      if (instigator != nullptr)
      {
         _SetPosition(OpenDirectionFor(instigator->GetActorLocation()));
      }
      else
      {
         const EDoorOpenDirection allowedOpenDirection = _GetAllowedDirections().AllowedOpenDirection;
         if (allowedOpenDirection == EDoorOpenDirection::OpenBackOnly)
         {
            _SetPosition(ESwingingDoorPosition::OpenBack);
         }
         else if (allowedOpenDirection != EDoorOpenDirection::None)
         {
            _SetPosition(ESwingingDoorPosition::OpenFront);
         }
      }
   }
}

void ATATSwingingDoor::OnWardDeactivated_Implementation(AActor* wardActor)
{
   ensure(wardActor != nullptr);
   AActor* currentWard = _currentWard.Get();
   if (currentWard != nullptr && wardActor == currentWard)
   {
      _currentWard.Reset();
   }
}

bool ATATSwingingDoor::AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->IsStateCorrect(allowIgnoringOfState);
}

FGameplayTagCountContainer& ATATSwingingDoor::GetGameplayTagCountContainer()
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->GetIncorrectStateTags();
}

#if WITH_EDITOR

void ATATSwingingDoor::CheckForErrors()
{
   Super::CheckForErrors();

   TArray<USceneComponent*> pivotAndDescendants;
   _pivot->GetChildrenComponents(true, pivotAndDescendants);
   pivotAndDescendants.Add(_pivot);

   const TArray<FCollisionProfileName>& allowedCollisionProfilesForPivotMeshes = UTATProjectSettings::Get().AllowedCollisionProfilesForOpenableDoorsAndWindows;

   FMessageLog msgLog(FName("MapCheck"));

   for (USceneComponent* comp : pivotAndDescendants)
   {
      if (auto* primitiveComp = Cast<UPrimitiveComponent>(comp))
      {
         const FName collisionProfileName = primitiveComp->GetCollisionProfileName();
         const bool isCollisonProfileAllowed = allowedCollisionProfilesForPivotMeshes.ContainsByPredicate([&](const FCollisionProfileName& item)
         {
            return item.Name == collisionProfileName;
         });

         if (!isCollisonProfileAllowed)
         {
            FString allowedCollisonProfileNamesString = TEXT("(");
            for (const FCollisionProfileName& allowedCollisionProfile : allowedCollisionProfilesForPivotMeshes)
            {
               allowedCollisonProfileNamesString += allowedCollisionProfile.Name.ToString();
               allowedCollisonProfileNamesString += TEXT(",");
            }
            allowedCollisonProfileNamesString += TEXT(")");

            msgLog.Error()
               ->AddToken(FUObjectToken::Create(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
                  TEXT("ATATSwingingDoor '%s' (BP: '%s') has primitive component '%s' with collision profile '%s'. ")
                  TEXT("If a primitive component is part of the pivot, it needs to have a collision profile that's one of: %s."),
                  *GetActorLabel(),
                  *GetClass()->GetName(),
                  *primitiveComp->GetName(),
                  *collisionProfileName.ToString(),
                  *allowedCollisonProfileNamesString))));
         }
      }
   }

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      // actually only matters with DORM_Initial
      if (_lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled && (_locked != GetClass()->GetDefaultObject<ATATSwingingDoor>()->_locked))
      {
         msgLog.Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
               TEXT("ATATSwingingDoor %s is locked by default, but is using a lock spawner. This currently has probably-dormancy bugs. So start with it unlocked. The behavior will be the same (but without the bugs). But still complain about this bug so it actually gets fixed."),
               *GetActorLabel()))));
      }
      
      _lockConfig.CheckForErrors([this, &msgLog](FText message)
      {
         msgLog.Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(MoveTemp(message)));
      });
   }
}
#endif

// Called when the game starts or when spawned
void ATATSwingingDoor::BeginPlay()
{
   Super::BeginPlay();

   SnapTo(_state.Position);
   OnDoorPositionChanged(_state.Position);
   _securityLockdownComponent->OnSecurityLockdownStateChanged.AddUniqueDynamic(this, &ThisClass::_OnSecurityLockdownStateChanged);

   if (HasAuthority())
   {
      _breakableComponent->OnBrokenAuthority.AddUObject(this, &ThisClass::_AuthorityOnBroken);
   }
   _breakableComponent->OnBrokenChanged.AddDynamic(this, &ThisClass::OnBrokenChanged);

   // Init lock config
   _lockConfig.RandomizeLockLevel(this);

   if (PortalAlwaysOpen)
   {
      _portalComponent->EnablePortal();
   }
}

// Called every frame
void ATATSwingingDoor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   // rotate towards new position
   // TODO: if turning at non-constant velocity, add a new variable for progress, and apply curve to that?
   const FRotator currentRotation = _pivot->GetRelativeRotation();
   const float currentAngle = currentRotation.GetComponentForAxis(_swingAxis);
   const float targetAngle = AngleFor(_state.Position);

   // assumption is that the convenience of specifying the duration vs speed outweighs the cost of calculating it (is this true?)
   const float brokenSpeedMultiplier = IsBroken() ? _brokenSwingSpeedMultiplier : 1;
   const float angleSpeed = brokenSpeedMultiplier * FMath::Abs(_openBackAngle - _closedAngle) * 0.5f / _swingDuration;

   // explicitly not using shortest angle in case that is the wrong direction
   float newAngle = FMath::FInterpConstantTo(currentAngle, targetAngle, deltaTime, angleSpeed);
   if (FMath::IsNearlyEqual(targetAngle, newAngle))
   {
      newAngle = targetAngle;
      PrimaryActorTick.SetTickFunctionEnable(false);
   }

   if (!PortalAlwaysOpen)
   {
      _UpdatePortalObstructionForAngle(newAngle);
   }

   FRotator newRotation = currentRotation;
   newRotation.SetComponentForAxis(_swingAxis, newAngle);
   _SetPivotRotation(newRotation);
}

UTATSmartObjectComponent* ATATSwingingDoor::GetSmartObjectComponent() const
{
   return _incorrectStateActionNodeComponent;
}

void ATATSwingingDoor::_SetPivotRotation(FRotator newRotation)
{
   _pivot->SetRelativeRotation(newRotation);

   if (_secondaryPivot)
   {
      newRotation.SetComponentForAxis(_swingAxis, _secondaryAngleOffset - newRotation.GetComponentForAxis(_swingAxis));
      _secondaryPivot->SetRelativeRotation(newRotation);
   }

   _RecomputePivotBounds();
}

void ATATSwingingDoor::SetLocked(bool newIsLocked)
{
   if (newIsLocked == _locked) return;

   FlushNetDormancy();
   _locked = newIsLocked;
   
   if (HasAuthority())
   {
      // Reset the tracks completed
      _lockpickCurrentTrack = 0;
      OnAuthorityDoorRecentlyChangedState.Broadcast();
   }
}

void ATATSwingingDoor::LockWithKey()
{
   SetLockDirection(EDoorLockDirection::Both);
   SetLocked(true);
}

void ATATSwingingDoor::OnLockpickTrackCompleted(int32 trackIndex)
{
   if (trackIndex < _lockpickCurrentTrack)
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("OnLockpickTrackCompleted() called for a track that was not active"));
      return;
   }
   else if (!IsLocked())
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("OnLockpickTrackCompleted() called on an unlocked actor"));
      return;
   }

   FlushNetDormancy();
   _lockpickCurrentTrack = trackIndex + 1;
}

/// This is for debugging purposes only.
void ATATSwingingDoor::SetClosedState(bool newIsClosed)
{
   const ESwingingDoorPosition newPosition =  newIsClosed ? ESwingingDoorPosition::Closed : ESwingingDoorPosition::OpenFront;
   _SetPosition(newPosition);
}

bool ATATSwingingDoor::IsBroken() const
{
   return _breakableComponent->IsBroken() || _state.bIsBroken;
}

void ATATSwingingDoor::SetLockDirection(EDoorLockDirection lockDirection)
{
   if (lockDirection != _lockDirection)
   {
      FlushNetDormancy();
      _lockDirection = lockDirection;
   }
}

bool ATATSwingingDoor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return !IsBroken() && !_IsDoorWarded() && !_interactionGates.IsInteractionBlockedWithoutMessage();
}

void ATATSwingingDoor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (!IsOpen() && !_IsOpenableFromActorDirection(interactingCharacter))
   {
      prompt.ErrorMessage = _unopenableFromSidePrompt;
      return;
   }

   if(_interactionGates.TryAddToPrompt(prompt))
   {
      return;
   }
   
   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   if (!lockContext.bIsLockedInCurrentDirection)
   {
      prompt.PressAction = IsOpen() ? _closePrompt : _openPrompt;
   }
   _lockConfig.AddToPrompt(prompt, lockContext, interactingCharacter);
}

FInteractStartResult ATATSwingingDoor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   check(interactingCharacter);

   if (!IsOpen() && !_IsOpenableFromActorDirection(interactingCharacter))
   {
      return {};
   }

   if(_interactionGates.IsInteractionBlocked())
   {
      return {};
   }

   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   FInteractStartResult result;
   if(_lockConfig.TryHandleInteractStart(this, interactingCharacter, lockContext, result))
   {
      if (!lockContext.bIsLockedInCurrentDirection)
      {
         _PopulateAnimationTagForSwing(result);
      }

      return result;
   }
   else if (!lockContext.bIsLockedInCurrentDirection)
   {
      _SwingForInteract(interactingCharacter, false);
      _PopulateAnimationTagForSwing(result);
   }

   return result;
}

bool ATATSwingingDoor::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (!interactingCharacter)
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("EndInteract called with null character"));
      return false;
   }

   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   if (context.IsProbablyInstant() && !lockContext.bIsLockedInCurrentDirection)
   {
      _SwingForInteract(interactingCharacter, false);
   }
   else if (context.IsComplete())
   {
      _lockConfig.HandleInteractComplete(this, interactingCharacter, lockContext);
   }

   return false;
}

void ATATSwingingDoor::_SwingForInteract(ACharacter* interactingCharacter, bool broken)
{
   // Still set position even if on client. Server can override later if wrong, and it will
   // correct. The major hole here is that oscillation can occur on the client if the player
   // interacts with the door at a higher frequency than the round trip latency with the server
   // (as old updates will clobber new input), but this might be okay.
   // TODO: should we allow opening/closing doors while they are still animating?

   ESwingingDoorPosition newPosition = _state.Position == ESwingingDoorPosition::Closed ?
      OpenDirectionFor(interactingCharacter->GetActorLocation()) :
      ESwingingDoorPosition::Closed;

   _SetPosition(newPosition);

}

void ATATSwingingDoor::_SetPosition(ESwingingDoorPosition newPosition)
{
   // Not Authority-only for client prediction
   if (newPosition == _state.Position)
   {
      return;
   }

   FlushNetDormancy();
   const FTATSwingingDoorState oldState = _state;
   _state.Position = newPosition;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);

   if (HasAuthority())
   {
      // Switching from closed to any open state
      if (_state.Position != ESwingingDoorPosition::Closed)
      {
         // Cancel lockpicking on anyone currently lockpicking this door
         _onRequestCancelLockpicking.Broadcast();

         // Opening a locked door implicitly unlocks it, as per Rick
         SetLocked(false);
      }

      const int32 currentState = IsOpen() ? 1 : 0;
      _incorrectStateActionNodeComponent->SetCurrentState(currentState);
      OnAuthorityDoorRecentlyChangedState.Broadcast();
   }
   
   OnRep_State(oldState);
}

void ATATSwingingDoor::_UpdatePortalObstructionForAngle(float angle)
{
   // In case we have asymmetrical open-angles (and the door is being closed), use the larger one 
   // so we never end up with a normalized angle outside the 0 <-> 1 range.
   // 
   // NOTE: if the difference is significant, this can produce noticeable obstruction snapping when closing 
   // the door from the side with a smaller open-angle. Ideally we could infer the direction the door was 
   // previously open in, but FTATSwingingDoorState does not provide that info.
   float openAngle = 0.f;
   if (_state.Position == ESwingingDoorPosition::Closed)
   {
      const float openFrontAngle = AngleFor(ESwingingDoorPosition::OpenFront);
      const float openBackAngle = AngleFor(ESwingingDoorPosition::OpenBack);
      openAngle = FMath::Abs(openFrontAngle) > FMath::Abs(openBackAngle) ? openFrontAngle : openBackAngle;
   }
   // Otherwise use angle door is being opened to
   else
   {
      openAngle = AngleFor(_state.Position);
   }
   const float closedAngle = AngleFor(ESwingingDoorPosition::Closed);

   // Set portal obstruction to door angle normalized between 0 <-> 1 (open <-> closed)
   const float angleRange = FMath::Abs(closedAngle - openAngle);
   float normalizedAngle = FMath::Abs(angle - closedAngle);
   normalizedAngle = FMath::Abs(normalizedAngle / angleRange);

   if (DoorCVars::InterpolateAudioPortals)
   {
      const float portalObstruction = 1.f - normalizedAngle;
      const float portalOcclusion = 0.f;
      UAkGameplayStatics::SetPortalObstructionAndOcclusion(_portalComponent, portalObstruction, portalOcclusion);
   }

   // Set portal open/closed based on angle
   if (FMath::IsNearlyEqual(angle, closedAngle))
   {
      _portalComponent->DisablePortal();
   }
   else
   {
      _portalComponent->EnablePortal();
   }
}

void ATATSwingingDoor::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

bool ATATSwingingDoor::DoesSupportInteractionBy_Implementation(ACharacter* interactor, const FGameplayTag& interactorIdentity) const
{
   return !_IsDoorWarded() && _allowedInteractors.HasTag(interactorIdentity);
}

void ATATSwingingDoor::_OnSecurityLockdownStateChanged(bool bIsInLockdown)
{
   UpdateLockdownVisual(bIsInLockdown);
   if(IsBroken())
   {
      // Door has been broken, do nothing.
      return;
   }
   if(HasAuthority())
   {
      if (bIsInLockdown)
      {
         _SetPosition(ESwingingDoorPosition::Closed);
         // Lockdown always locks on both sides
         SetLockDirection(EDoorLockDirection::Both);
         SetLocked(true);
      }
      FlushNetDormancy();
      _lockedDown = bIsInLockdown;
   }
}

bool ATATSwingingDoor::CanTraverseDoor(const UObject* probablyController, bool isFront) const
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_SwingingDoor_CanTraverseDoor);
   if (_IsDoorWarded())
   {
      return false;
   }

   if (IsOpen())
   {
      return true;
   }

   const bool isBlockedByGate = _interactionGates.IsInteractionBlocked();
   if (_IsOpenableFromDirection(isFront) && !isBlockedByGate && (!IsLocked() || !_DoesLockApplyInDirection(isFront)))
   {
      return true;
   }

   const ACharacter* character = Cast<ACharacter>(probablyController);
   const AController* controller = Cast<AController>(probablyController);
   if (character == nullptr && controller != nullptr)
   {
      character = controller->GetPawn<ACharacter>();
   }
   else if(character != nullptr && controller == nullptr)
   {
      controller = character->GetController();
   }

   // deferring character lookup until it is needed. It is not yet
   // clear how performance-sensitive this function is
   if (IsValid(controller) && IsValid(character))
   {
      return CanControllerBreakDoor(controller) || (!isBlockedByGate && _lockConfig.DoesCharacterHaveKey(character));
   }

   return false;
}

void ATATSwingingDoor::OnDoorRecentlySwung_Implementation(ESwingingDoorPosition newPosition)
{
   if (!IsNetMode(NM_DedicatedServer))
   {
      // Post open/close audio event
      UAkAudioEvent* audioEvent = newPosition == ESwingingDoorPosition::Closed ? _closedAudioEvent : _openedAudioEvent;
      const FVector postAtLocation = GetActorTransform().TransformPositionNoScale(_audioEventPostLocation);
      UAkGameplayStatics::PostEventAtLocation(audioEvent, postAtLocation, GetActorRotation(), this);
   }
}

bool ATATSwingingDoor::ShouldVisualizeLockInDirection(bool isFrontSide) const
{
   const bool isSpawnerDrivenLock = _lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled;
   const bool potentiallyLocked = _locked || isSpawnerDrivenLock;
   return potentiallyLocked && _DoesLockApplyInDirection(isFrontSide);
}

bool ATATSwingingDoor::ShouldVisualizeUnopenableDirection(bool isFrontSide) const
{
    return !_IsOpenableFromDirection(isFrontSide);
}

void ATATSwingingDoor::SetSecondaryPivot(USceneComponent* secondaryPivot)
{
   _secondaryPivot = secondaryPivot;
}

void ATATSwingingDoor::_PopulateAnimationTagForSwing(FInteractStartResult& result) const
{
   result.InstantAnimationTag = IsOpen() ? _closeInteractAnimationTag : _openInteractAnimationTag;
}

bool ATATSwingingDoor::_IsOnFrontSide(const FVector& characterPosition) const
{
   // do it in actor-local space for the heck of it
   // local character position is also unnormalized direction of the
   FVector localCharacterPosition = GetActorTransform().InverseTransformPositionNoScale(characterPosition);
   localCharacterPosition -= _pivot->GetRelativeLocation().ProjectOnTo(_frontDirection);
   return FVector::DotProduct(localCharacterPosition, _frontDirection) > 0;
}

ESwingingDoorPosition ATATSwingingDoor::OpenDirectionFor(const FVector& characterPosition) const
{
   return _IsOnFrontSide(characterPosition) ? ESwingingDoorPosition::OpenBack : ESwingingDoorPosition::OpenFront;
}

FRotator ATATSwingingDoor::RotatorFor(ESwingingDoorPosition position) const
{
   FRotator rotator(ForceInitToZero);
   rotator.SetComponentForAxis(_swingAxis, AngleFor(position));
   return rotator;
}

float ATATSwingingDoor::AngleFor(ESwingingDoorPosition position) const
{
   switch (position)
   {
   case ESwingingDoorPosition::OpenFront:
      return _openFrontAngle;
   case ESwingingDoorPosition::OpenBack:
      return _openBackAngle;
   case ESwingingDoorPosition::Closed:
   default:
      return _closedAngle;
   }
}

void ATATSwingingDoor::SnapTo(ESwingingDoorPosition position)
{
   _SetPivotRotation(RotatorFor(position));
   if (!PortalAlwaysOpen)
   {
      _UpdatePortalObstructionForAngle(AngleFor(position));
   }
}

void ATATSwingingDoor::OnRecentlyBroken_Implementation(bool wasClosed)
{
   if (!IsNetMode(NM_DedicatedServer))
   {
      UAkAudioEvent* audioEvent = wasClosed ? _brokenWhileClosedAudioEvent : _brokenWhileOpenAudioEvent;
      const FVector postAtLocation = GetActorTransform().TransformPositionNoScale(_audioEventPostLocation);
      UAkGameplayStatics::PostEventAtLocation(audioEvent, postAtLocation, GetActorRotation(), this);
   }
}

void ATATSwingingDoor::OnBrokenChanged_Implementation(bool isBroken)
{
   if (isBroken)
   {
      // Open portal and clear obstruction/occlusion values
      _portalComponent->EnablePortal();
      if (DoorCVars::InterpolateAudioPortals)
      {
         const float obstruction = 0.f;
         const float occlusion = 0.f;
         UAkGameplayStatics::SetPortalObstructionAndOcclusion(_portalComponent, obstruction, occlusion);
      }
   }
   else
   {
      if (!PortalAlwaysOpen)
      {
         _UpdatePortalObstructionForAngle(AngleFor(_state.Position));
      }

      if (HasAuthority() && _state.bIsBroken)
      {
         FlushNetDormancy();
         _state.bIsBroken = false;
      }
   }
}

FTATSwingingDoorAllowedDirections ATATSwingingDoor::_GetAllowedDirections() const
{
   return (_lockedDown && _overrideLockdownDirections) ? _lockdownAllowedDirections : _normalAllowedDirections;
}

FLockInteractContext ATATSwingingDoor::MakeLockContext(ACharacter* interactingCharacter) const
{
   const bool isOnFront = _IsOnFrontSide(interactingCharacter->GetActorLocation());
   const FTATSwingingDoorAllowedDirections allowedDirections = _GetAllowedDirections();
   
   FLockInteractContext ctx;
   ctx.bIsLocked = _locked;
   ctx.bIsLockRelevant =_state.Position == ESwingingDoorPosition::Closed;
   ctx.bAllowsKey = _lockConfig.KeyTag.IsValid() && DoorHelpers::MatchDirection(allowedDirections.KeyDirection, isOnFront);
   ctx.bHasKey = ctx.bIsLockRelevant && _lockConfig.DoesCharacterHaveKey(interactingCharacter);
   ctx.bCanInteractorLockpick = _lockConfig.CanBeLockpicked && FTATLockConfig::CanActorLockpick(interactingCharacter);
   ctx.bCanBePickedInCurrentDirection = DoorHelpers::MatchDirection(allowedDirections.LockpickableDirection, isOnFront);
   ctx.bIsLockedInCurrentDirection = _locked && ctx.bIsLockRelevant && _DoesLockApplyInDirection(isOnFront);
   ctx.bCanBeRelockedInCurrentDirection = !_locked && ctx.bIsLockRelevant && DoorHelpers::MatchDirection(allowedDirections.RelockDirection, isOnFront); 
   // the above two conditions are mutually exclusive, so it will only evaluate lock direction at most once
   ctx.bAreAllSidesLocked = _lockDirection == EDoorLockDirection::Both;
   return ctx;
}

bool ATATSwingingDoor::_DoesLockApplyInActorDirection(const AActor* actor) const
{
   return _DoesLockApplyInDirection(_IsOnFrontSide(actor->GetActorLocation()));
}

bool ATATSwingingDoor::_DoesLockApplyInDirection(bool isOnFront) const
{
   return DoorHelpers::MatchDirection(_lockDirection, isOnFront);
}

bool ATATSwingingDoor::_IsOpenableFromActorDirection(const AActor* actor) const
{
   // minor duplication to calculate side lazily (can replace if not useful)
   const EDoorOpenDirection allowedOpenDirection = _GetAllowedDirections().AllowedOpenDirection;
   if(allowedOpenDirection == EDoorOpenDirection::Both)
   {
      return true;
   }
   
   return DoorHelpers::MatchDirection(allowedOpenDirection, _IsOnFrontSide(actor->GetActorLocation()));
}

bool ATATSwingingDoor::_IsOpenableFromDirection(bool isOnFront) const
{
   const EDoorOpenDirection allowedOpenDirection = _GetAllowedDirections().AllowedOpenDirection;
   return DoorHelpers::MatchDirection(allowedOpenDirection, isOnFront);
}

void ATATSwingingDoor::_AuthorityOnBroken(const FTATAuthorityBreakContext& context)
{
   // Redundantly set broken flag on the actor
   //  The use-case is that the swing sound on open-from-broken may be different,
   //  and the main breakage is a replicated subobject which would likely be in the
   //  same bunch, but might be hard to know at the time of OnRep or PostRepNotifies.
   //  I didn't try very hard, but this is easy to reason about an modest overhead.
   //  This should not be necessary for most breakables, however.
   FlushNetDormancy();
   _state.bIsBroken = true;
   OnRecentlyBroken(!IsOpen()); //< Not called via SetPosition, as it won't show a diff on broken-ness

   if (!IsOpen())
   {
      // even if the origin is garbage, it is a direction, so don't bother to check
      _SetPosition(OpenDirectionFor(context.OptionalOrigin));
      UE_VLOG_LOCATION(this, LogTemp, Log, context.OptionalOrigin, 10, FColor::Red, TEXT("Break Origin"));
   }
}

bool ATATSwingingDoor::CanControllerBreakDoor(const AController* controller) const
{
   if (!_breakableComponent || !_breakableComponent->IsBreakable() || _IsDoorWarded())
   {
      return false;
   }
   if (const ATATAIController* aiController = Cast<ATATAIController>(controller))
   {
      return aiController->HasTrait(TAG_AI_Trait_CanBreakDownDoors);
   }
   return false;
}

bool ATATSwingingDoor::IsInteractionBlocked() const
{
   return _interactionGates.IsInteractionBlocked() || _IsDoorWarded();
}

void ATATSwingingDoor::OnRep_State(const FTATSwingingDoorState& previous)
{
   if (_state.bIsBroken && !previous.bIsBroken && !_state.IsOld(this, 0.75f))
   {
      OnRecentlyBroken(previous.Position == ESwingingDoorPosition::Closed);
   }

   if (previous.Position == _state.Position) return;

   if (_state.IsOld(this, 0.75f))
   {
      SnapTo(_state.Position);
   }
   else
   {
      //start animating
      PrimaryActorTick.SetTickFunctionEnable(true);
      OnDoorRecentlySwung(_state.Position);
   }
   OnDoorPositionChanged(_state.Position);
}

void ATATSwingingDoor::_AuthorityOnLockedSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // if we "spawn" from the mission system that means lock
   SetLocked(true);
}

void ATATSwingingDoor::_AuthorityOnLockedSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // if we "don't spawn" from the mission system that means unlock
   SetLocked(false);
}

void ATATSwingingDoor::_AuthorityOnOpenSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   check(HasAuthority());

   // Door must be allowed to open in some direction
   if (_normalAllowedDirections.AllowedOpenDirection == EDoorOpenDirection::None)
   {
      UE_LOG(LogTATSwingingDoor, Error, TEXT("%s | _AuthorityOnOpenSpawnerSpawn() failed due to _normalAllowedDirections.AllowedOpenDirection == None!")
         , *GetName());
      return;
   }

   // Should always return at least 1 valid open position, if AllowedOpenDirection != None
   TConstArrayView<ESwingingDoorPosition> allowedSwingOpenPositions = DoorHelpers::GetAllowedSwingOpenPositions(_normalAllowedDirections.AllowedOpenDirection);
   check(!allowedSwingOpenPositions.IsEmpty());

   // Select a random open position
   const int32 randIndex = randomStream.RandHelper(allowedSwingOpenPositions.Num());
   const ESwingingDoorPosition swingingDoorPosition = allowedSwingOpenPositions[randIndex];

   check(swingingDoorPosition != ESwingingDoorPosition::Closed);
   UE_LOG(LogTATSwingingDoor, VeryVerbose, TEXT("%s | _AuthorityOnOpenSpawnerSpawn() spawning door as %s")
      , *GetName()
      , *UEnum::GetValueAsString(swingingDoorPosition));

   _SetPosition(swingingDoorPosition);

   _UpdateIncorrectStateInitialState();

   check(_openSpawnerComponent != nullptr);
   _openSpawnerComponent->AuthorityOnSpawn.RemoveDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerSpawn);
   _openSpawnerComponent->AuthorityOnNotSpawn.RemoveDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerNotSpawn);
}

void ATATSwingingDoor::_AuthorityOnOpenSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   _UpdateIncorrectStateInitialState();

   check(_openSpawnerComponent != nullptr);
   _openSpawnerComponent->AuthorityOnSpawn.RemoveDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerSpawn);
   _openSpawnerComponent->AuthorityOnNotSpawn.RemoveDynamic(this, &ATATSwingingDoor::_AuthorityOnOpenSpawnerNotSpawn);
}

bool FTATSwingingDoorState::IsOld(UObject* worldContext, float thresholdSeconds) const
{
   float now = UOSEInteractionHelpers::GetServerTimeForComparison(worldContext);
   return ChangedServerTime == 0 || (now - ChangedServerTime) >= thresholdSeconds;
}

void ATATSwingingDoor::_RecomputePivotBounds()
{
   // TODO: avoid allocating array for this? (or use interaction as heuristic)
   TArray<USceneComponent*> pivotComponents;
   _pivot->GetChildrenComponents(true, pivotComponents);

   _pivotBounds = _pivot->Bounds;
   for (USceneComponent* pivotComponent : pivotComponents)
   {
      _pivotBounds = _pivotBounds + pivotComponent->Bounds;
   }
   if(_secondaryPivot)
   {
      _secondaryPivot->GetChildrenComponents(true, pivotComponents);
      for (USceneComponent* pivotComponent : pivotComponents)
      {
         _pivotBounds = _pivotBounds + pivotComponent->Bounds;
      }
   }
}

#if WITH_EDITOR
void ATATSwingingDoor::_RefreshAkPostLocationEditorPreview()
{
   if (_akEventPostLocationEditorBillboard)
   {
      // Position where ak event will be played
      _akEventPostLocationEditorBillboard->SetRelativeLocation(_audioEventPostLocation);
   }
}
#endif // WITH_EDITOR

void ATATSwingingDoor::_RefreshPivotComponentOffsets()
{
   if (UTATSwingingDoorPivotComponent* pivotComponent = Cast<UTATSwingingDoorPivotComponent>(_pivot))
   {
      const FRotator currentRotation = _pivot->GetRelativeRotation();
      const FRotator desiredRotation = RotatorFor(ESwingingDoorPosition::Closed);
      pivotComponent->SetOffsetFromClosed(FTransform(desiredRotation - currentRotation));
   }
   else
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("%s | _RefreshPivotComponentOffsets() called for _pivot which does is not an UTATSwingingDoorPivotComponent")
         , *GetName());
   }

   if (UTATSwingingDoorPivotComponent* secondaryPivotComponent = Cast<UTATSwingingDoorPivotComponent>(_secondaryPivot))
   {
      const FRotator currentRotation = _secondaryPivot->GetRelativeRotation();
      FRotator desiredRotation = RotatorFor(ESwingingDoorPosition::Closed);
      desiredRotation.SetComponentForAxis(_swingAxis, _secondaryAngleOffset - desiredRotation.GetComponentForAxis(_swingAxis));
      secondaryPivotComponent->SetOffsetFromClosed(FTransform(desiredRotation - currentRotation));
   }
   else
   {
      UE_LOG(LogTATSwingingDoor, Warning, TEXT("%s | _RefreshPivotComponentOffsets() called for _secondaryPivot which does is not an UTATSwingingDoorPivotComponent")
         , *GetName());
   }
}

void ATATSwingingDoor::_UpdateIncorrectStateInitialState()
{
   if (_incorrectStateActionNodeComponent != nullptr)
   {
      const int32 currentState = IsOpen() ? 1 : 0;
      _incorrectStateActionNodeComponent->SetInitialState(currentState, currentState);
   }
}

#undef LOCTEXT_NAMESPACE
