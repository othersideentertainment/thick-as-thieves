// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include "TATQuestGraphSubgraphNode.generated.h"

class UTATQuestGraphModule;


/// Execute a quest subgraph
UCLASS()
class TAT_API UTATQuestGraphSubgraphNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphSubgraphNode();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests")
   TObjectPtr<UTATQuestGraphModule> Module;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;
   virtual bool PostExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;
   virtual FTATQuestGraphCompatibility GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const override;
   virtual FTATQuestGraphSceneCache::MaskType GetSceneMask(FTATQuestGraphSceneCache& cache) const override;


   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // From UOSEGenericGraphNode
   virtual TSharedPtr<SWidget> ConstructNodeBodyWidget() override;
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return EOSEGenericGraphNodeStyle::Flat; }
   virtual FLinearColor GetBackgroundColor() const override;
   virtual FText GetNodeDisplayTitle() const override;
   virtual void OnNodeDoubleClicked() override;

   // from UTATQuestGraphNode
   virtual void ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const override;
#endif
};
