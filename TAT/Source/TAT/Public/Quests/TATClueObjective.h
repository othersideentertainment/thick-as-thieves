// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATQuestObjective.h"
#include "TATQuestObjectiveTracker.h"

#include "TATClueObjective.generated.h"

class UTATKnownCluesComponent;

// Get a specific clue fact
//
// NOTE: This does not (currently) handle location-specific clues.
//       Since clues are distributed in a single pass, it cannot know
//       where the "target" clues are before deciding which clues to
//       use. If we plan to use this objective type for contracts post-VS,
//       we will need to solve this. (Missions can use quest choices)
USTRUCT(meta = (DisplayName = "Recieve Clue"))
struct TAT_API FTATClueObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()
public:
   // Clues facts that required to be known in order to complete the objective
   //
   // For now, treated as an "AND", all listed facts must be known
   UPROPERTY(EditAnywhere, meta = (Categories="ClueFact"))
   FGameplayTagContainer RequiredClueFacts;

   // Objective text
   UPROPERTY(EditAnywhere)
   FText ObjectiveText;

   // If true, the objective fails if the player does not escape
   UPROPERTY(EditAnywhere)
   bool FailIfCaught = false;

   virtual FText GetObjectiveText(const UObject* worldContext) const override;
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual FString GetDebugDescription() const override { return FString::Printf(TEXT("Get Clue: %s"), *RequiredClueFacts.ToStringSimple()); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};


UCLASS()
class TAT_API UTATClueObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

   virtual void Initialize(const FTATObjectiveTrackerContext& context) override;
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const override;
   virtual void CheatComplete() override;
private:
   UFUNCTION()
   void _RefreshObjective(UTATKnownCluesComponent* knownClues);
   bool _CalculateObjectiveComplete() const;

   FGameplayTag _questTag;
   FGameplayTagContainer _requiredClueFacts;
   bool _failIfCaught = false;
   
   UPROPERTY(Transient)
   TObjectPtr<UTATKnownCluesComponent> _knownClues;
};
