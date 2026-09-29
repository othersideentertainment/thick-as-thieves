// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/TATStateTreeConditionActorDistanceFromOwner.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionActorDistanceFromOwner)

bool FTATStateTreeConditionActorDistanceFromOwner::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.Target == nullptr)
      return false;

   const AAIController* aiController = instanceData.Controller;
   if(aiController == nullptr)
      return false;
   if(const auto pawn = aiController->GetPawn())
   {
      float agentRadius = 0.f;
      float agentHalfHeight = 0.f;
      pawn->GetSimpleCollisionCylinder(agentRadius, agentHalfHeight);
      
      const FVector toGoal = instanceData.Target->GetActorLocation() - pawn->GetActorLocation();
	   const FVector::FReal dist2DSq = toGoal.SizeSquared2D();
      const FVector::FReal useRadius = instanceData.ComparisonValue + (agentRadius * 0.05f);
      return UOSEMathFunctionLibrary::CompareFloats(dist2DSq, FMath::Square(useRadius), instanceData.ComparisonMethod);
   }
   return false;
}
