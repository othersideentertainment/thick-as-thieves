// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/Tasks/AITask_UseSpecificGameplayBehaviorOnSmartObject.h"
#include "AI/Tasks/TATAITask_RotateToFace.h"
#include "Character/TATCharacterAIBase.h"

// ue5
#include "AIController.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "GameplayBehavior.h"
#include "GameplayBehaviorConfig.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "GameplayBehaviorSubsystem.h"
#include "MotionWarpingComponent.h"
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"
#include "AI/TATAIController.h"
#include "AI/StateTrees/SmartObjects/TATSmartObjectBehaviorDefinition.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "Misc/ScopeExit.h"
#include "Tasks/AITask_MoveTo.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AITask_UseSpecificGameplayBehaviorOnSmartObject)

UAITask_UseSpecificGameplayBehaviorOnSmartObject::UAITask_UseSpecificGameplayBehaviorOnSmartObject(
   const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   _bBehaviorFinished = false;
   bTickingTask = true;
}

UAITask_UseSpecificGameplayBehaviorOnSmartObject* UAITask_UseSpecificGameplayBehaviorOnSmartObject::UseClaimedGameplayBehaviorSmartObjectWithFilters(
   AAIController* controller,
   FBlackboardKeySelector claimKey,
   FBlackboardKeySelector behaviorClassKey,
   bool bShouldMatchSlotRotation,
   bool bLockAILogic,
   bool bShouldReleaseClaimHandle)
{
   if (!ensureMsgf(controller != nullptr, TEXT("UseClaimedGameplayBehaviorSmartObjectWithFilters called with a null controller!")))
   {
      return nullptr;
   }
   if (!ensureMsgf(claimKey.IsSet(), TEXT("UseClaimedGameplayBehaviorSmartObjectWithFilters called for %s with an invalid 'claimKey'!"),
      *controller->GetName()))
   {
      return nullptr;
   }
   if (!ensureMsgf(behaviorClassKey.IsSet(), TEXT("UseClaimedGameplayBehaviorSmartObjectWithFilters called for %s with an invalid 'behaviorClassKey'!"),
      *controller->GetName()))
   {
      return nullptr;
   }

   const UBlackboardComponent* blackboardComponent = controller->GetBlackboardComponent();
   if (blackboardComponent == nullptr)
   {
      return nullptr;
   }

   const FSmartObjectClaimHandle claimHandle = blackboardComponent->GetValue<UBlackboardKeyType_SOClaimHandle>(claimKey.SelectedKeyName);

   UClass* behaviorClass = blackboardComponent->GetValue<UBlackboardKeyType_Class>(behaviorClassKey.SelectedKeyName);
   if (behaviorClass == nullptr)
   {
      behaviorClass = USmartObjectBehaviorDefinition::StaticClass();
   }
   const TSubclassOf<USmartObjectBehaviorDefinition> behaviorDefinitionClass = behaviorClass;
   if (!ensureMsgf(behaviorClass->IsChildOf(USmartObjectBehaviorDefinition::StaticClass()),
      TEXT("UseClaimedGameplayBehaviorSmartObjectWithFilters called for %s with invalid behavior class type!"
         " Should inherit from USmartObjectBehaviorDefinition."), *controller->GetName()))
   {
      return nullptr;
   }

   return UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters(
      controller, 
      claimHandle, 
      behaviorDefinitionClass,
      bShouldMatchSlotRotation, 
      bLockAILogic, 
      bShouldReleaseClaimHandle);
}

UAITask_UseSpecificGameplayBehaviorOnSmartObject* UAITask_UseSpecificGameplayBehaviorOnSmartObject::UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters(
   AAIController* controller,
   FSmartObjectClaimHandle claimHandle,
   TSubclassOf<USmartObjectBehaviorDefinition> behaviorClass,
   bool bShouldMatchSlotRotation,
   bool bLockAILogic,
   bool bShouldReleaseClaimHandle)
{
   if (!ensureMsgf(controller != nullptr, TEXT("UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters called with a null controller!")))
   {
      return nullptr;
   }
   if (!ensureMsgf(claimHandle.IsValid(), TEXT("UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters called for %s with an invalid 'claimHandle'!"),
      *controller->GetName()))
   {
      return nullptr;
   }
   if (!ensureMsgf(behaviorClass != nullptr, TEXT("UseMoveToAndUseClaimedGameplayBehaviorSmartObjectWithFilters called for %s with a null 'behaviorClass'!"),
      *controller->GetName()))
   {
      return nullptr;
   }

   UAITask_UseSpecificGameplayBehaviorOnSmartObject* myTask = NewAITask<
      UAITask_UseSpecificGameplayBehaviorOnSmartObject>(*controller, EAITaskPriority::High);
   if (myTask == nullptr)
   {
      return nullptr;
   }

   myTask->SetClaimHandle(claimHandle);
   myTask->SetBehaviorClass(behaviorClass);
   myTask->_bShouldReleaseClaimHandle = bShouldReleaseClaimHandle;
   myTask->_bShouldMatchSlotRotation = bShouldMatchSlotRotation;

   if (bLockAILogic)
   {
      myTask->RequestAILogicLocking();
   }

   return myTask;
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::Activate()
{
   Super::Activate();
   bool bSuccess = false;
   ON_SCOPE_EXIT
   {
      if (!bSuccess)
      {
         EndTask();
      }
   };

   if (!ensureMsgf(_claimedHandle.IsValid(), TEXT("SmartObject handle must be valid at this point.")))
   {
      return;
   }

   if (OwnerController->GetPawn() == nullptr)
   {
      UE_VLOG(OwnerController, LogSmartObject, Error, TEXT("Pawn required to use claim handle: %s."),
              *LexToString(_claimedHandle));
      return;
   }

   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(OwnerController->GetWorld());
   if (!ensureMsgf(smartObjectSubsystem != nullptr, TEXT("SmartObjectSubsystem must be accessible at this point.")))
   {
      return;
   }

   // A valid claimed handle can point to an object that is no longer part of the simulation
   if (!smartObjectSubsystem->IsClaimedSmartObjectValid(_claimedHandle))
   {
      UE_VLOG(OwnerController, LogSmartObject, Log,
              TEXT("Claim handle: %s refers to an object that is no longer available."), *LexToString(_claimedHandle));
      return;
   }

   const TOptional<FVector> goalLocation = smartObjectSubsystem->GetSlotLocation(_claimedHandle);
   if (!ensureMsgf(goalLocation.IsSet(), TEXT("Unable to extract a valid slot location.")))
   {
      return;
   }

   // Register a callback to be notified if the claimed slot became unavailable
   smartObjectSubsystem->RegisterSlotInvalidationCallback(_claimedHandle,
                                                          FOnSlotInvalidated::CreateUObject(
                                                             this,
                                                             &UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnSlotInvalidated));

   FAIMoveRequest moveReq(goalLocation.GetValue());
   moveReq.SetUsePathfinding(true);
   moveReq.SetAllowPartialPath(false);
   moveReq.SetNavigationFilter(OwnerController->GetDefaultNavigationFilterClass());

   MoveToTask = UAITask::NewAITask<UAITask_MoveTo>(*OwnerController, *this, EAITaskPriority::High, TEXT("SmartObject"));
   MoveToTask->SetUp(OwnerController, moveReq);
   MoveToTask->ReadyForActivation();

   _tatOwnerController = Cast<ATATAIController>(OwnerController);
   bSuccess = true;
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::TickTask(float deltaTime)
{
   Super::TickTask(deltaTime);
   if(MoveToTask == nullptr && RotateTask == nullptr)
   {
      if(_bIsUsingStateTree)
      {
         const bool bKeepTicking = _smartObjectInteractionContext.Tick(deltaTime);
         if (bKeepTicking == false)
         {
            _bBehaviorFinished = true;
            EndTask();
         }
      }
   }
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnGameplayTaskDeactivated(UGameplayTask& task)
{   
   check(OwnerController);
   // Note at least sometimes it is possible for this to be called when the pawn is being destroyed (on end-PIE)
   //check(OwnerController->GetPawn());
   if (OwnerController->GetPawn() == nullptr)
   {
      return;
   }

   if (MoveToTask == &task)
   {
      if (!MoveToTask->IsFinished())
      {
         // MoveToTask is deactivated by pausing, possibly from the BP of Behavior itself
         EndTask();
      }
      else if (!MoveToTask->WasMoveSuccessful())
      {
         // MoveToTask failed to navigate the AI to the destination
         OnMoveToFailed.Broadcast();
         EndTask();
      }
      else
      {
         // MoveToTask succeeded
         const bool nextStepTriggered = _bShouldMatchSlotRotation ? TriggerRotation() : TriggerGameplayBehavior();
         if (!nextStepTriggered)
         {
            EndTask();
         }
      }
   }
   else if (RotateTask == &task)
   {
      if (!RotateTask->IsFinished())
      {
         // RotateTask is deactivated by pausing, possibly from the BP of Behavior itself
         EndTask();
      }
      else if (!RotateTask->WasRotationSuccessful())
      {
         // RotateTask failed to turn the AI to the necessary direction
         EndTask();
      }
      else
      {
         // RotateTask succeeded
         const bool nextStepTriggered = TriggerGameplayBehavior();
         if (!nextStepTriggered)
         {
            EndTask();
         }
      }
   }

   Super::OnGameplayTaskDeactivated(task);
}

bool UAITask_UseSpecificGameplayBehaviorOnSmartObject::TriggerRotation()
{
   // Manually resetting previous task ptrs here in case the behavior coming after that is long running. 
   // We don't need it anymore so if GC happens in the mean time it can clean it up.
   MoveToTask = nullptr;

   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(OwnerController->GetWorld());
   check(smartObjectSubsystem != nullptr); // should have previously been validated in Activate()

   const APawn* ownerPawn = OwnerController->GetPawn();
   check(ownerPawn != nullptr); // should have previously been validated in Activate()
   
   const TOptional<FTransform> goalTransform = smartObjectSubsystem->GetSlotTransform(_claimedHandle);
   if (!ensureMsgf(goalTransform.IsSet(), TEXT("Unable to extract a valid slot transform.")))
   {
      return false;
   }

   RotateTask = UTATAITask_RotateToFace::RotateToFaceDirection(OwnerController, goalTransform.GetValue().Rotator(), this);
   if (RotateTask == nullptr)
   {
      UE_VLOG(OwnerController, LogSmartObject, Log,
         TEXT("UAITask_UseSpecificGameplayBehaviorOnSmartObject::TriggerRotation failed to create RotateToFace task."));
      return false;
   }

   RotateTask->ReadyForActivation();
   return true;
}

bool UAITask_UseSpecificGameplayBehaviorOnSmartObject::TriggerGameplayBehavior()
{
   UWorld* world = OwnerController->GetWorld();
   // Manually resetting previous task ptrs here in case the behavior coming after that is long running. 
   // We don't need it anymore so if GC happens in the mean time it can clean it up.
   MoveToTask = nullptr;
   RotateTask = nullptr;
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(world);
   if (ensure(smartObjectSubsystem))
   {
      if (smartObjectSubsystem->GetSlotState(_claimedHandle.SlotHandle) != ESmartObjectSlotState::Claimed)
      {
         UE_VLOG(OwnerController, LogSmartObject, Error,
            TEXT("Claim handle %s was not claimed when ending movement. (Is the agent dead?)"),
            *LexToString(_claimedHandle));
         EndTask();
         return false;
      }
      if (const UTATSmartObjectBehaviorDefinition* interactionSmartObjectBehaviorDefinition =
         smartObjectSubsystem->GetBehaviorDefinition<UTATSmartObjectBehaviorDefinition>(_claimedHandle))
      {
         _bIsUsingStateTree = true;
         if(smartObjectSubsystem->MarkSlotAsOccupied<UTATSmartObjectBehaviorDefinition>(_claimedHandle) == nullptr)
            return false;
         if(_tatOwnerController)
         {
            _tatOwnerController->SetCurrentActiveSmartObjectBehavior(this);
         }
         _smartObjectInteractionContext.SetContextController(OwnerController);
         _smartObjectInteractionContext.SetContextPawn(OwnerController->GetPawn());
         const USmartObjectComponent* smartObjectComponent = smartObjectSubsystem->GetSmartObjectComponent(_claimedHandle);
         _smartObjectInteractionContext.SetSmartObjectActor(smartObjectComponent ? smartObjectComponent->GetOwner() : nullptr);
         _smartObjectInteractionContext.SetClaimedHandle(_claimedHandle);
         return _smartObjectInteractionContext.Activate(*interactionSmartObjectBehaviorDefinition);
      }

      if (const UGameplayBehaviorSmartObjectBehaviorDefinition* smartObjectGameplayBehaviorDefinition =
         Cast<UGameplayBehaviorSmartObjectBehaviorDefinition>(smartObjectSubsystem->GetBehaviorDefinition(_claimedHandle, _behaviorClass)))
      {
         _bIsUsingStateTree = false;
         if(smartObjectSubsystem->MarkSlotAsOccupied<UGameplayBehaviorSmartObjectBehaviorDefinition>(_claimedHandle) == nullptr)
            return false;
         const UGameplayBehaviorConfig* gameplayBehaviorConfig = smartObjectGameplayBehaviorDefinition != nullptr
            ? smartObjectGameplayBehaviorDefinition->GameplayBehaviorConfig : nullptr;
         GameplayBehavior = gameplayBehaviorConfig != nullptr
            ? gameplayBehaviorConfig->GetBehavior(*world) : nullptr;


         const TOptional<FTransform> goalTransform = smartObjectSubsystem->GetSlotTransform(_claimedHandle);

         if (goalTransform.IsSet())
         {
            if (const ATATCharacterAIBase* character = OwnerController->GetPawn<ATATCharacterAIBase>())
            {
               character->GetMotionWarpingComponent()->AddOrUpdateWarpTarget(FMotionWarpingTarget(TEXT("StartPoint"), goalTransform.GetValue()));
            }
         }
         if (GameplayBehavior != nullptr)
         {
            const USmartObjectComponent* smartObjectComponent = smartObjectSubsystem->GetSmartObjectComponent(
               _claimedHandle);
            AActor& interactorActor = *OwnerController->GetPawn();
            AActor* interacteeActor = smartObjectComponent ? smartObjectComponent->GetOwner() : nullptr;
            
            // Behavior can be successfully triggered AND ended synchronously. We are only interested to register callback when still running
            if (UGameplayBehaviorSubsystem::TriggerBehavior(*GameplayBehavior, interactorActor, gameplayBehaviorConfig, interacteeActor))
            {
               _onBehaviorFinishedNotifyHandle = GameplayBehavior->GetOnBehaviorFinishedDelegate().AddUObject(
                  this, &UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnSmartObjectBehaviorFinished);

               return true;
            }
         }
      }
   }

   return false;
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::SendStateTreeEvent(const FStateTreeEvent& event)
{
   if(_bIsUsingStateTree == false)
   {
      return;
   }
   _smartObjectInteractionContext.SendEvent(event.Tag, event.Payload, event.Origin);
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::Abort()
{
   check(OwnerController);
   // Note at least sometimes it is possible for this to be called when the pawn is being destroyed (on end-PIE)
   //check(OwnerController->GetPawn());

   if(_tatOwnerController)
   {
      _tatOwnerController->SetCurrentActiveSmartObjectBehavior(nullptr);
   }
   
   if(const ATATCharacterAIBase* character = OwnerController->GetPawn<ATATCharacterAIBase>())
   {
      character->GetMotionWarpingComponent()->RemoveWarpTarget(TEXT("StartPoint"));
   }

   if(_bShouldReleaseClaimHandle)
   {
      const UWorld* world = OwnerController->GetWorld();
      if (USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(world))
      {
         const ESmartObjectSlotState slotState = smartObjectSubsystem->GetSlotState(_claimedHandle.SlotHandle);
         if(slotState == ESmartObjectSlotState::Claimed || slotState == ESmartObjectSlotState::Occupied)
         {
            smartObjectSubsystem->Release(_claimedHandle);                  
         }
      }
   }

   if (MoveToTask)
   {
      // clear before triggering 'the end' so that OnGameplayTaskDeactivated
      // ignores the incoming info about task end
      UAITask_MoveTo* task = MoveToTask;
      MoveToTask = nullptr;
      task->ExternalCancel();
   }
   else if (RotateTask)
   {
      // clear before triggering 'the end' so that OnGameplayTaskDeactivated
      // ignores the incoming info about task end
      UTATAITask_RotateToFace* task = RotateTask;
      RotateTask = nullptr;
      task->ExternalCancel();
   }
   else if (!_bBehaviorFinished)
   {
      if(_bIsUsingStateTree)
      {
      }
      else
      {
         if (GameplayBehavior != nullptr)
         {
            GameplayBehavior->GetOnBehaviorFinishedDelegate().Remove(_onBehaviorFinishedNotifyHandle);
            AActor* pawn = OwnerController->GetPawn();
            // TODO: mildly sketch if not tearing down the world
            if (pawn)
            {
               GameplayBehavior->AbortBehavior(*pawn);
            }
         }
      }
   }
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnDestroy(bool bInOwnerFinished)
{
   Abort();

   if (TaskState != EGameplayTaskState::Finished)
   {
      if(_bIsUsingStateTree)
      {
         _smartObjectInteractionContext.Deactivate();
         if (_bBehaviorFinished)
         {
            OnSucceeded.Broadcast();
         }
         else
         {
            OnFailed.Broadcast();
         }
      }
      else
      {
         if (GameplayBehavior)
         {
            OnSucceeded.Broadcast();
         }
         else
         {
            OnFailed.Broadcast();
         }
      }
   }
   Super::OnDestroy(bInOwnerFinished);
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnSmartObjectBehaviorFinished(UGameplayBehavior& behavior,
   AActor& avatar, const bool bInterrupted)
{
   // Adding an ensure in case the assumptions change in the future.
   ensure(OwnerController != nullptr);

   // make sure we handle the right pawn - we can get this notify for a different
   // Avatar if the behavior sending it out is not instanced (CDO is being used to perform actions)
   if (OwnerController && OwnerController->GetPawn() == &avatar)
   {
      behavior.GetOnBehaviorFinishedDelegate().Remove(_onBehaviorFinishedNotifyHandle);
      _bBehaviorFinished = true;
      EndTask();
   }
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::OnSlotInvalidated(const FSmartObjectClaimHandle& claimHandle,
                                                                         const ESmartObjectSlotState state)
{
   // On world teardown, slots getting invalidated cause there to be checks against the owner controller
   // which usually has already had the pawn destroyed, causing a crash on the check
   if (OwnerController == nullptr || OwnerController->GetPawn() == nullptr)
   {
      return;
   }

   Abort();

   EndTask();
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::SetClaimHandle(const FSmartObjectClaimHandle& claimHandle)
{
   _claimedHandle = claimHandle;
}

void UAITask_UseSpecificGameplayBehaviorOnSmartObject::SetBehaviorClass(
   const TSubclassOf<USmartObjectBehaviorDefinition> behaviorClass)
{
   _behaviorClass = behaviorClass;
}
