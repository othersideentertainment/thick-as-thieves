// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Services/BTService_RunUtilityBehaviors.h"

// ose
#include "AI/Utility/UtilityAIBehaviorComponent.h"

// ue4
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_RunUtilityBehaviors)
DEFINE_LOG_CATEGORY_STATIC(LogBTService_RunUtilityBehaviors, Log, All);

UBTService_RunUtilityBehaviors::UBTService_RunUtilityBehaviors(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run Utility Behaviors";
}

void UBTService_RunUtilityBehaviors::OnInstanceCreated(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceCreated(ownerComp);
   CreateBehaviorInstances( _GetUtilityAIBehaviorComponent(ownerComp));
   SortBehaviorInstances();
}

FString UBTService_RunUtilityBehaviors::GetStaticDescription() const
{
#if WITH_EDITORONLY_DATA
   return FString::Printf(TEXT("%d behavior %s containing %d behaviors")
      , _behaviorSets.Num()
      , _behaviorSets.Num() == 1 ? TEXT("set") : TEXT("sets")
      , NumBehaviors);
#else
   return Super::GetStaticDescription();
#endif // WITH_EDITORONLY_DATA
}

void UBTService_RunUtilityBehaviors::CreateBehaviorInstancesFromSet(UUtilityAIBehaviorComponent* utilityAIComponent, UUtilityBehaviorSet* behaviorSet)
{
   if (behaviorSet == nullptr)
   {
      UE_LOG(LogBTService_RunUtilityBehaviors, Error, TEXT("%s : %s : CreateBehaviorInstancesFromSet - null behavior set passed in"), *GetName(), *GetStaticDescription());
      return;
   }
   for (const FBehaviorSetItem& setItem : behaviorSet->Behaviors)
   {
      FUtilityStateEvaluatorInstance::CreateAndAdd(*this, *utilityAIComponent, setItem.Name, setItem.Weight, setItem.MomentumBonus, setItem.Evaluator, setItem.Behavior, _behaviors);
   }
}

void UBTService_RunUtilityBehaviors::CreateBehaviorInstances(UUtilityAIBehaviorComponent* utilityAIComponent)
{
   for(UUtilityBehaviorSet* behaviorSet : _behaviorSets)
   {
      CreateBehaviorInstancesFromSet(utilityAIComponent, behaviorSet);
   }
}

void UBTService_RunUtilityBehaviors::SortBehaviorInstances()
{
   // Utility weighting assumes this list is pre-sorted by weight, and we don't want to leave that up to the utility component or it'll have to sort each time the list of behaviors changes
   _behaviors.StableSort([](const FUtilityStateEvaluatorInstance& a, const FUtilityStateEvaluatorInstance& b)
   {
      return a.Weight > b.Weight;
   });
}

#if WITH_EDITOR
EDataValidationResult UBTService_RunUtilityBehaviors::IsDataValid(FDataValidationContext& context)
{
   for (int idx = 0; idx < _behaviorSets.Num(); ++idx)
   {
      UUtilityBehaviorSet* behaviorSet = _behaviorSets[idx];
      if (!behaviorSet)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no behavior set at index %d!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UBTService_RunUtilityBehaviors::PostLoad()
{
   Super::PostLoad();
   _RefreshWeights();
}

void UBTService_RunUtilityBehaviors::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshWeights();
}

void UBTService_RunUtilityBehaviors::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshWeights();
}
#endif // WITH_EDITOR

void UBTService_RunUtilityBehaviors::_RefreshWeights()
{
#if WITH_EDITOR
   WeightInfo.Reset();
   NumBehaviors = 0;

   TArray<UtilityAIWeightHelper::WeightInfo> weightInfos;
   
   for (UUtilityBehaviorSet* behaviorSet : _behaviorSets)
   {
      if (!behaviorSet)
         continue;

      for (FBehaviorSetItem& behaviorSetItem : behaviorSet->Behaviors)
      {
         if (behaviorSetItem.Evaluator)
         {
            weightInfos.Add({ behaviorSetItem.Name, behaviorSetItem.Weight });
            NumBehaviors++;
         }
      }
   }

   weightInfos.Sort();

   for (const UtilityAIWeightHelper::WeightInfo& info : weightInfos)
   {
      WeightInfo.Add(FString::Printf(TEXT("%.02f: %s"), info.Weight, *info.Name.ToString()));
   }
#endif // WITH_EDITOR
}

