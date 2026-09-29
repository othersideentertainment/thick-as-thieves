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
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "TATElectrotypeBooth.generated.h"

// Resolved payload for electrotype clue
struct FTATElectrotypeClueData
{
   FText ClueText;
   FTATSharedClueFactThunk Facts;
   FGameplayTag SourceTag;
};

UENUM()
enum class ETATActiveElectrotypeClueState : uint8
{
   NotActive,
   Pending,
   Dispensed
};

UENUM(BlueprintType)
enum class ETATElectrotypeInteractAnimationState : uint8
{
   InitialState,
   HeldInteractInProgress,
   HeldInteractCompleted
};

// replicated struct for currently-shown clue
USTRUCT()
struct FTATActiveElectrotypeClue
{
   GENERATED_BODY()

   UPROPERTY()
   FText ClueText;

   UPROPERTY()
   TWeakObjectPtr<ACharacter> Interactor;

   UPROPERTY()
   ETATActiveElectrotypeClueState ClueState = ETATActiveElectrotypeClueState::NotActive;

   UPROPERTY()
   uint8 ClueIndex = NoClue;

   static constexpr uint8 NoClue = 0xFF;

   // NOTE: Could add timestamp a la InteractableToggle if there is a need for FX to care about recency,
   //       but the duration is limited enough that that may not matter.
   // CLUE-WIP: Should have a "version" number so clients don't get confused if they miss an end?

   bool NetSerialize( FArchive& ar, class UPackageMap* map, bool& outSuccess );

   FORCEINLINE bool IsActive() const { return ClueState != ETATActiveElectrotypeClueState::NotActive; }
};

template<>
struct TStructOpsTypeTraits<FTATActiveElectrotypeClue> : public TStructOpsTypeTraitsBase2<FTATActiveElectrotypeClue>
{
   enum
   {
      WithNetSerializer = true
   };
};

// A clue spawner component for electrotype clues. Mostly delegates to actor.
// CLUE-WIP: What should the balance of what lives at the actor vs. component level here?
//           I could move everything into the component, technically, unless c++ wants to do something specifically with
//           another component. For now, erring on side of purely delegating to the actor.
UCLASS()
class TAT_API UTATElectrotypeClueComponent : public UTATClueSpawnerComponent
{
   GENERATED_BODY()

public:
   UTATElectrotypeClueComponent();

   void AddClue(const FTATElectrotypeClueData& clue);
   virtual void OnAllCluesApplied() override final;
};

// An interactable that show electrotype clues
UCLASS()
class TAT_API ATATElectrotypeBooth : public AActor, public IInteractableInterface
{
   GENERATED_BODY()
public:
   ATATElectrotypeBooth();

   /// IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;

   // pass-through from component
   void AddClue(const FTATElectrotypeClueData& clue);
   void OnAllCluesApplied();

protected:
   UFUNCTION(BlueprintImplementableEvent, Category=Electrotype, meta=(DisplayName="OnClueActive"))
   void BP_OnClueActive(bool isActive);
   UFUNCTION(BlueprintImplementableEvent, Category=Electrotype, meta=(DisplayName="HandleClueText"))
   void BP_HandleClueText(const FText& clueText);
   
   // Called when local player has available clues, or if they are out
   // (assumed to start without any until told it does)
   UFUNCTION(BlueprintImplementableEvent, Category=Electrotype, meta=(DisplayName="OnLocalClueAvailabilityChanged"))
   void BP_OnLocalClueAvailabilityChanged(bool hasAvailableClues);

   UFUNCTION(BlueprintImplementableEvent, Category=Electrotype, meta=(DisplayName="OnClueTakenOrExpired"))
   void BP_OnClueTakenOrExpired(ACharacter* clueOwner);

   UFUNCTION(BlueprintImplementableEvent, Category = Electrotype, meta = (DisplayName = "HandleInteractAnimationState"))
   void BP_HandleInteractAnimationState(ETATElectrotypeInteractAnimationState oldState, ETATElectrotypeInteractAnimationState newState);

   // Called from BP when the clue-dispense animation is completed, allowing the clue to be "taken" via a subsequent interact
   // NOTE: a dedicated server will have to call this, requiring either playing (or simulating via delay) the animations server-side
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Electrotype)
   void AuthorityOnClueFinishedDispensing();

private:
   void _SetInteractAnimationState(ETATElectrotypeInteractAnimationState newInteractAnimationState);

   void _PresentNextClue(ACharacter* interactingCharacter);

   UFUNCTION()
   void _OnRep_ActiveClue(const FTATActiveElectrotypeClue& previous);
   UFUNCTION()
   void _OnRep_ClueSources();
   UFUNCTION()
   void _OnRep_InteractState(ETATElectrotypeInteractAnimationState previous);
   void _SetActiveClue(const FTATActiveElectrotypeClue& newClue);
   void _ClearActiveClue();
   void _ExpireActiveClue();
   void _TakeActiveClue(ACharacter* interactingCharacter);
   void _InitLocalClues();
   void _OnLocalQuestChanged(TConstArrayView<FGameplayTag> questTags);

   // already using uint8 for the replicated payload
   using FClueIndices = TArray<uint8, TInlineAllocator<8>>;

   TConstArrayView<FGameplayTag> _GetQuestTagsForCharacter(const ACharacter* character) const;
   
   FClueIndices _CalculateClueIndicesForMissions(TConstArrayView<FGameplayTag> questTags) const;
   FClueIndices _CalculateClueIndicesForCharacter(const ACharacter* character) const;
   int32 _GetNextClueIndex(const ACharacter* character) const;
   int32 _GetNextClueIndex(int32 lastIndex, TConstArrayView<uint8> allowedIndices) const;
   void _SetLastClueIndex(const ACharacter* character, int32 index);
   void _AuthorityRecordSeenClue(const ACharacter* character, const FTATElectrotypeClueData& clueData) const;

   void _SetHasNextClue(bool hasNext);
   void _UpdateHasNextOrActiveClue();
   
   UPROPERTY(VisibleAnywhere)
   TObjectPtr<UTATElectrotypeClueComponent> _clueComponent;
   
   TArray<FTATElectrotypeClueData> _authorityClues;

   // replicated clue source tags, so clients can know which ones they would skip before seeing them
   // 1:1 with _authorityClues
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_ClueSources)
   TArray<FGameplayTag> _clueSources;
   
   UPROPERTY(ReplicatedUsing=_OnRep_ActiveClue)
   FTATActiveElectrotypeClue _activeClue;

   // We replicate this separately from interacting-character and clue-state, so we can produce animations for non-local clients keyed to interact start/completion/cancellation
   UPROPERTY(ReplicatedUsing=_OnRep_InteractState)
   ETATElectrotypeInteractAnimationState _interactAnimState = ETATElectrotypeInteractAnimationState::InitialState;

   // How long the clue is revealed for (including animations)
   UPROPERTY(EditDefaultsOnly, Category="Clues")
   float _clueDisplayDuration = 3.f;

   // Maximum number of clues that each player can get
   UPROPERTY(EditDefaultsOnly, Category="Clues", meta=(UIMin=1, ClampMin = 1))
   int32 _maxCluesToDisplay = 2;

   UPROPERTY(EditDefaultsOnly, Category="Interaction", meta = (UIMin = 0, ClampMin = 0, Units = "Seconds"))
   float _heldInteractDuration = 2.5f;

   UPROPERTY(EditDefaultsOnly, Category="Interaction")
   FText _generateCluePrompt;

   UPROPERTY(EditDefaultsOnly, Category="Interaction")
   FText _pendingCluePrompt;

   UPROPERTY(EditDefaultsOnly, Category="Interaction")
   FText _takeCluePrompt;

   // Tag for held interact animation
   UPROPERTY(EditDefaultsOnly, Category="Interaction", meta = (Categories = "InteractAnimation", EditCondition = "_heldInteractDuration > 0", EditConditionHides))
   FGameplayTag _interactHoldAnimationTag;

   // Tag for instant interact animation
   UPROPERTY(EditDefaultsOnly, Category="Interaction", meta = (Categories = "InteractAnimation", EditCondition = "_heldInteractDuration <= 0", EditConditionHides))
   FGameplayTag _interactPressAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (EditCondition = "_heldInteractDuration > 0", EditConditionHides))
   FOSEHeldActionCues _interactHoldActionCues;
   
   // Last clue index by character, indirectly synchronized (may revisit)
   TMap<TWeakObjectPtr<const ACharacter>, int32> _clueIndexByPlayer;
   FTimerHandle _expireClueTimer;

   struct FLocalPlayerClueState
   {
      bool HasNextClue = false;
      bool HasNextOrActiveClue = false;
   };
   FLocalPlayerClueState _localPlayerClues;

   UPROPERTY(Transient, Replicated)
   TWeakObjectPtr<ACharacter> _interactingCharacter;
};
