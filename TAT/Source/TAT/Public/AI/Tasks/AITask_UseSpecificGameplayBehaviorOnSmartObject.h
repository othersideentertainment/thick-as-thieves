// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeContext.h"
#include "AI/TATAIController.h"

// ue
#include "SmartObjectRuntime.h"
#include "Tasks/AITask.h"

#include "AITask_UseSpecificGameplayBehaviorOnSmartObject.generated.h"

class AAIController;
class UAITask_MoveTo;
class UGameplayBehavior;
class USmartObjectComponent;
class UTATAITask_RotateToFace;

// Smart objects can have multiple behaviors set on a slot, however the default implementation only ever uses the first
// behavior, this allows us to specify which slot handle + behavior to activate.
UCLASS()
class TAT_API UAITask_UseSpecificGameplayBehaviorOnSmartObject : public UAITask
{
   GENERATED_BODY()
public:
   UAITask_UseSpecificGameplayBehaviorOnSmartObject(
      const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   UFUNCTION(BlueprintCallable, Category = "TAT|AI|Tasks", meta = (DefaultToSelf = "Controller" , BlueprintInternalUseOnly = "true"))
   static UAITask_UseSpecificGameplayBehaviorOnSmartObject* UseClaimedGameplayBehaviorSmartObjectWithFilters(
      AAIController* controller, FBlackboardKeySelector claimKey, FBlackboardKeySelector behaviorClassKey,
      bool bShouldMatchSlotRotation = false,
      bool bLockAILogic = true,
      bool bShouldReleaseClaimHandle = false);
   
   UFUNCTION(BlueprintCallable, Category = "TAT|AI|Tasks", meta = (DefaultToSelf = "Controller" , BlueprintInternalUseOnly = "true"))
   static UAITask_UseSpecificGameplayBehaviorOnSmartObject* UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters(
      AAIController* controller, FSmartObjectClaimHandle claimHandle,  TSubclassOf<USmartObjectBehaviorDefinition> behaviorClass,
      bool bShouldMatchSlotRotation = false,
      bool bLockAILogic = true,
      bool bShouldReleaseClaimHandle = false);
   
   void SendStateTreeEvent(const FStateTreeEvent& event);

protected:
   virtual void Activate() override;
   virtual void TickTask(float deltaTime) override;
   virtual void OnGameplayTaskDeactivated(UGameplayTask& task) override;

   void Abort();
   
   virtual void OnDestroy(bool bInOwnerFinished) override;
   void OnSmartObjectBehaviorFinished(UGameplayBehavior& behavior, AActor& avatar, const bool bInterrupted);
   void OnSlotInvalidated(const FSmartObjectClaimHandle& claimHandle, const ESmartObjectSlotState state);

   void SetClaimHandle(const FSmartObjectClaimHandle& claimHandle);  
   void SetBehaviorClass(TSubclassOf<USmartObjectBehaviorDefinition> behaviorClass);

   bool TriggerRotation();
   bool TriggerGameplayBehavior();

   UPROPERTY(BlueprintAssignable)
   FGenericGameplayTaskDelegate OnSucceeded;

   UPROPERTY(BlueprintAssignable)
   FGenericGameplayTaskDelegate OnFailed;

   UPROPERTY(BlueprintAssignable)
   FGenericGameplayTaskDelegate OnMoveToFailed;

   UPROPERTY()
   TObjectPtr<UAITask_MoveTo> MoveToTask { nullptr };

   UPROPERTY()
   TObjectPtr<UTATAITask_RotateToFace> RotateTask{ nullptr };

   UPROPERTY()
   TObjectPtr<UGameplayBehavior> GameplayBehavior { nullptr };

   FSmartObjectClaimHandle _claimedHandle;
   TSubclassOf<USmartObjectBehaviorDefinition> _behaviorClass { nullptr };
   FDelegateHandle _onBehaviorFinishedNotifyHandle;

   bool _bBehaviorFinished {false};
   bool _bShouldReleaseClaimHandle {false};
   bool _bShouldMatchSlotRotation {false};

   bool _bIsUsingStateTree { false };
   
   UPROPERTY()
   FTATSmartObjectStateTreeContext _smartObjectInteractionContext;
   UPROPERTY()
   TObjectPtr<ATATAIController> _tatOwnerController { nullptr };
};
