// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphObjectiveNode.h"

// tat
#include "Quests/TATQuestObjective.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphObjectiveNode)


UTATQuestGraphObjectiveNode::UTATQuestGraphObjectiveNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Set Objective"));
#endif
}

bool UTATQuestGraphObjectiveNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::ExecuteNode(params, ctx))
   {
      return false;
   }
   ctx.ObjectiveNodeHandle = FOSEGenericGraphNodeHandle(GetGraph(), NodeId);
   return true;
}

const FTATQuestObjectiveInfo& UTATQuestGraphObjectiveNode::GetObjectiveChecked() const
{
   const FTATQuestObjectiveInfo* objectiveInfo = Objective.GetPtr<FTATQuestObjectiveInfo>();
   check(objectiveInfo != nullptr);
   return *objectiveInfo;
}

FString UTATQuestGraphObjectiveNode::_GetObjectiveDebugDescription() const
{
   if (const FTATQuestObjectiveInfo* objective = Objective.GetPtr<FTATQuestObjectiveInfo>())
   {
      return objective->GetDebugDescription();
   }
   return FString();
}
