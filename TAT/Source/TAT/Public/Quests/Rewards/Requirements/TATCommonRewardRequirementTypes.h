// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Quests/Rewards/Requirements/TATQuestRewardRequirementType.h"

// ue
#include "GameplayTagContainer.h"

#include "TATCommonRewardRequirementTypes.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Require Escape Route"))
struct TAT_API FTATQuestRewardRequirement_EscapeRoute : public FTATQuestRewardRequirementType
{
   GENERATED_BODY()

public:
   // from FTATQuestRewardRequirementType
   virtual bool IsMet(const FTATQuestRewardRequirementContext& context) const override;
#if WITH_EDITOR
   virtual void IsValid(TFunctionRef<void(const FText&)> reportError) const override;
#endif //WITH_EDITOR

private:
   // Player must escape through one of these. 
   // Non-exact matches are allowed (eg. EscapeRoute.North in container, and player uses EscapeRoute.North.Sewer)
   UPROPERTY(EditAnywhere, meta = (Categories = "EscapeRoute"))
   FGameplayTagContainer RequiredEscapeRoutes;

   UPROPERTY(EditAnywhere)
   bool ExactMatchRequired = false;
};
