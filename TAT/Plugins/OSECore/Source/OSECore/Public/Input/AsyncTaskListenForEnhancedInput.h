// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "EnhancedInputComponent.h"

#include "AsyncTaskListenForEnhancedInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnEnhancedInputEvent, const UInputAction*, action, const FInputActionValue&, actionValue, float, elapsedSeconds, float, triggeredSeconds);

class APlayerController;
struct FInputActionInstance;
class UInputAction;

/**
 * Blueprint node to automatically register a listener for enhanced input
 * NOTE: You must call EndTask() when the owner of this async task is going away (Destruct in UI, EndPlay in actors)
 */
UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskListenForEnhancedInput : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   UPROPERTY(BlueprintAssignable)
   FOnEnhancedInputEvent Started;
   UPROPERTY(BlueprintAssignable)
   FOnEnhancedInputEvent Ongoing;
   UPROPERTY(BlueprintAssignable)
   FOnEnhancedInputEvent Canceled;
   UPROPERTY(BlueprintAssignable)
   FOnEnhancedInputEvent Triggered;
   UPROPERTY(BlueprintAssignable)
   FOnEnhancedInputEvent Completed;
   
   // Listens for an enhanced input event
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnLocalEnhancedInputListener")
   static UAsyncTaskListenForEnhancedInput* ListenForLocalEnhancedInput(UObject* worldContextObject, UInputAction* inputAction);

   // Listens for an enhanced input event
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnLocalEnhancedInputsListener")
   static UAsyncTaskListenForEnhancedInput* ListenForLocalEnhancedInputs(UObject* worldContextObject, TArray<UInputAction*> inputActions);

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

protected:
   void _OnInputEvent_Started(const FInputActionInstance& actionInstance);
   void _OnInputEvent_Ongoing(const FInputActionInstance& actionInstance);
   void _OnInputEvent_Canceled(const FInputActionInstance& actionInstance);
   void _OnInputEvent_Triggered(const FInputActionInstance& actionInstance);
   void _OnInputEvent_Completed(const FInputActionInstance& actionInstance);

private:
   TArray<int> _bindingHandles;
   TWeakObjectPtr<UEnhancedInputComponent> _enhInputComponent = nullptr;
};
