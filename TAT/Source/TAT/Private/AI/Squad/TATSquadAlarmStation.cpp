// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Squad/TATSquadAlarmStation.h"

// tat
#include "AI/SmartObjects/TATSmartObjectObjectTags.h"
#include "AI/Squad/TATSquadAlarmStationSettings.h"
#include "AI/Perception/TATAISense_Sight.h"
#include "AI/TATAISettings.h"
#include "Character/TATTeams.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATEndgameActionComponent.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "WorldMap/TATMapActorComponent.h"
#include "Interactables/TATLockConfig.h"
#include "AI/Coordinators/TATLockdownCoordinator.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSquadAlarmStation)

UE_DEFINE_GAMEPLAY_TAG(TAG_AlarmStation_SmartObject_Activity, "SmartObjects.Activity.Alarm");
UE_DEFINE_GAMEPLAY_TAG(TAG_AlarmStation_Reset_SmartObject_Activity, "SmartObjects.Activity.ResetAlarm");

#define LOCTEXT_NAMESPACE "TATSquadAlarmStation" 

DEFINE_LOG_CATEGORY(LogTATSquadAlarmStation);

ATATSquadAlarmStation::ATATSquadAlarmStation()
{
   PrimaryActorTick.bCanEverTick = false;

   bReplicates = true;
   bGenerateOverlapEventsDuringLevelStreaming = true;
   NetDormancy = DORM_Initial;

   USceneComponent* sceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent = sceneComponent;
   RootComponent->SetMobility(EComponentMobility::Static);
   
   _actionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent>(TEXT("ActionNode"));
   _actionNodeComponent->SetupAttachment(RootComponent);

   _mapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   _mapActorComponent->bAutoActivate = false;

   _endgameActionComponent = CreateDefaultSubobject<UTATEndgameActionComponent>(TEXT("EndgameAction"));
   _endgameActionComponent->SetDefaultAction(ETATDefaultEndgameAction::TriggerTrapAction);

   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
}

uint8 ATATSquadAlarmStation::GetTeam() const
{
   return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Guard);
}

void ATATSquadAlarmStation::BeginPlay()
{   
   Super::BeginPlay();
   _lockConfig.RandomizeLockLevel(this);
   // AI need to see us to correct our incorrect state
   if (ensure(_perceptionStimuliSource))
   {
      _perceptionStimuliSource->RegisterForSense(UTATAISense_Sight::StaticClass());
   }
   if(HasAuthority())
   {
      _SetTagsForState(_alarmState.State);
   }
   _TriggerDelegates(false, _alarmState);
}

void ATATSquadAlarmStation::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATSquadAlarmStation, _alarmState);
   DOREPLIFETIME(ATATSquadAlarmStation, _lockpickCurrentTrack);
}

void ATATSquadAlarmStation::AuthorityTrigger()
{
   if (!ensure(HasAuthority()) || !IsReady())
      return;

   _SetState(EAlarmState::Triggered);
}

void ATATSquadAlarmStation::AuthorityArm()
{
   if (!ensure(HasAuthority()))
      return;

   _lockpickCurrentTrack = 0;
   _SetState(EAlarmState::Ready);
   UTATLockdownCoordinator* lockdownCoordinator = GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
   if(lockdownCoordinator != nullptr)
   {
      lockdownCoordinator->AbandonLockdown(this);
   }
}

void ATATSquadAlarmStation::_OnRep_AlarmState(const FAlarmState& previousState)
{
   if (previousState.State == _alarmState.State)
      return;

   const bool isRecent = !UOSEInteractionHelpers::IsOld(this, _alarmState.ChangedServerTime);
   _TriggerDelegates(isRecent, previousState);

   const bool showOnMap = _alarmState.State == EAlarmState::Triggered;
   _mapActorComponent->SetActive(showOnMap);
}

void ATATSquadAlarmStation::OnResetAlarm()
{
   _SetState(EAlarmState::Ready);
   GetWorld()->GetTimerManager().ClearTimer(_alarmForcedResetHandle);
}

void ATATSquadAlarmStation::_TriggerDelegates(const bool isRecent, const FAlarmState& previousState)
{
   K2_OnAlarmStateChanged(_alarmState.State, previousState.State, isRecent);
   OnAlarmStateChanged.Broadcast(_alarmState.State, previousState.State, isRecent);
   if (isRecent)
   {
      K2_OnAlarmStateChangedRecently(_alarmState.State, previousState.State);
   }
   if (previousState.State == EAlarmState::Triggered)
   {
      _onRequestCancelLockpicking.Broadcast();
   }
}

bool ATATSquadAlarmStation::IsReady() const
{
   return _alarmState.State == EAlarmState::Ready;
}

void ATATSquadAlarmStation::_SetState(const EAlarmState newState)
{
   if (newState == _alarmState.State)
      return;

   FlushNetDormancy();
   const FAlarmState oldState = _alarmState;
   _alarmState.State = newState;
   _alarmState.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_AlarmState(oldState);

   _SetTagsForState(newState);

   if (ShouldForceResetOnAlarm)
   {
      if(_alarmForcedResetHandle.IsValid() == false && newState == EAlarmState::Triggered)
      {
         GetWorld()->GetTimerManager().SetTimer(_alarmForcedResetHandle, FTimerDelegate::CreateUObject(this, &ThisClass::OnResetAlarm), _TimeUntilAlarmForcedToReset, false);
      }
   }
}

void ATATSquadAlarmStation::_SetTagsForState(const EAlarmState state)
{
   // NOTE: Counting Destroyed as a correct state for now. The interactable
   //       for repairing the breakable is the breakable component rather
   //       than the actor, and the IncorrectState interface no longer has
   //       the ability to specify the interactable to interact with
   _gameplayTagCountContainer.SetTagCount(TAG_AlarmStation_Reset_SmartObject_Activity, 0);
   switch (state)
   {
      case EAlarmState::Triggered:
         _OnTriggerAlarmStim();
         _gameplayTagCountContainer.SetTagCount(TAG_AlarmStation_SmartObject_Activity, 0);
         break;
      case EAlarmState::Ready:
         _gameplayTagCountContainer.SetTagCount(TAG_AlarmStation_SmartObject_Activity, 1);
         break;
   }
}

bool ATATSquadAlarmStation::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return true;
}

void ATATSquadAlarmStation::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

FLockInteractContext ATATSquadAlarmStation::MakeLockContext(const ACharacter* interactingCharacter) const
{
   FLockInteractContext ctx;
   ctx.bIsLocked = GetState() == EAlarmState::Triggered;
   ctx.bIsLockRelevant = ctx.bIsLocked;
   ctx.bAllowsKey = _lockConfig.KeyTag.IsValid();
   ctx.bHasKey = ctx.bIsLockRelevant && _lockConfig.DoesCharacterHaveKey(interactingCharacter);
   ctx.bCanInteractorLockpick = _lockConfig.CanBeLockpicked && FTATLockConfig::CanActorLockpick(interactingCharacter);
   ctx.bCanBePickedInCurrentDirection = true;
   ctx.bIsLockedInCurrentDirection = ctx.bIsLocked;
   ctx.bCanBeRelockedInCurrentDirection = false;
   ctx.bAreAllSidesLocked = ctx.bIsLocked;
   return ctx;
}

void ATATSquadAlarmStation::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   if(GetState() == EAlarmState::Ready)
   {
      prompt.PressAction = LOCTEXT("PressArmedPrompt", "Trigger Alarm");
   }
   else
   {
      _lockConfig.AddToPrompt(prompt, lockContext, interactingCharacter);
   }
   prompt.InteractStatusTag = InteractionStatusTag;
}

FInteractStartResult ATATSquadAlarmStation::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;
   if(IsInState(EAlarmState::Triggered))
   {
      const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
      if(_lockConfig.TryHandleInteractStart(this, interactingCharacter, lockContext, result))
      {
         result.HoldAnimationTag = ArmingAnimationTag;
         result.Delay = _HoldTimeToDisableAlarm;
      }
      return result;
   }
   const UTATSquadAlarmStationSettings& alarmSettings = UTATSquadAlarmStationSettings::Get();
   result = FInteractStartResult::Wait(alarmSettings.HoldDuration);
   result.InstantAnimationTag = TriggeringAnimationTag;
   return result;
}

bool ATATSquadAlarmStation::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (HasAuthority() == false)
   {
      return false;
   }
   if(context.IsProbablyInstant())
   {
      if(IsInState(EAlarmState::Ready))
      {
         AuthorityTrigger();
      }
   }
   if (context.IsComplete())
   {
      if(IsInState(EAlarmState::Triggered))
      {
         const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
         _lockConfig.HandleInteractComplete(this, interactingCharacter, lockContext);
      }
   }
   return false;
}

bool ATATSquadAlarmStation::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation,
   int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible,
   int32* userData) const
{
   static constexpr bool kNonColliding = false;
   static constexpr bool kIncludeFromChildActors = false;
   const FBox actorBounds = GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);
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

bool ATATSquadAlarmStation::HasMatchingGameplayTag(FGameplayTag tagToCheck) const
{
   FGameplayTagContainer container;
   GetOwnedGameplayTags(container);
   return container.HasTag(tagToCheck);
}

bool ATATSquadAlarmStation::HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const
{
   FGameplayTagContainer container;
   GetOwnedGameplayTags(container);
   return container.HasAll(tagContainer);
}

bool ATATSquadAlarmStation::HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const
{
   FGameplayTagContainer container;
   GetOwnedGameplayTags(container);
   return container.HasAny(tagContainer);
}

void ATATSquadAlarmStation::GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const
{
   tagContainer.AppendTags(_gameplayTagCountContainer.GetExplicitGameplayTags());
}

void ATATSquadAlarmStation::Unlock()
{
   if (GetState() != EAlarmState::Triggered)
   {
      UE_LOG(LogTATSquadAlarmStation, Warning, TEXT("Trying to unlock an alarm that isn't triggered"));
      return;
   }
   if (HasAuthority())
   {
      AuthorityArm();
   }
}

void ATATSquadAlarmStation::Lock()
{
   UE_LOG(LogTATSquadAlarmStation, Warning, TEXT("Trying to lock an alarm - that shouldn't be possible"));
}

void ATATSquadAlarmStation::OnLockpickTrackCompleted(int32 trackIndex)
{
   if (trackIndex < _lockpickCurrentTrack)
   {
      UE_LOG(LogTATSquadAlarmStation, Warning, TEXT("OnLockpickTrackCompleted() called for a track that was not active"));
      return;
   }
   if (GetState() != EAlarmState::Triggered)
   {
      UE_LOG(LogTATSquadAlarmStation, Warning, TEXT("OnLockpickTrackCompleted() called on an unlocked actor"));
      return;
   }

   FlushNetDormancy();
   _lockpickCurrentTrack = trackIndex + 1;
}

void ATATSquadAlarmStation::TriggerActionFromTrap_Implementation(AActor* optionalTarget)
{
   if(HasAuthority())
   {
      AuthorityTrigger();
   }
}

void ATATSquadAlarmStation::_OnTriggerAlarmStim_Implementation()
{
}

#undef LOCTEXT_NAMESPACE

