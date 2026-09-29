// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/SmartObjects/TATActionNodeComponent_MajorLoot.h"

// tat
#include "AI/Target/TATTargetingGroups.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent_MajorLoot)

FGameplayTag UTATActionNodeComponent_MajorLoot::GetUtilityAITargetingGroup() const
{
   return TAG_AI_TargetingGroup_SmartObject_MajorLoot;
}
