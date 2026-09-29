// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "EnhancedInputComponent.h"

#include "AsyncTaskWaitForToolIsReady.generated.h"

class UToolComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FToolIsReadyEvent, UToolComponent*, toolComponent);

/// Blueprint node to get a callback when a tool is ready.  Does not require manual task completion, cleans itself up after sending the event.
UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskWaitForToolIsReady : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   UPROPERTY(BlueprintAssignable)
   FToolIsReadyEvent ToolIsReady;
   
   /// Sends an event callback when a tool is ready
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
   static UAsyncTaskWaitForToolIsReady* WaitForToolIsReady(UObject* worldContextObject, UToolComponent* toolComponent);

   // from UBlueprintAsyncActionBase
   virtual void Activate() override;

private:
   UFUNCTION()
   void _TickIsToolReady();

private:
   FTimerHandle _timerHandle;
   TWeakObjectPtr<UToolComponent> _toolComponent = nullptr;
};
