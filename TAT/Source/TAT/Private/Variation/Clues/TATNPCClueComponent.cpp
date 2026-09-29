// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/Clues/TATNPCClueComponent.h"

// tat
#include "AI/TATAISettings.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Variation/Clues/Effects/TATClueEffectHelpers.h"
#include "Quests/TATQuestDependentActorHelpers.h"

// ose
#include "OSECommon.h"

// ue
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNPCClueComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATNPCClueComponent, Log, All);

UTATNPCClueComponent::UTATNPCClueComponent()
{
   SetIsReplicatedByDefault(true);
}

void UTATNPCClueComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATNPCClueComponent, _clue);
   DOREPLIFETIME(UTATNPCClueComponent, _isClueGivingEnabled);
}

bool UTATNPCClueComponent::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (!_HasClueToShare() || !_isClueGivingEnabled)
   {
      return false;
   }

   if (interactingCharacter && _clue.FollowUpDialogue.IsEmpty() && _previousInteractingPlayers.Contains(interactingCharacter->GetPlayerState()))
   {
      return false;
   }

   return TATQuestDependentActorHelpers::IsRelevantForCharacter(_clue.SourceTag, interactingCharacter);
}

FInteractStartResult UTATNPCClueComponent::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   check(IInteractableInterface::Execute_IsInteractable(this, interactingCharacter));

   check(interactingCharacter);

   // Should not be possible for AI to interact with clue-giving NPC, but soft-fail anyway
   APlayerState* playerState = interactingCharacter->GetPlayerState();
   if (!playerState)
   {
      UE_LOG(LogTATNPCClueComponent, Verbose, TEXT("[%s] non-player %s prompted for clue! Aborting interaction")
         , *GetOwner()->GetName()
         , *interactingCharacter->GetName());
      return FInteractStartResult();
   }

   const bool clueAlreadyShared = _HasSharedClueWithPlayer(playerState);

   UE_LOG(LogTATNPCClueComponent, Verbose, TEXT("[%s] player %s prompted for clue (%s)...")
      , *GetOwner()->GetName()
      , *interactingCharacter->GetName()
      , clueAlreadyShared ? TEXT("previously shared") : TEXT("first time"));

   OnCluePrompted(interactingCharacter, clueAlreadyShared);
   OnCluePromptedEvent.Broadcast(interactingCharacter, clueAlreadyShared);

   if (!clueAlreadyShared)
   {
      _ShareClueWithPlayer(playerState);
   }

   if (GetOwner()->HasAuthority())
   {
      // Fire RPC to notify remote clients of interaction
      _ClientOnPromptForClue(interactingCharacter, clueAlreadyShared);
      TATClueEffectHelpers::ApplyEffects(_clue.Effects, playerState);
   }

   return FInteractStartResult();
}

void UTATNPCClueComponent::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   check(IInteractableInterface::Execute_IsInteractable(this, interactingCharacter));

   // Legacy support for interaction prompts defined on actor instances.
   // Once the dummy clue-giving actors have been deleted, we can simply use '_clue.InteractionPrompt'.
   if (!InteractionPrompt.IsEmpty())
   {
      outPrompt.PressAction = InteractionPrompt;
   }
   else
   {
      const FText& prompt = !_clue.InteractionPrompt.IsEmpty() ? _clue.InteractionPrompt : UTATAISettings::Get().AIClueInteractionPrompt;
      outPrompt.PressAction = prompt;
   }
}

void UTATNPCClueComponent::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(GetOwner(), showHighlight);
}

void UTATNPCClueComponent::AuthorityInitFromSpawner(const UTATNPCClueSpawnerComponent* clueSpawner)
{
   check(clueSpawner != nullptr);
   check(clueSpawner->AuthorityHasClueToShare());

   AuthoritySetClueGivingEnabled(UTATAISettings::Get().InitialClueSpawnerAIClueGivingEnabled);
   _AuthoritySetClue(clueSpawner->AuthorityGetClue());
}

void UTATNPCClueComponent::AuthoritySetClueGivingEnabled(bool enabled)
{
   if (_isClueGivingEnabled != enabled)
   {
      check(GetOwner()->HasAuthority());
      GetOwner()->FlushNetDormancy();
      _isClueGivingEnabled = enabled;
   }
}

bool UTATNPCClueComponent::HasClueToShareWithPlayer(const APlayerState* playerState) const
{
   return _clue.IsValid() && !_HasSharedClueWithPlayer(playerState);
}

void UTATNPCClueComponent::_AuthoritySetClue(const FTATNPCDialogueClueData& newClue)
{
   if (!newClue.IsValid())
   {
      UE_LOG(LogTATNPCClueComponent, Error, TEXT("[%s] | _AuthoritySetClue() called with invalid clue!"), *GetOwner()->GetName());
      return;
   }

   // Should only be called once
   check(!_clue.IsValid());

   check(GetOwner()->HasAuthority());
   GetOwner()->FlushNetDormancy();
   _clue = newClue;

   // UTATNPCClueComponent instances should only exist on AI character classes. Fail silently in
   // the chance this component is added and this method called before possession can occur. The
   // notification is only necessary if the AI's state tree has already begun running.
   if (AAIController* ownerController = UOSECommon::GetController<AAIController>(GetOwner()))
   {
      // All AI should be running state tree brain logic.
      UStateTreeComponent* stateTreeComponent = CastChecked<UStateTreeComponent>(ownerController->GetBrainComponent());
      
      FStateTreeEvent event;
      event.Tag = TAG_StateTreeEvent_ClueChange;

      stateTreeComponent->SendStateTreeEvent(event);
   }

   if (!IsNetMode(NM_DedicatedServer))
   {
      _TryNotifyClueSet();
   }
}

bool UTATNPCClueComponent::_HasSharedClueWithPlayer(const APlayerState* playerState) const
{
   return _previousInteractingPlayers.Contains(playerState);
}

void UTATNPCClueComponent::_ShareClueWithPlayer(const APlayerState* playerState)
{
   check(playerState);
   check(!_previousInteractingPlayers.Contains(playerState));
   _previousInteractingPlayers.Add(playerState);

   if (GetOwner()->HasAuthority())
   {
      UTATKnownCluesComponent::RecordKnownClueFacts(playerState, _clue.Facts.Get());
   }
}

void UTATNPCClueComponent::_ClientOnPromptForClue_Implementation(const ACharacter* interactingCharacter, bool cluePreviouslyShared)
{
   // Ignore callbacks produced for local player (or no-longer-existing character)
   if (!interactingCharacter || interactingCharacter->IsLocallyControlled())
   {
      UE_CLOG(interactingCharacter == nullptr, LogTATNPCClueComponent, Log
         , TEXT("[%s] | _ClientOnPromptForClue() called with invalid interactingCharacter! Dropping call to OnCluePrompted_RemoteCharacter...")
         , *GetOwner()->GetName());
      return;
   }

   // Notify client of remote character clue interaction
   OnCluePrompted_RemoteCharacter(interactingCharacter, cluePreviouslyShared);
   OnCluePrompted_RemoteCharacterEvent.Broadcast(interactingCharacter, cluePreviouslyShared);
}

void UTATNPCClueComponent::_OnRep_Clue(const FTATNPCDialogueClueData& oldClue)
{
   if (!oldClue.IsValid() && _clue.IsValid())
   {
      _TryNotifyClueSet();
   }
}

void UTATNPCClueComponent::_TryNotifyClueSet()
{
   TATQuestDependentActorHelpers::OnceRelevantToLocalPlayer(_clue.SourceTag, this, [weakThis = MakeWeakObjectPtr(this)]
   {
      if (UTATNPCClueComponent* clueComponent = weakThis.Get())
      {
         clueComponent->OnClueSet();
         clueComponent->OnClueSetEvent.Broadcast();
      }
   });
}
