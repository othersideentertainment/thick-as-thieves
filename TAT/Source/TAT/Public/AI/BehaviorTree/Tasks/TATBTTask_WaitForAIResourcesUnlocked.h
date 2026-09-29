// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "BehaviorTree/BTTaskNode.h"

#include "TATBTTask_WaitForAIResourcesUnlocked.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATBTTask_WaitForAIResourcesUnlocked, Log, All);

class ATATAIController;

UCLASS()
class TAT_API UTATBTTask_WaitForAIResourcesUnlocked : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()
	
public:
   // from UBTTaskNode
   virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;

   // from UBTNode
   virtual void SetOwner(AActor* actorOwner) override;

   // If true, wait for movement resource locks to be lifted before finishing.
   UPROPERTY(EditAnywhere)
   bool TrackMovementLocked = true;

   // If true, wait for logic resource locks to be lifted before finishing.
   UPROPERTY(EditAnywhere)
   bool TrackLogicLocked = true;

protected:
   UPROPERTY(Transient)
   ATATAIController* AIController;

   UPROPERTY(Transient)
   UBehaviorTreeComponent* OwnerComponent;

   void OnAIResourceLockChanged(bool isMovementLocked, bool isLogicLocked);

   void CleanUp();
};
