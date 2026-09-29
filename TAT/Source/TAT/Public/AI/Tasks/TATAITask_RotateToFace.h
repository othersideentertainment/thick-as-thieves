// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Tasks/AITask.h"

#include "TATAITask_RotateToFace.generated.h"

class AAIController;
class IGameplayTaskOwnerInterface;

DECLARE_LOG_CATEGORY_EXTERN(LogTATAITask_RotateToFace, Log, All);

UCLASS()
class TAT_API UTATAITask_RotateToFace : public UAITask
{
	GENERATED_BODY()

public:
   UTATAITask_RotateToFace(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // Use to have AI rotate towards a specific point in world-space
   UFUNCTION(BlueprintCallable, Category = "TAT|AI|Tasks", meta = (DefaultToSelf = "Controller", BlueprintInternalUseOnly = "true"))
   static UTATAITask_RotateToFace* RotateToFaceLocation(AAIController* controller, FVector location, float precision = 10.0f);
   static UTATAITask_RotateToFace* RotateToFaceLocation(AAIController* controller, FVector location, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision = 10.0f);

   // Use to have AI rotate towards a world-space direction, relative to the pawn's location, offset by the eye Z-location
   UFUNCTION(BlueprintCallable, Category = "TAT|AI|Tasks", meta = (DefaultToSelf = "Controller", BlueprintInternalUseOnly = "true"))
   static UTATAITask_RotateToFace* RotateToFaceDirection(AAIController* controller, FRotator direction, float precision = 10.0f);
   static UTATAITask_RotateToFace* RotateToFaceDirection(AAIController* controller, FRotator direction, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision = 10.0f);

   // from UGameplayTask
   virtual void TickTask(float deltaTime) override;

   virtual void OnDestroy(bool bInOwnerFinished) override;
   bool WasRotationSuccessful() const { return WasSuccessful; }

protected:
   UPROPERTY(BlueprintAssignable)
   FGenericGameplayTaskDelegate OnSucceeded;

   UPROPERTY(BlueprintAssignable)
   FGenericGameplayTaskDelegate OnFailed;

   // cached to compare to focal point to ensure something else
   // hasn't changed the focal point
   FVector LocationToFace;

   // cached Precision tangent value
   float PrecisionDot = 0.0f;

   // cache the result of this task, only valid when task is finished
   bool WasSuccessful = false;

   static UTATAITask_RotateToFace* RotateToFaceInitialize(AAIController* controller, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision = 10.0f);
	
};
