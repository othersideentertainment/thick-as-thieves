// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATQuestGraphNode.h"
#include "TATQuestGraphClueSetNode.generated.h"

/**
 * 
 */
UCLASS()
class TAT_API UTATQuestGraphClueSetNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphClueSetNode();

   UPROPERTY(EditAnywhere, Category = "Clues", meta = (AllowedClasses="/Script/TAT.TATClueSet"))
   FSoftObjectPath ClueSet;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UOSEGenericGraphNode
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return TATQuestGraphUtil::kSelectorStyle; }
   virtual FLinearColor GetBackgroundColor() const override;
   virtual FText GetNodeTooltipText() const override;
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;

   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

// Adds a request for a clue set to be used when a specific quest actor is spawned
// (Its clues are then parameterized by that spawn info)
UCLASS()
class TAT_API UTATQuestGraphClueSetForSpawnNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphClueSetForSpawnNode();

   UPROPERTY(EditAnywhere, Category = "Clues", meta = (Categories="Loot.Quest"))
   FGameplayTag LootTag;
   
   UPROPERTY(EditAnywhere, Category = "Clues", meta = (AllowedClasses="/Script/TAT.TATClueSet"))
   FSoftObjectPath ClueSet;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UOSEGenericGraphNode
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return TATQuestGraphUtil::kSelectorStyle; }
   virtual FLinearColor GetBackgroundColor() const override;
   virtual FText GetNodeTooltipText() const override;
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;

   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
