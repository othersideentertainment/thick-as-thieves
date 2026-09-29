// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphCondenseNode.h"

// tat
#include "Quests/Modules/TATQuestGraphUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphCondenseNode)


UTATQuestGraphCondenseNode::UTATQuestGraphCondenseNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Condense (Reroute)"));
#endif
}

FString UTATQuestGraphCondenseNode::GetNodeDebugName() const
{
#if WITH_EDITORONLY_DATA
   UTATQuestGraphCondenseNode* cdo = StaticClass()->GetDefaultObject<UTATQuestGraphCondenseNode>();
   check(cdo != nullptr);
   return FString::Printf(TEXT("CondenseNode(%s)"), ((Label != cdo->Label) ? *Label : TEXT("")));
#else
   return TEXT("CondenseNode");
#endif
}

#if WITH_EDITOR
FLinearColor UTATQuestGraphCondenseNode::GetBackgroundColor() const
{
   return TATQuestGraphUtil::kCondenseColor;
}
#endif // WITH_EDITOR
