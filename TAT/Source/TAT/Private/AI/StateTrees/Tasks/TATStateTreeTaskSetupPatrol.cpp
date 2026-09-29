// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskSetupPatrol.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Patrol/PatrolPath.h"
#include "AI/Patrol/TATAIPatrolAccessorInterface.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetupPatrol)

EStateTreeRunStatus FTATStateTreeTaskSetupPatrol::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if(aiController == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   const APawn* pawn = aiController->GetPawn();
   if(pawn == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   const APatrolPath* patrolPath = ITATAIPatrolAccessorInterface::Execute_GetPatrolPath(pawn);
   if(patrolPath == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   UBlackboardComponent* blackboard = aiController->GetBlackboardComponent();
   if(blackboard == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   FPatrolPoint* patrolPointInstance = instanceData.PatrolData.GetMutablePtr<FPatrolPoint>(context);
   FVector* currentPatrolWorldLocation = instanceData.PatrolWorldLocation.GetMutablePtr<FVector>(context);

   FVector* patrolBrokenLocationPtr = instanceData.PatrolBrokenLocation.GetMutablePtr<FVector>(context);
   bool* patrolWasInterruptedPtr = instanceData.PatrolWasInterrupted.GetMutablePtr<bool>(context);

   if(patrolPointInstance == nullptr || currentPatrolWorldLocation == nullptr || patrolBrokenLocationPtr == nullptr || patrolWasInterruptedPtr == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   *patrolBrokenLocationPtr = blackboard->GetValueAsVector(ATATAIController::GetPatrolBrokenLocationBlackboardKey());
   *patrolWasInterruptedPtr = blackboard->IsVectorValueSet(ATATAIController::GetPatrolBrokenLocationBlackboardKey());
   
   const int currentPatrolPointIndexInBlackboard = blackboard->GetValueAsInt(ATATAIController::GetPatrolIndexBlackboardKey());
   
   if(currentPatrolPointIndexInBlackboard == INDEX_NONE)
   {
      // Get Closest point as this is the first time we're running this.
      const FVector currentActorPosition = pawn->GetActorLocation();
      float closestDistance = FLT_MAX;
      int closestPatrolPointIndex = 0;
      for (int i=0; i < patrolPath->Points.Num(); ++i)
      {
         const float distanceToPoint = FVector::DistSquared(patrolPath->GetPointLocationWorldSpace(i), currentActorPosition);
         if(closestDistance > distanceToPoint)
         {
            closestDistance = distanceToPoint;
            closestPatrolPointIndex = i;
         }
      }
      
      blackboard->SetValueAsInt(ATATAIController::GetPatrolIndexBlackboardKey(), closestPatrolPointIndex);
      blackboard->SetValueAsBool(ATATAIController::GetPatrolPointDirectionForward(), true);
      *patrolPointInstance = patrolPath->Points[closestPatrolPointIndex];
      *currentPatrolWorldLocation = patrolPath->GetPointLocationWorldSpace(closestPatrolPointIndex);
      return EStateTreeRunStatus::Succeeded;
   }
   else
   {
      *patrolPointInstance = patrolPath->Points[currentPatrolPointIndexInBlackboard];
      *currentPatrolWorldLocation = patrolPath->GetPointLocationWorldSpace(currentPatrolPointIndexInBlackboard);
      return EStateTreeRunStatus::Succeeded;
   }
}

EStateTreeRunStatus FTATStateTreeTaskCompleteBrokenSegment::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if(aiController == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   UBlackboardComponent* blackboard = aiController->GetBlackboardComponent();
   if(blackboard == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   FVector* patrolBrokenLocationPtr = instanceData.PatrolBrokenLocation.GetMutablePtr<FVector>(context);
   bool* patrolWasInterruptedPtr = instanceData.PatrolWasInterrupted.GetMutablePtr<bool>(context);

   if(patrolBrokenLocationPtr == nullptr || patrolWasInterruptedPtr == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   *patrolBrokenLocationPtr = FVector::ZeroVector;
   *patrolWasInterruptedPtr = false;   
   blackboard->SetValueAsVector(ATATAIController::GetPatrolBrokenLocationBlackboardKey(), FAISystem::InvalidLocation);

   return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FTATStateTreeTaskCompletePatrolSegment::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if(aiController == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   const APawn* pawn = aiController->GetPawn();
   if(pawn == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   const APatrolPath* patrolPath = ITATAIPatrolAccessorInterface::Execute_GetPatrolPath(pawn);
   if(patrolPath == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   UBlackboardComponent* blackboard = aiController->GetBlackboardComponent();
   if(blackboard == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   const int32 currentPatrolPointIndex = blackboard->GetValueAsInt(ATATAIController::GetPatrolIndexBlackboardKey());
   const bool currentPatrolDirectionForward = blackboard->GetValueAsBool(ATATAIController::GetPatrolPointDirectionForward());
   const FNextPointData nextPoint = patrolPath->GetNextPoint(currentPatrolPointIndex, currentPatrolDirectionForward);
   
   blackboard->SetValueAsInt(ATATAIController::GetPatrolIndexBlackboardKey(), nextPoint.nextIndexID);
   blackboard->SetValueAsBool(ATATAIController::GetPatrolPointDirectionForward(), nextPoint.forwardMovementDirection);

   return EStateTreeRunStatus::Succeeded;
}
