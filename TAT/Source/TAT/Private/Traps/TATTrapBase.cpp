// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/TATTrapBase.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"
#include "Developer/TATProjectSettings.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"
#include "Traps/TATTrapDetectorComponent.h"
#include "Environment/TATInhibitorSubsystem.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTrapBase)

#define LOCTEXT_NAMESPACE "TrapBase"

// Sets default values
ATATTrapBase::ATATTrapBase()
{
   PrimaryActorTick.bCanEverTick = false;

   bReplicates = true;
   NetDormancy = DORM_Initial;
   
   _trapDetector = CreateDefaultSubobject<UTATTrapDetectorComponent>("TrapDetector_");
   _electricalDeviceComponent = CreateOptionalDefaultSubobject<UTATElectricalDeviceComponent>("ElectricalDevice");
   if (_electricalDeviceComponent)
   {
      _electricalDeviceComponent->SetRequiresPowerSource(false);
   }
}

// Called when the game starts or when spawned
void ATATTrapBase::BeginPlay()
{
   Super::BeginPlay();
   
   BP_OnStateChanged(_state.State, _state.State, false);
}

void ATATTrapBase::AuthorityTrigger(AActor* optionalTarget)
{
   if (!IsInState(ETATTrapState::Ready)) return;
   if (!ensure(HasAuthority())) return;
   if (!_IsTriggeringAllowed()) return;

   _OnAuthorityTriggeredBy(optionalTarget);

   if (_triggeringDuration == 0)
   {
      _AuthoritySetTriggerComplete();
   }
   else
   {
      _SetState(ETATTrapState::Triggering);
      GetWorldTimerManager().SetTimer(_nextStateTimerHandle, this, &ATATTrapBase::_OnTriggeringTimerComplete, _triggeringDuration, false);
   }
}

void ATATTrapBase::AuthorityDisarm()
{
   check(HasAuthority());
   _SetState(ETATTrapState::Disarmed);
}

void ATATTrapBase::AuthorityRearm()
{
   check(HasAuthority());
   if (IsInState(ETATTrapState::Disarmed) || IsInState(ETATTrapState::TriggerComplete))
   {
      _SetState(ETATTrapState::Ready);
   }
}

void ATATTrapBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATTrapBase, _state);
}

void ATATTrapBase::_OnRep_State(const FTATTrapState& previousState)
{
   _OnStateChanged(previousState);
}

void ATATTrapBase::_OnStateChanged(const FTATTrapState& previousState)
{
   if (previousState.State == _state.State) return;

   const bool isRecent = !UOSEInteractionHelpers::IsOld(this, _state.ChangedServerTime);
   _HandleStateChanged(previousState);
   BP_OnStateChanged(_state.State, previousState.State, isRecent);
   if (isRecent)
   {
      BP_OnStateChangedRecently(_state.State, previousState.State);
   }
}

bool ATATTrapBase::_IsTriggeringAllowed() const
{
   // Don't allow triggering the trap when inhibited
   if (IsInhibitable)
   {
      UTATInhibitorSubsystem* inhibitorSubsystem = GetWorld()->GetSubsystem<UTATInhibitorSubsystem>();
      if (inhibitorSubsystem != nullptr && inhibitorSubsystem->IsActorCurrentlyInhibited(this))
      {
         return false;
      }
   }

   if (_requirePowerToTrigger && _electricalDeviceComponent && !_electricalDeviceComponent->IsPowered())
   {
      return false;
   }

   return true;
}

void ATATTrapBase::_OnAuthorityTriggeredBy(AActor* optionalTarget)
{
   BP_OnAuthorityTriggeredBy(optionalTarget);

   if (_triggerHearingStim.IsValid())
   {
      UTATAISense_Hearing::ReportNoiseEvent(this, _triggerHearingStim, ActorToWorld().TransformPosition(_hearingStimOffset), this);
   }

   _trapDetector->TriggerTrapActions(optionalTarget);
}

void ATATTrapBase::_SetState(ETATTrapState newState)
{
   check(HasAuthority());
   if (newState == _state.State) return;

   FlushNetDormancy();
   FTATTrapState oldState = _state;
   _state.State = newState;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_State(oldState);
}

void ATATTrapBase::_AuthoritySetTriggerComplete()
{
   BP_OnAuthorityTriggerComplete();

   if (_resetDuration == 0)
   {
      _SetState(ETATTrapState::TriggerComplete);
   }
   else
   {
      _SetState(ETATTrapState::Resetting);
      GetWorldTimerManager().SetTimer(_nextStateTimerHandle, this, &ATATTrapBase::_OnResettingTimerComplete, _resetDuration, false);
   }
}

void ATATTrapBase::_OnTriggeringTimerComplete()
{
   check(HasAuthority());
   if (IsInState(ETATTrapState::Triggering))
   {
      _AuthoritySetTriggerComplete();
   }
}

void ATATTrapBase::_OnResettingTimerComplete()
{
   check(HasAuthority());
   if (IsInState(ETATTrapState::Resetting))
   {
      _SetState(ETATTrapState::Ready);
   }
}

bool ATATTrapBase::IsArmed() const
{
   return IsInState(ETATTrapState::Ready);
}

// Interactable Implementation (for disarming)
bool ATATTrapBase::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // For now you can't disarm traps, in the future we want to be able to disarm IF the character is holding a certain tool.
   // Just returning false for now.
   // 
   // NOTE: Handle power state if that comes back
   return false;
}

void ATATTrapBase::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (IsArmed())
   {
      prompt.HoldAction = LOCTEXT("DisarmPrompt", "Disarm");
   }
   else
   {
      prompt.HoldAction = LOCTEXT("RearmPrompt", "Re-arm");
   }
}

FInteractStartResult ATATTrapBase::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   FInteractStartResult result; 

   if (IsArmed())
   {
      result = FInteractStartResult::Wait(settings.TrapDisarmHoldDuration);
      result.HoldAnimationTag = settings.TrapDisarmInteractAnimation;
      result.HoldActionCues = settings.TrapDisarmHeldActionCues;
   }
   else
   {
      result = FInteractStartResult::Wait(settings.TrapRearmHoldDuration);
      result.HoldAnimationTag = settings.TrapRearmInteractAnimation;
      result.HoldActionCues = settings.TrapRearmHeldActionCues;
   }

   return result;
}

bool ATATTrapBase::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      if (HasAuthority())
      {
         if (IsArmed())
         {
            _SetState(ETATTrapState::Disarmed);
         }
         else if(IsInState(ETATTrapState::Disarmed) || IsInState(ETATTrapState::TriggerComplete))
         {
            _SetState(ETATTrapState::Ready);
         }
      }
   }

   return true;
}

void ATATTrapBase::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

FTATInhibitorPlacementInfo ATATTrapBase::GetInhibitorPlacementInfo_Implementation() const
{
   // Child classes will almost certainly want to override this, but we can provide a simple baseline implementation here
   return FTATInhibitorPlacementInfo::Make(GetActorLocation(), GetActorRotation());
}

#undef LOCTEXT_NAMESPACE

