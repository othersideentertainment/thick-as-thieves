// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "Kismet/BlueprintAsyncActionBase.h"

#include "TATWaitForLocalPlayerState.generated.h"

class ATATPlayerState;

UCLASS(MinimalAPI)
class UTATWaitForLocalPlayerState : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitComplete, ATATPlayerState*, localPlayerState);

   UPROPERTY(BlueprintAssignable)
   FWaitComplete OnFoundLocalPlayerState;

   UPROPERTY(BlueprintAssignable)
   FWaitComplete OnFailure;

   /// Get the local player state, which may not be ready yet. If it's not ready yet, delay until it is.
   /// This is the same as calling GetLocalTATPlayerState in a loop until it returns non-null.
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT", meta = (BlueprintInternalUseOnly = "true", WorldContext = "contextObj"))
   static UTATWaitForLocalPlayerState* WaitForLocalTATPlayerState(UObject* contextObj, float maxWaitTimeSeconds = 4.0f);

   // from UBlueprintAsyncActionBase
   virtual void Activate() override;
   virtual void SetReadyToDestroy() override;

private:
   UFUNCTION()
   void _Update();

private:
   UPROPERTY(Transient)
   TObjectPtr<UObject> _contextObj;

   float _maxWaitTimeSeconds = 4.0f;

   double _realStartTimeSeconds = 0.0;

   double _lastWarningTimeSeconds = 0.0;

   FTimerHandle _timerHandle;
};
