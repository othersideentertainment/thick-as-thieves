// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphTypes.h"

// ose
#include "OSEGenericGraph.h"

#include "TATQuestGraphNode.generated.h"

struct FTATQuestGraphMapCheckContext;

UCLASS()
class TAT_API UTATQuestGraphNode : public UOSEGenericGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphNode();

   virtual UTATQuestGraphNode* SelectNextNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const;

   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const { return true; }

   virtual bool PostExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const { return true; }

   // Gets compatibility without worrying about the next nodes in the path (but including nested nodes in the case of modules)
   virtual FTATQuestGraphCompatibility GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const;
   virtual FTATQuestGraphSceneCache::MaskType GetSceneMask(FTATQuestGraphSceneCache& cache) const;

   virtual bool IsRootNode() const { return false; }


#if WITH_EDITOR
   // from UOSEGenericGraphNode
   virtual bool GetNodeIndicatorVisible() const override;
   virtual FText GetNodeIndicatorText() const override;
   virtual FVector2D GetNodeIndicatorSize() const override { return FVector2D(60, 40); }
   virtual FLinearColor GetNodeIndicatorBackgroundColor() const override { return FLinearColor::Black; }
   virtual FText GetNodeListViewTitle() const override { return FText::FromString(GetNodeDebugName()); }

   virtual void ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const {}
   // Should only return true if a module with this Node should fail validation if it doesn't have a map to validate against
   // It is allowed to return false but still implement ValidateAgainstWorld, if validation is just nice-to-have
   // It will still be checked via map-check
   virtual bool RequireMapForValidation() const { return false; }
#endif
};
