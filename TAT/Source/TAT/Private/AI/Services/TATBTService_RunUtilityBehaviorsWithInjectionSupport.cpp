// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Services/TATBTService_RunUtilityBehaviorsWithInjectionSupport.h"

#include "AI/Utility/TATUtilityAIBehaviorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBTService_RunUtilityBehaviorsWithInjectionSupport)

UTATBTService_RunUtilityBehaviorsWithInjectionSupport::UTATBTService_RunUtilityBehaviorsWithInjectionSupport(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run Utility Behaviors with Injection Support";
   
}

void UTATBTService_RunUtilityBehaviorsWithInjectionSupport::OnInstanceCreated(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceCreated(ownerComp);
   // To avoid creating the instances twice (and clearing the first set of created instances, we override the base functionality
   // to stop them creating instances until the "IsBehaviorCreationEnabled" = true.
   // We could have alternatively blocked calls to the Super::OnInstanceCreated, however this may lead to issues if parent functions
   // change their behaviors and they are unaware of this class blocking the function call.
   IsBehaviorCreationEnabled = true;
   // Calling the OnBehaviorInjectionChanges will clear the behaviors and re-create them. 
   if (UTATUtilityAIBehaviorComponent* utilityAIComponent = Cast<UTATUtilityAIBehaviorComponent>(_GetUtilityAIBehaviorComponent(ownerComp)))
   {
      utilityAIComponent->CallAndRegisterInjectionBehaviorChanged(BehaviorInjectionTag,
         UTATUtilityAIBehaviorComponent::FOnInjectedBehaviorsChanged::FDelegate::CreateUObject(this, &ThisClass::OnBehaviorInjectionChanges)
         );
   }
}

void UTATBTService_RunUtilityBehaviorsWithInjectionSupport::SetBehaviors(UUtilityAIComponent& utilityAIComponent)
{
   Super::SetBehaviors(utilityAIComponent);
   StatesHaveChanged = false;
}

bool UTATBTService_RunUtilityBehaviorsWithInjectionSupport::ShouldSetBehaviors(
   const UUtilityAIComponent& utilityAIComponent) const
{
   return Super::ShouldSetBehaviors(utilityAIComponent) || StatesHaveChanged;
}

void UTATBTService_RunUtilityBehaviorsWithInjectionSupport::OnBehaviorInjectionChanges(UTATUtilityAIBehaviorComponent* behaviorComponent)
{
   _behaviors.Empty();
   CreateBehaviorInstances(behaviorComponent);
   TArray<UUtilityBehaviorSet*> utilityBehaviorSets;
   behaviorComponent->GetInjectedBehaviors(BehaviorInjectionTag, utilityBehaviorSets);
   for (UUtilityBehaviorSet* injectedBehaviorSet : utilityBehaviorSets)
   {
      CreateBehaviorInstancesFromSet(behaviorComponent, injectedBehaviorSet);
   }
   SortBehaviorInstances();
   StatesHaveChanged = true;
}

void UTATBTService_RunUtilityBehaviorsWithInjectionSupport::CreateBehaviorInstances(UUtilityAIBehaviorComponent* utilityAIBehaviorComponent)
{
   if(IsBehaviorCreationEnabled == false)
      return;
   Super::CreateBehaviorInstances(utilityAIBehaviorComponent);
}
