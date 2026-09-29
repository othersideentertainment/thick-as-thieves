// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Services/BTService_RunUtilityBehaviorsBase.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/Utility/UtilityAIBehavior.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"

// ue4
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_RunUtilityBehaviorsBase)

DEFINE_LOG_CATEGORY_STATIC(LogBTService_RunUtilityBehaviorsBase, Log, All);

UBTService_RunUtilityBehaviorsBase::UBTService_RunUtilityBehaviorsBase(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run Utility Behaviors";
   bCreateNodeInstance = true;
   bNotifyBecomeRelevant = true;
   bNotifyCeaseRelevant = true;
   bCallTickOnSearchStart = true;

   // accept only FName's
   _behaviorNameBlackboardKey.AddNameFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_RunUtilityBehaviorsBase, _behaviorNameBlackboardKey));
}

void UBTService_RunUtilityBehaviorsBase::InitializeFromAsset(UBehaviorTree& asset)
{
   Super::InitializeFromAsset(asset);
}

void UBTService_RunUtilityBehaviorsBase::OnInstanceCreated(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceCreated(ownerComp);
   
   UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : OnInstanceCreated"), *GetName(), *GetStaticDescription());
}

void UBTService_RunUtilityBehaviorsBase::OnSearchStart(FBehaviorTreeSearchData& searchData)
{
   Super::OnSearchStart(searchData);
}

void UBTService_RunUtilityBehaviorsBase::OnBecomeRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
   Super::OnBecomeRelevant(ownerComp, nodeMemory);

   UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : OnBecomeRelevant"), *GetName(), *GetStaticDescription());
}

void UBTService_RunUtilityBehaviorsBase::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
   Super::TickNode(ownerComp, nodeMemory, deltaSeconds);

   if (UUtilityAIComponent* utilityAIComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      if (ShouldSetBehaviors(*utilityAIComponent))
      {
         utilityAIComponent->SetStatesOwner(this);
         SetBehaviors(*utilityAIComponent);
         UE_VLOG_UELOG(ownerComp.GetOwner(), LogBTService_RunUtilityBehaviorsBase, Log, TEXT("%s : %s : TickNode : Installed New Behaviors"), *GetName(), *GetStaticDescription());
      }

      if (utilityAIComponent->GetStatesOwner() == this)
      {
         _UpdateBlackboard(*utilityAIComponent);
         UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : TickNode : Tick"), *GetName(), *GetStaticDescription());
      }
   }
}

void UBTService_RunUtilityBehaviorsBase::OnCeaseRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
   Super::OnCeaseRelevant(ownerComp, nodeMemory);

   if (UUtilityAIComponent* utilityAIComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      if (utilityAIComponent->GetStatesOwner() == this)
      {
         utilityAIComponent->ClearStates();
         utilityAIComponent->SetStatesOwner(nullptr);
         _UpdateBlackboard(*utilityAIComponent);

         UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : OnCeaseRelevant"), *GetName(), *GetStaticDescription());
      }
      else
      {
         UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : OnCeaseRelevant (not current!)"), *GetName(), *GetStaticDescription());
      }
   }
}

void UBTService_RunUtilityBehaviorsBase::OnInstanceDestroyed(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceDestroyed(ownerComp);
   _behaviors.Reset();
   UE_LOG(LogBTService_RunUtilityBehaviorsBase, Verbose, TEXT("%s : %s : OnInstanceDestroyed"), *GetName(), *GetStaticDescription());
}

void UBTService_RunUtilityBehaviorsBase::SetBehaviors(UUtilityAIComponent& utilityAIComponent)
{
   utilityAIComponent.SetStates(GetBehaviorInstances());
}

bool UBTService_RunUtilityBehaviorsBase::ShouldSetBehaviors(const UUtilityAIComponent& utilityAIComponent) const
{
   return utilityAIComponent.GetStatesOwner() != this;
}

UUtilityAIBehaviorComponent* UBTService_RunUtilityBehaviorsBase::_GetUtilityAIBehaviorComponent(const UBehaviorTreeComponent& ownerComp) const
{
   if (const AOSEAIController* controller = Cast<AOSEAIController>(ownerComp.GetOwner()))
   {
      return controller->GetUtilityAIBehaviorComponent();
   }
   return nullptr;
}

UUtilityAIGoalComponent* UBTService_RunUtilityBehaviorsBase::_GetUtilityAIGoalComponent(const UBehaviorTreeComponent& ownerComp) const
{
   if (const AOSEAIController* controller = Cast<AOSEAIController>(ownerComp.GetOwner()))
   {
      return controller->GetUtilityAIGoalComponent();
   }
   return nullptr;
}

void UBTService_RunUtilityBehaviorsBase::_UpdateBlackboard(UUtilityAIComponent& utilityAIComp)
{
   if (AActor* ownerActor = utilityAIComp.GetOwner())
   {
      // TODO: maybe add an interface and not a find?  they do this lookup a lot in engine code, too, idk
      if (UBlackboardComponent* blackboardComponent = ownerActor->FindComponentByClass<UBlackboardComponent>())
      {
         const FUtilityStateEvaluatorInstance& currentInstance = utilityAIComp.GetCurrentEvaluatorInstance();

         if (currentInstance.IsValid())
         {
            blackboardComponent->SetValue<UBlackboardKeyType_Name>(_behaviorNameBlackboardKey.SelectedKeyName, currentInstance.Name);
         }
         else
         {
            blackboardComponent->SetValue<UBlackboardKeyType_Name>(_behaviorNameBlackboardKey.SelectedKeyName, NAME_None);
         }
      }
   }
}

