// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/BehaviorTree/Tasks/TATBTTask_WaitForAIResourcesUnlocked.h"

// tat
#include "AI/TATAIController.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBTTask_WaitForAIResourcesUnlocked)

DEFINE_LOG_CATEGORY(LogTATBTTask_WaitForAIResourcesUnlocked);

UTATBTTask_WaitForAIResourcesUnlocked::UTATBTTask_WaitForAIResourcesUnlocked(const FObjectInitializer& ObjectInitializer) 
   : Super(ObjectInitializer)
{
   bCreateNodeInstance = true;
}

EBTNodeResult::Type UTATBTTask_WaitForAIResourcesUnlocked::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
   OwnerComponent = &ownerComp;

   if (IsValid(AIController))
   {
      bool isMovementLocked = false;
      bool isLogicLocked = false;
      AIController->GetAIResourceLockStates(isMovementLocked, isLogicLocked);

      if ((isMovementLocked && TrackMovementLocked) || (TrackLogicLocked && isLogicLocked))
      {
         AIController->OnAIResourceLockChanged.AddUObject(this, &UTATBTTask_WaitForAIResourcesUnlocked::OnAIResourceLockChanged);
         return EBTNodeResult::InProgress;
      }
      else
      {
         UE_CLOG(!(TrackMovementLocked || TrackLogicLocked), LogTATBTTask_WaitForAIResourcesUnlocked, Warning, 
            TEXT("'TrackMovementLocked' and 'TrackMovementLocked' were both set to false. Auto-succeeding UTATBTTask_WaitForAIResourcesUnlocked node..."));
         return EBTNodeResult::Succeeded;
      }
   }

   return EBTNodeResult::Failed;
}

void UTATBTTask_WaitForAIResourcesUnlocked::SetOwner(AActor* actorOwner)
{
   AIController = Cast<ATATAIController>(actorOwner);
}

void UTATBTTask_WaitForAIResourcesUnlocked::OnAIResourceLockChanged(bool isMovementLocked, bool isLogicLocked)
{
   if (!(isMovementLocked && TrackMovementLocked) || (TrackLogicLocked && isLogicLocked))
   {
      UBehaviorTreeComponent* owner = OwnerComponent; // will be cleared in CleanUp, make a quick local copy
      CleanUp();
      FinishLatentTask(*owner, EBTNodeResult::Succeeded);
   }
}

void UTATBTTask_WaitForAIResourcesUnlocked::CleanUp()
{
   if (IsValid(AIController))
   {
      AIController->OnAIResourceLockChanged.RemoveAll(this);
      AIController = nullptr;
   }

   OwnerComponent = nullptr;
}
