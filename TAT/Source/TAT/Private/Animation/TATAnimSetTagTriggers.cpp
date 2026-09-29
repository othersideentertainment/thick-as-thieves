// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/TATAnimSetTagTriggers.h"

// tat
#include "Animation/TATAnimSetOverride.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimSetTagTriggers)

void UTATAnimSetTagTriggerSet::ApplyTagCountChange(const FGameplayTag tag, int32 newTagCount, FTATAnimSetOverrides& overrides) const
{
   const FTATAnimSetTagTrigger* trigger = AnimSetTriggers.FindByKey(tag);
   if(trigger == nullptr)
   {
      return;
   }

   const FTATAnimSetRequest request = {
      .AnimSetTag = trigger->AnimSetTag,
      .Source = this,
      .Priority = trigger->Priority
   };
   if(newTagCount > 0)
   {
      overrides.AddRequest(request);
   }
   else
   {
      overrides.RemoveRequest(request);
   }
}

#if WITH_EDITOR
EDataValidationResult UTATAnimSetTagTriggerSet::IsDataValid(FDataValidationContext& context) const
{
   return Super::IsDataValid(context);
   // TODO
}
#endif
