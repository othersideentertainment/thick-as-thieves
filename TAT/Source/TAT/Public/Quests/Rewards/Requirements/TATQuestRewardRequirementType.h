// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"

#include "TATQuestRewardRequirementType.generated.h"

/// Struct to be stuffed with anything that can influence a FTATQuestRewardRequirementType subclass
/// If you add a new requirement to TATCommonRewardRequriementTypes, make sure any match-specific data it relies on goes here!
/// NOTE: May want to revisit this if the pool of considered-things grows out of control
struct FTATQuestRewardRequirementContext
{
   FGameplayTag EscapeUsed;
};

/// Struct intended to be subclass ed with specific requirement data
USTRUCT(BlueprintType)
struct TAT_API FTATQuestRewardRequirementType
{
   GENERATED_BODY()

   virtual ~FTATQuestRewardRequirementType() {}

public:
   virtual bool IsMet(const FTATQuestRewardRequirementContext& context) const 
   {
      unimplemented();
      return true;
   }

#if WITH_EDITOR
   // TODO: call from FTATQuestInfo::Validate() when integrated into reward data
   virtual void  IsValid(TFunctionRef<void(const FText&)> reportError) const {}
#endif // WITH_EDITOR
};

