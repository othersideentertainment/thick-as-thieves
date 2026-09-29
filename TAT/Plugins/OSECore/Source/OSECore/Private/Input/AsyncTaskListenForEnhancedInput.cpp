// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/AsyncTaskListenForEnhancedInput.h"

// ose
#include "Player/OSEPlayerController.h"

// ue4
#include "EnhancedInputComponent.h"
#include "InputAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskListenForEnhancedInput)

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTaskListenForEnhancedInput, Log, All);

UAsyncTaskListenForEnhancedInput* UAsyncTaskListenForEnhancedInput::ListenForLocalEnhancedInput(UObject* worldContextObject, UInputAction* inputAction)
{
   TArray<UInputAction*> inputActions;
   inputActions.Add(inputAction);
   return ListenForLocalEnhancedInputs(worldContextObject, inputActions);
}

UAsyncTaskListenForEnhancedInput* UAsyncTaskListenForEnhancedInput::ListenForLocalEnhancedInputs(UObject* worldContextObject, TArray<UInputAction*> inputActions)
{
   AOSEPlayerController* pc = AOSEPlayerController::GetLocalOSEPlayerController(worldContextObject);
   if (!IsValid(pc))
   {
      UE_LOG(LogAsyncTaskListenForEnhancedInput, Error, TEXT("%s: Cannot instantiate AsyncTaskListenForEnhancedInput, cannot find local player controller!"), *worldContextObject->GetName());
      return nullptr;
   }

   UEnhancedInputComponent* enhInputComponent = Cast<UEnhancedInputComponent>(pc->InputComponent);
   if (!IsValid(enhInputComponent))
   {
      UE_LOG(LogAsyncTaskListenForEnhancedInput, Error, TEXT("%s: Cannot instantiate AsyncTaskListenForEnhancedInput, enhanced input component was not found on the player controller!"), *worldContextObject->GetName());
      return nullptr;
   }

   if (inputActions.Num() == 0)
   {
      UE_LOG(LogAsyncTaskListenForEnhancedInput, Error, TEXT("%s: Cannot instantiate AsyncTaskListenForEnhancedInput, empty actions array!"), *worldContextObject->GetName());
      return nullptr;
   }

   UAsyncTaskListenForEnhancedInput* listenForInputTask = NewObject<UAsyncTaskListenForEnhancedInput>();
   check(listenForInputTask);
   listenForInputTask->_enhInputComponent = enhInputComponent;

   for(UInputAction* inputAction : inputActions)
   {
      if (!inputAction)
         continue;
      
      listenForInputTask->_bindingHandles.Add(enhInputComponent->BindAction(inputAction, ETriggerEvent::Started, listenForInputTask, &UAsyncTaskListenForEnhancedInput::_OnInputEvent_Started).GetHandle());
      listenForInputTask->_bindingHandles.Add(enhInputComponent->BindAction(inputAction, ETriggerEvent::Ongoing, listenForInputTask, &UAsyncTaskListenForEnhancedInput::_OnInputEvent_Ongoing).GetHandle());
      listenForInputTask->_bindingHandles.Add(enhInputComponent->BindAction(inputAction, ETriggerEvent::Canceled, listenForInputTask, &UAsyncTaskListenForEnhancedInput::_OnInputEvent_Canceled).GetHandle());
      listenForInputTask->_bindingHandles.Add(enhInputComponent->BindAction(inputAction, ETriggerEvent::Triggered, listenForInputTask, &UAsyncTaskListenForEnhancedInput::_OnInputEvent_Triggered).GetHandle());
      listenForInputTask->_bindingHandles.Add(enhInputComponent->BindAction(inputAction, ETriggerEvent::Completed, listenForInputTask, &UAsyncTaskListenForEnhancedInput::_OnInputEvent_Completed).GetHandle());
   }

   return listenForInputTask;
}

void UAsyncTaskListenForEnhancedInput::EndTask()
{
   if (_enhInputComponent.IsValid())
   {
      for(int bindingHandle : _bindingHandles)
      {
         _enhInputComponent->RemoveBindingByHandle(bindingHandle);
      }
   }
   _bindingHandles.Reset();

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UAsyncTaskListenForEnhancedInput::_OnInputEvent_Started(const FInputActionInstance& actionInstance)
{
   Started.Broadcast(actionInstance.GetSourceAction(), actionInstance.GetValue(), actionInstance.GetElapsedTime(), actionInstance.GetTriggeredTime());
}

void UAsyncTaskListenForEnhancedInput::_OnInputEvent_Ongoing(const FInputActionInstance& actionInstance)
{
   Ongoing.Broadcast(actionInstance.GetSourceAction(), actionInstance.GetValue(), actionInstance.GetElapsedTime(), actionInstance.GetTriggeredTime());
}

void UAsyncTaskListenForEnhancedInput::_OnInputEvent_Canceled(const FInputActionInstance& actionInstance)
{
   Canceled.Broadcast(actionInstance.GetSourceAction(), actionInstance.GetValue(), actionInstance.GetElapsedTime(), actionInstance.GetTriggeredTime());
}

void UAsyncTaskListenForEnhancedInput::_OnInputEvent_Triggered(const FInputActionInstance& actionInstance)
{
   Triggered.Broadcast(actionInstance.GetSourceAction(), actionInstance.GetValue(), actionInstance.GetElapsedTime(), actionInstance.GetTriggeredTime());
}

void UAsyncTaskListenForEnhancedInput::_OnInputEvent_Completed(const FInputActionInstance& actionInstance)
{
   Completed.Broadcast(actionInstance.GetSourceAction(), actionInstance.GetValue(), actionInstance.GetElapsedTime(), actionInstance.GetTriggeredTime());
}

