// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/OSEInteractableToggle.h"

// ose
#include "Interactables/InteractorInterface.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Player/OSEPlayerStats.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEInteractableToggle)

DEFINE_LOG_CATEGORY(LogOSESyncedToggle);

// Sets default values
AOSESyncedToggle::AOSESyncedToggle()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;
   SetNetUpdateFrequency(10.0f);
   NetPriority = 1; //< Iris treats priority <1 as not-replicated (previously 0.5f)

   // Changing back to DORM_Initial. The original issues were the result of incorrectly flushing dormancy after changes were made (rather than before)
   NetDormancy = DORM_Initial;
}

// Called when the game starts or when spawned
void AOSESyncedToggle::BeginPlay()
{
   Super::BeginPlay();

   SyncTimelines(State);
   _OnStateChanged(State.bIsOn, false);
}

void AOSESyncedToggle::GatherCurrentMovement()
{
   // Most interactable actors never move, but many are attached.
   // So we want to skip the default replication logic which resends the attachment at spawn
   if (RootComponent && RootComponent->Mobility != EComponentMobility::Movable)
   {
      return;
   }
   Super::GatherCurrentMovement();
}

bool AOSESyncedToggle::IsInPermanentToggledState() const
{
   switch (AllowedToggleTransition)
   {
   case EOSESyncedToggleAllowedTransition::ToggleBothWays:
      return false;
   case EOSESyncedToggleAllowedTransition::OnlyToggleOff:
      return !IsOn();
   case EOSESyncedToggleAllowedTransition::OnlyToggleOn:
      return IsOn();

   default:
      checkNoEntry();
      return false;
   }
}

void AOSESyncedToggle::_OnStateChanged(bool bIsOn, bool bWasRecent)
{
   OnStateChanged(bIsOn, bWasRecent);
   OnToggleStateChanged.Broadcast(bIsOn);
}

void AOSESyncedToggle::SetOn(bool newOn)
{
   if (newOn == State.bIsOn) return;
   
   if (!CanSwitchState())
   {
      UE_LOG(LogOSESyncedToggle, Warning, TEXT("Toggle can't switch states but tried to."));
      return;
   }

   FlushNetDormancy();
   const auto OldState = State;
   State.bIsOn = newOn;
   State.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);

   // "Predict" OnRep if not server
   OnRep_State(OldState);
}

void AOSESyncedToggle::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(AOSESyncedToggle, State);
}

void AOSESyncedToggle::OnRep_State(const FOSEToggleState& PreviousState)
{
   SyncTimelines(State);

   if(PreviousState.bIsOn != State.bIsOn)
   {
      const bool wasRecent = !State.IsOld(this, 0.75f);
      _OnStateChanged(State.bIsOn, wasRecent);
      if (wasRecent)
      {
         OnRecentlyToggled(State.bIsOn);
      }
   }
}

bool AOSEInteractableToggle::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return CanSwitchState();
}

void AOSEInteractableToggle::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = State.bIsOn ? TurnOffPrompt : TurnOnPrompt;
   prompt.InteractStatusTag = InteractionStatusTag;
}

FInteractStartResult AOSEInteractableToggle::StartInteract_Implementation(ACharacter* InteractingCharacter)
{
   ToggleForInteraction(InteractingCharacter);

   FInteractStartResult result;
   result.InstantAnimationTag = IsOn() ? TurnOnAnimationTag : TurnOffAnimationTag;
   return result;
}

void AOSEInteractableToggle::ToggleForInteraction(ACharacter* interactingCharacter)
{
   bool newIsOn = !State.bIsOn;
   SetOn(newIsOn);

   if (HasAuthority())
   {
      // On the server, keep track of interaction stats
      UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(interactingCharacter, newIsOn ? StatTagOn : StatTagOff);
      OnAuthorityToggledBy(interactingCharacter, newIsOn);
   }
}

