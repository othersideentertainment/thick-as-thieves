// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Modules/TATQuestGraphClueSetNode.h"

#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphClueSetNode)

namespace ClueSetNodeHelpers
{
   TStringBuilder<32> GetNameForRef(const FSoftObjectPath& clueSet)
   {
      return WriteToString<32>(clueSet.GetAssetFName());
   }
}

UTATQuestGraphClueSetNode::UTATQuestGraphClueSetNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = INVTEXT("Add Clue Set");
#endif
}

bool UTATQuestGraphClueSetNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if(!ClueSet.IsNull())
   {
      ctx.ClueSets.Add(ClueSet);
      TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("Added ClueSet %s"), *ClueSetNodeHelpers::GetNameForRef(ClueSet));
   }
   else
   {
      TAT_QUESTGRAPH_LOG_ERROR(ctx, this, TEXT("No ClueSet in node"));
   }

   return true;
}

FString UTATQuestGraphClueSetNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("ClueSet(%s)"), *ClueSetNodeHelpers::GetNameForRef(ClueSet));
}

#if WITH_EDITOR
FLinearColor UTATQuestGraphClueSetNode::GetBackgroundColor() const
{
   return ClueSet.IsNull() ? FLinearColor::Red : TATQuestGraphUtil::kSelectorColor;
}

FText UTATQuestGraphClueSetNode::GetNodeTooltipText() const
{
   return INVTEXT("Adds Clue Set");
}

FText UTATQuestGraphClueSetNode::GetNodeDisplayTitle() const
{
   return INVTEXT("Add Clue Set");
}

FText UTATQuestGraphClueSetNode::GetNodeDisplaySubtitle() const
{
   return FText::FromStringView(ClueSetNodeHelpers::GetNameForRef(ClueSet).ToView());
}

EDataValidationResult UTATQuestGraphClueSetNode::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if(ClueSet.IsNull())
   {
      context.AddWarning(INVTEXT("ClueSet in 'Add Clue Set' Node is null"));
      result = EDataValidationResult::Invalid;
   }
   
   return result;
}
#endif

UTATQuestGraphClueSetForSpawnNode::UTATQuestGraphClueSetForSpawnNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = INVTEXT("Add Clue Set for Spawn");
#endif
}

bool UTATQuestGraphClueSetForSpawnNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if(!ClueSet.IsNull())
   {
      ctx.SpawnSpecificClueSets.Add({.ClueSet = ClueSet, .ActorTag = LootTag});
      TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("Added ClueSet %s for %s"),
         *ClueSetNodeHelpers::GetNameForRef(ClueSet),
         *WriteToString<64>(LootTag.GetTagName()));
   }
   else
   {
      TAT_QUESTGRAPH_LOG_ERROR(ctx, this, TEXT("No ClueSet in node"));
   }

   return true;
}

FString UTATQuestGraphClueSetForSpawnNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("ClueSetForSpawn(%s)"), *ClueSetNodeHelpers::GetNameForRef(ClueSet));
}

#if WITH_EDITOR
FLinearColor UTATQuestGraphClueSetForSpawnNode::GetBackgroundColor() const
{
   return ClueSet.IsNull() ? FLinearColor::Red : TATQuestGraphUtil::kSelectorColor;
}

FText UTATQuestGraphClueSetForSpawnNode::GetNodeTooltipText() const
{
   return INVTEXT("Adds Clue Set for a specific spawn");
}

FText UTATQuestGraphClueSetForSpawnNode::GetNodeDisplayTitle() const
{
   return INVTEXT("Add Clue Set for Spawn");
}

FText UTATQuestGraphClueSetForSpawnNode::GetNodeDisplaySubtitle() const
{
   return FText::FormatOrdered(INVTEXT("{0}\n{1}"),
      FText::FromStringView(ClueSetNodeHelpers::GetNameForRef(ClueSet).ToView()),
      FText::FromName(LootTag.GetTagName()));
}

EDataValidationResult UTATQuestGraphClueSetForSpawnNode::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if(ClueSet.IsNull())
   {
      context.AddWarning(INVTEXT("ClueSet in 'Add Clue Set for Spawn' Node is null"));
      result = EDataValidationResult::Invalid;
   }

   if(!LootTag.IsValid())
   {
      context.AddWarning(INVTEXT("LootTag in 'Add Clue Set for Spawn' Node is none"));
      result = EDataValidationResult::Invalid;
   }
   
   return result;
}
#endif
