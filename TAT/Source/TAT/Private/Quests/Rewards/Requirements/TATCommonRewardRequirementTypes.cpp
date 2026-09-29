// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Rewards/Requirements/TATCommonRewardRequirementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCommonRewardRequirementTypes)

bool FTATQuestRewardRequirement_EscapeRoute::IsMet(const FTATQuestRewardRequirementContext& context) const
{
   return ExactMatchRequired ? RequiredEscapeRoutes.HasTagExact(context.EscapeUsed) : RequiredEscapeRoutes.HasTag(context.EscapeUsed);
}

#if WITH_EDITOR
void FTATQuestRewardRequirement_EscapeRoute::IsValid(TFunctionRef<void(const FText&)> reportError) const
{
   Super::IsValid(reportError);
   if (RequiredEscapeRoutes.IsEmpty())
   {
      reportError(INVTEXT("Quest has reward with escape route requirement, but RequiredEscapeRoutes is empty"));
   }
   if (!RequiredEscapeRoutes.IsValid())
   {
      reportError(INVTEXT("Quest has reward with escape route requirement, but RequiredEscapeRoute is invalid (%s)"));
   }
}
#endif //WITH_EDITOR
