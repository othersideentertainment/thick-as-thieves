// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueSpawner.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATTalkingDoor.generated.h"

// ustruct for replication
USTRUCT()
struct FTATTalkingDoorClueData
{
   GENERATED_BODY()
   
   UPROPERTY()
   TSoftObjectPtr<UAkAudioEvent> AudioEvent;

   UPROPERTY()
   FGameplayTag SourceTag;

   FTATSharedClueFactThunk Facts;

   bool IsValid() const { return !AudioEvent.IsNull(); }
};

// A clue spawner component for talking door clues. Mostly delegates to actor.
UCLASS()
class TAT_API UTATTalkingDoorClueComponent : public UTATClueSpawnerComponent
{
   GENERATED_BODY()

public:
   UTATTalkingDoorClueComponent();

   void SetClue(const FTATTalkingDoorClueData& clue);
};

// Whether there is a clue that the local player has not seen
UENUM(BlueprintType)
enum class ETATTalkingDoorLocalState : uint8
{
   NoClue,
   UnseenClue,
   SeenClue,
};

// A talking door interactable that gives clues from an informant
//
// CLUE-WIP: This currently replicates the clue payload upfront for expediency during an initial TOD implementation.
//   This has a couple drawbacks
//   1. Higher upfront bandwidth cost. Maybe okay if not that many of these
//   2. Potential leakage of the clue information to clients.
UCLASS()
class TAT_API ATATTalkingDoor : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   ATATTalkingDoor();

   void AuthoritySetClue(const FTATTalkingDoorClueData& clue);

protected:
   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnLocalClueStateChanged(ETATTalkingDoorLocalState state);
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnShowingClueChanged(bool showingClue);
   
   UFUNCTION(BlueprintPure, meta = (BlueprintProtected))
   TSoftObjectPtr<UAkAudioEvent> GetClueAudioEvent() const;

private:
   void _SetShowingClue(bool newShowingClue);
   void _ExpireActiveClue();
   void _SetLocalClueState(ETATTalkingDoorLocalState newState);
   UFUNCTION()
   void _OnRep_ClueData(const FTATTalkingDoorClueData& previous);
   UFUNCTION()
   void _OnRep_ShowingClue(const bool& wasShowing);

   UPROPERTY(VisibleAnywhere)
   TObjectPtr<UTATTalkingDoorClueComponent> _clueComponent;
   
   // TODO/CLUE-WIP: don't replicate clue info right away
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_ClueData)
   FTATTalkingDoorClueData _clueData;

   // How long the clue is revealed for (including animations)
   UPROPERTY(EditDefaultsOnly, Category="Clues")
   float _clueDisplayDuration = 5.f;

   // Interact prompt for getting a clue
   UPROPERTY(EditDefaultsOnly, Category="Interaction")
   FText _getCluePrompt;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_ShowingClue)
   bool _isShowingClue = false;

   // Tag for instant interact animation
   UPROPERTY(EditDefaultsOnly, Category="Interaction", meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag _instantAnimationTag;

   // Fact tag to inject when a player interacts with it
   // Does not add any payload, but can represent the fact
   // of interacting with this specific door, for suppression purposes.
   UPROPERTY(EditInstanceOnly, Category="Clues", meta = (Categories="ClueFact"))
   FGameplayTag _additionalClueFactTag;
   
   TArray<TWeakObjectPtr<const ACharacter>> _previousPlayers;
   FTimerHandle _expireClueTimer;
   ETATTalkingDoorLocalState _localClueState;
};
