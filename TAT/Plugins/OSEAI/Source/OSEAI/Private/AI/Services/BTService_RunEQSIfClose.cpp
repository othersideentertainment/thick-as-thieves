// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Services/BTService_RunEQSIfClose.h"

// ue5

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Navigation/PathFollowingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_RunEQSIfClose)

UBTService_RunEQSIfClose::UBTService_RunEQSIfClose(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run EQS query if close to target";
}

void UBTService_RunEQSIfClose::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{

   AActor* queryOwner = ownerComp.GetOwner();
   AController* controllerOwner = Cast<AController>(queryOwner);
   if (controllerOwner)
   {
      queryOwner = controllerOwner->GetPawn();
   }

   if (queryOwner && EQSRequest.IsValid())
   {
      const UBlackboardComponent* blackboardComponent = ownerComp.GetBlackboardComponent();
      FBTEQSServiceMemory* myMemory = CastInstanceNodeMemory<FBTEQSServiceMemory>(nodeMemory);

      // Trigger new query only if the previous one has already finished and close enough
      if (myMemory->RequestID == INDEX_NONE && _IsCloseEnough(ownerComp, blackboardComponent))
      {
         myMemory->RequestID = EQSRequest.Execute(*queryOwner, blackboardComponent, QueryFinishedDelegate);
      }
   }

   // HACK: explicitly skipping UBTService_RunEQS::TickNode, so we can add other criteria
   //       This is not ideal, but avoids a lot more duplication
   UBTService::TickNode(ownerComp, nodeMemory, deltaSeconds);
}

bool UBTService_RunEQSIfClose::_IsCloseEnough(UBehaviorTreeComponent & ownerComp, const UBlackboardComponent* blackboardComponent) const
{
   AAIController* aiOwner = ownerComp.GetAIOwner();
   UPathFollowingComponent* pathComponent = aiOwner ? aiOwner->GetPathFollowingComponent() : nullptr;
   if (pathComponent == nullptr) return false;

   // support only vectors for now
   const FVector targetLocation = blackboardComponent->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
   if (FAISystem::IsValidLocation(targetLocation))
   {
      return pathComponent->HasReached(targetLocation, EPathFollowingReachMode::OverlapAgent, CloseRadius);
   }

   return false;
}

