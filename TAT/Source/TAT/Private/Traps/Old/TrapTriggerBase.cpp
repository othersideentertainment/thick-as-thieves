// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/TrapTriggerBase.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Traps/Old/TrapEmitterInterface.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TrapTriggerBase)

DEFINE_LOG_CATEGORY_STATIC(LogTrapTriggerBase_Old, Log, All);

#define LOCTEXT_NAMESPACE "TrapTrigger"

// Sets default values
ATrapTriggerBase_Old::ATrapTriggerBase_Old()
   : _resetAfterTriggered(true)
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = false;

   bReplicates = true;
   NetDormancy = DORM_DormantAll;
}

// Called when the game starts or when spawned
void ATrapTriggerBase_Old::BeginPlay()
{
   Super::BeginPlay();
   
   K2_OnStateChanged(_state.State, _state.State, false);
}


void ATrapTriggerBase_Old::AuthorityTrigger()
{
   if (IsInState(ETrapTriggerState::Triggered) || IsInState(ETrapTriggerState::Disarmed)) return;
   if (!ensure(HasAuthority())) return;

   float emitterDelay = GetDelayBeforeFiringEmitters();
   if( emitterDelay <= 0)
   {
      _TriggerEmitters();
   }
   else
   {
      FTimerHandle timerHandle;
      GetWorldTimerManager().SetTimer(timerHandle, this, &ATrapTriggerBase_Old::_TriggerEmitters, emitterDelay, false);
   }

   ClientOnTriggered();

   if (!_resetAfterTriggered)
   {
      _SetState(ETrapTriggerState::Triggered);
   }
   else if (_resetDuration > 0)
   {
      _SetState(ETrapTriggerState::Resetting);
      GetWorldTimerManager().SetTimer(_resetTimerHandle, this, &ATrapTriggerBase_Old::_OnResetTimer, _resetDuration, false);
   }
   else
   {
      _SetState(GetStateAfterReset());
   }
}

void ATrapTriggerBase_Old::_TriggerEmitters()
{
   TInlineComponentArray<UActorComponent*> emitterComponents;
   for (UActorComponent* component : GetComponents())
   {
      if (component && component->Implements<UTrapEmitterInterface_Old>())
      {
         emitterComponents.Add(component);
      }
   }
   for (UActorComponent* component : emitterComponents)
   {
      ITrapEmitterInterface_Old::Execute_OnTriggered(component);
   }

   for (TScriptInterface<ITrapEmitterInterface_Old> emitter : Emitters)
   {
      if (emitter.GetObject())
      {
         ITrapEmitterInterface_Old::Execute_OnTriggered(emitter.GetObject());
      }
   }
}

void ATrapTriggerBase_Old::AuthorityDisarm()
{
   _SetState(ETrapTriggerState::Disarmed);
}

void ATrapTriggerBase_Old::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATrapTriggerBase_Old, _state);
   DOREPLIFETIME(ATrapTriggerBase_Old, _lockpickCurrentTrack);
}

void ATrapTriggerBase_Old::OnRep_State(const FTrapTriggerState& previousState)
{
   if (previousState.State == _state.State) return;

   bool isRecent = !UOSEInteractionHelpers::IsOld(this, _state.ChangedServerTime);
   K2_OnStateChanged(_state.State, previousState.State, isRecent);
   OnStateChanged.Broadcast(_state.State, previousState.State, isRecent);
   if (isRecent)
   {
      K2_OnStateChangedRecently(_state.State, previousState.State);
   }
}

void ATrapTriggerBase_Old::ClientOnTriggered_Implementation()
{
   K2_OnTriggered();
}

void ATrapTriggerBase_Old::_SetState(ETrapTriggerState newState)
{
   if (newState == _state.State) return;

   FlushNetDormancy();
   FTrapTriggerState oldState = _state;
   _state.State = newState;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   OnRep_State(oldState);
}

void ATrapTriggerBase_Old::_OnResetTimer()
{
   if (!IsInState(ETrapTriggerState::Resetting)) return;
   _SetState(GetStateAfterReset());
}

ETrapTriggerState ATrapTriggerBase_Old::GetStateAfterReset() const
{
   return ETrapTriggerState::Ready;
}

bool ATrapTriggerBase_Old::IsArmed_Implementation() const
{
   return IsArmed();
}

bool ATrapTriggerBase_Old::IsArmed() const
{
   return !IsInState(ETrapTriggerState::Ready) && !IsInState(ETrapTriggerState::Triggered);
}

bool ATrapTriggerBase_Old::IsCurrentlyArmed() const
{
   return IsInState(ETrapTriggerState::Ready) || IsInState(ETrapTriggerState::Depressed);
}

bool ATrapTriggerBase_Old::IsIndefinitelyDisarmed() const
{
   return IsInState(ETrapTriggerState::Disarmed) || IsInState(ETrapTriggerState::Triggered);
}

void ATrapTriggerBase_Old::AuthorityDisableTemporarily(float disabledDuration)
{
   if (!HasAuthority() || IsIndefinitelyDisarmed()) return;

   if (!ensure(disabledDuration > 0)) return;

   _SetState(ETrapTriggerState::Resetting);
   GetWorldTimerManager().SetTimer(_resetTimerHandle, this, &ATrapTriggerBase_Old::_OnResetTimer, disabledDuration, false);
}

// Interactable Implementation (for disarming)
bool ATrapTriggerBase_Old::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return _canDisarmMechanically && IsArmed();
}

void ATrapTriggerBase_Old::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = LOCTEXT("DisarmPrompt", "Disarm");
}

FInteractStartResult ATrapTriggerBase_Old::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   _lockConfig.StartLockpicking(interactingCharacter, this);
   return FInteractStartResult();
}

void ATrapTriggerBase_Old::Unlock()
{
   _SetState(ETrapTriggerState::Disarmed);
}

void ATrapTriggerBase_Old::OnLockpickFailed()
{
   if (HasAuthority())
   {
      AuthorityTrigger();
   }
}

void ATrapTriggerBase_Old::OnLockpickTrackCompleted(int32 trackIndex)
{
   if (trackIndex < _lockpickCurrentTrack)
   {
      UE_LOG(LogTrapTriggerBase_Old, Warning, TEXT("OnLockpickTrackCompleted() called for a track that was not active"));
      return;
   }

   FlushNetDormancy();
   _lockpickCurrentTrack = trackIndex + 1;
}

void ATrapTriggerBase_Old::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

#undef LOCTEXT_NAMESPACE

