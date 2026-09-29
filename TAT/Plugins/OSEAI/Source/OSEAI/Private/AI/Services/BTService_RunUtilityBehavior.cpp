// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Services/BTService_RunUtilityBehavior.h"

// ose
#include "AI/Utility/UtilityAIBehavior.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_RunUtilityBehavior)

// ue4

UBTService_RunUtilityBehavior::UBTService_RunUtilityBehavior(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run Utility Behavior";
}

void UBTService_RunUtilityBehavior::OnInstanceCreated(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceCreated(ownerComp);

   if (UUtilityAIBehaviorComponent* utilityAIComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      _emptyEvalulator = NewObject<UUtilityStateEvaluator>(this);
      static const float kDefaultWeight = 1.0f; // weight doesn't matter if there's just one thing to do
      static const float kDefaultMomenumBonus = 0.2f; // bonus doesn't matter if there's just one thing to do
      FUtilityStateEvaluatorInstance::CreateAndAdd(*this, *utilityAIComponent, Behavior.Name, kDefaultWeight, kDefaultMomenumBonus, _emptyEvalulator, Behavior.Behavior, _behaviors);
   }
}

FString UBTService_RunUtilityBehavior::GetStaticDescription() const
{
   return FString::Printf(TEXT("Behavior: %s"), *Behavior.Name.ToString());
}

