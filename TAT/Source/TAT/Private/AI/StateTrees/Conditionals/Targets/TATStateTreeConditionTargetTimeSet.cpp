// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/Targets/TATStateTreeConditionTargetTimeSet.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionTargetTimeSet)

#define LOCTEXT_NAMESPACE "TATStateTreeConditionTargetTimeSet"

bool FTATStateTreeConditionTargetTimeSet::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.Controller);
   if (aiController == nullptr)
   {
      UE_VLOG(
         context.GetOwner(),
         LogStateTree,
         Error,
         TEXT("FTATStateTreeConditionHasTargetInGroup failed since AIController is not a TATAIController")
      );
      return false;
   }
   if (const UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent())
   {
      const float timeSet = targetingComponent->GetTimeTargetSetForTargetingGroup(instanceData.TargetingGroup);
      const float worldTimeNow = aiController->GetWorld()->GetTimeSeconds();
      const float timeDiff = worldTimeNow - timeSet;
      return UOSEMathFunctionLibrary::CompareFloats(timeDiff, instanceData.ComparisonValue, instanceData.ComparisonMethod);
   }
   return false;
}

#if WITH_EDITOR
FText FTATStateTreeConditionTargetTimeSet::GetDescription(const FGuid& id,
                                                          const FStateTreeDataView instanceDataView,
                                                          const IStateTreeBindingLookup& bindingLookup,
                                                          const EStateTreeNodeFormatting formatting) const
{
   const FInstanceDataType* instanceData = instanceDataView.GetPtr<FInstanceDataType>();
   check(instanceData);
   const FText format = LOCTEXT("TATStateTreeConditionTargetTimeSet_Description", "{Target} has been set for {Comparision} {Time}");
   return FText::FormatNamed(format,
      TEXT("Target"), FText::FromString(*instanceData->TargetingGroup.ToString()),
      TEXT("Comparision"), FText::FromString(*UOSEMathFunctionLibrary::GetComparisonMethodDescription(instanceData->ComparisonMethod)),
      TEXT("Time"), instanceData->ComparisonValue);
}
#endif
#undef LOCTEXT_NAMESPACE
