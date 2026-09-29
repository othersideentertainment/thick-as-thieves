// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Modules/TATQuestGraphLockCombinationNode.h"

// tat
#include "Lockpicking/TATCombinationHelpers.h"
#include "Quests/Modules/TATQuestGraphMapCheckContext.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphLockCombinationNode)

UTATQuestGraphLockCombinationNode::UTATQuestGraphLockCombinationNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Lock Combination"));
#endif
}

bool UTATQuestGraphLockCombinationNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   for(const TPair<FString, FTATLockCombinationNameRef>& pair : LockCombinations)
   {
      ctx.ClueFormatParams.Add(pair.Key, CombinationHelpers::FormatCombinationFromName(pair.Value.Name, params.MapSeed));
   }
   return true;
}

FString UTATQuestGraphLockCombinationNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("LockCombination(%s)"), *_GetDetailsString());
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphLockCombinationNode::IsDataValid(FDataValidationContext& context) const
{
   return Super::IsDataValid(context);
}

FText UTATQuestGraphLockCombinationNode::GetNodeDisplayTitle() const
{
   return INVTEXT("Lock Combination");
}

FText UTATQuestGraphLockCombinationNode::GetNodeDisplaySubtitle() const
{
   return FText::FromString(_GetDetailsString());
}

void UTATQuestGraphLockCombinationNode::ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const
{
   Super::ValidateAgainstWorld(context);

   for (const TPair<FString, FTATLockCombinationNameRef>& entry : LockCombinations)
   {
      if (!context.GetLockCombinationNames().Contains(entry.Value.Name))
      {
         context.Report(this, FText::Format(INVTEXT("Lock combination name '{0}' not found in level"),
            FText::FromName(entry.Value.Name)));
      }
   }
}
#endif


FString UTATQuestGraphLockCombinationNode::_GetDetailsString() const
{
   FString result;
   for(const TPair<FString, FTATLockCombinationNameRef>& pair : LockCombinations)
   {
      if(result.Len() > 0)
      {
         result.Append(TEXT(", "));
      }
      result.Appendf(TEXT("{%s} = %s"), *pair.Key, *pair.Value.Name.ToString());
   }
   return result;
}
