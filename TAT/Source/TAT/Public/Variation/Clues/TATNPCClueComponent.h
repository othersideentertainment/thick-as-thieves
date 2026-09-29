// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATNPCDialogueClue.h"
#include "Variation/Clues/TATNPCClueSpawnerComponent.h"

// ose
#include "Interactables/InteractableInterface.h"

#include "TATNPCClueComponent.generated.h"

class ACharacter;
class APlayerState;

/// Component which handles player-interaction for NPCs which hold clues.
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATNPCClueComponent : public UActorComponent, public IInteractableInterface
{
   GENERATED_BODY()

public:
   UTATNPCClueComponent();

   // from AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

   UFUNCTION(BlueprintCallable)
   void AuthorityInitFromSpawner(const UTATNPCClueSpawnerComponent* clueSpawner);

   UFUNCTION(BlueprintCallable)
   void AuthoritySetClueGivingEnabled(bool enabled);

   bool HasClueToShareWithPlayer(const APlayerState* playerState) const;

   UFUNCTION(BlueprintCallable)
   FORCEINLINE bool IsClueGivingEnabled() const { return _isClueGivingEnabled; }

   UFUNCTION(BlueprintPure)
   const FText& GetClueDialogueText() const { return _clue.ClueDialogue; }

   UFUNCTION(BlueprintPure)
   const FText& GetFollowUpDialogueText() const { return _clue.FollowUpDialogue; }

   // Prompt displayed when interacting with an NPC to receive this clue.
   UPROPERTY(EditAnywhere, meta = (DeprecatedProperty, DeprecationMessage = "Interaction prompts should be set in TATAISettings or overridden on clues."))
   FText InteractionPrompt;

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void OnClueSet();

   UFUNCTION(BlueprintImplementableEvent)
   void OnCluePrompted(const ACharacter* interactingCharacter, bool cluePreviouslyShared);

   UFUNCTION(BlueprintImplementableEvent)
   void OnCluePrompted_RemoteCharacter(const ACharacter* interactingCharacter, bool cluePreviouslyShared);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClueEvent);
   UPROPERTY(BlueprintAssignable)
   FOnClueEvent OnClueSetEvent;

   // Fired on interacting client/server when a player prompts the NPC for a clue
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueStart, const ACharacter*, interactingCharacter, bool, cluePreviouslyShared);
   UPROPERTY(BlueprintAssignable)
   FOnDialogueStart OnCluePromptedEvent;

   // Fired on client when a remote player prompts the NPC for a clue. Use for one-shot transient feedback (eg. generic animation / whispering VO)
   // NOTE: Not guaranteed to fire - called via unreliable RPC
   UPROPERTY(BlueprintAssignable)
   FOnDialogueStart OnCluePrompted_RemoteCharacterEvent;

private:
   FORCEINLINE bool _HasClueToShare() const { return _clue.IsValid(); }

   void _AuthoritySetClue(const FTATNPCDialogueClueData& newClue);

   bool _HasSharedClueWithPlayer(const APlayerState* playerState) const;

   void _ShareClueWithPlayer(const APlayerState* playerState);

   UFUNCTION(NetMulticast, Unreliable)
   void _ClientOnPromptForClue(const ACharacter* interactingCharacter, bool cluePreviouslyShared);

   UFUNCTION()
   void _OnRep_Clue(const FTATNPCDialogueClueData& oldClue);

   void _TryNotifyClueSet();

   // NOTE: we should revisit the clue replication to mitigate data leakage and bandwidth usage.
   // Players can examine packets to extract all clue data on clue "spawn", and text is expensive to replicate.
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_Clue)
   FTATNPCDialogueClueData _clue;

   // Players with whom the clue has been shared
   // NOTE: this will get out of sync between client/server as multiple players interact. 
   // Clients only need to know if they have interacted, so replicating the collection would serve no purpose.
   // HOWEVER: being unreplicated, if attached to a dynamically-spawned actor, exiting/re-entering relevancy range
   // will result in the actor being destroyed/recreated, with this local data being lost for clients.
   UPROPERTY(Transient)
   TSet<TWeakObjectPtr<const APlayerState>> _previousInteractingPlayers;

   UPROPERTY(Replicated, Transient)
   bool _isClueGivingEnabled = false;

};
