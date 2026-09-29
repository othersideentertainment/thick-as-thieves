// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/SmartObjects/TATActionNodeComponent_PlayerTool.h"

// tat
#include "AI/Target/TATTargetingGroups.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent_PlayerTool)

FGameplayTag UTATActionNodeComponent_PlayerTool::GetUtilityAITargetingGroup() const
{
   return TAG_AI_TargetingGroup_SmartObject_PlayerTool;
}
