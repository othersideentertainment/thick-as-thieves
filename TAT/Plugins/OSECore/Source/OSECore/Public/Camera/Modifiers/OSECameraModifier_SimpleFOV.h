// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Camera/Modifiers/OSECameraModifier.h"
#include "OSECameraModifier_SimpleFOV.generated.h"

// Simple field of view modifier
UCLASS(BlueprintType, Blueprintable)
class OSECORE_API UOSECameraModifier_SimpleFOV : public UOSECameraModifier
{
   GENERATED_BODY()

public:

   UOSECameraModifier_SimpleFOV();

   UFUNCTION(BlueprintCallable)
   void AddFovBoost(float amount) { _targetFovBoost += amount; }

   UFUNCTION(BlueprintCallable)
   void RemoveFovBoost(float amount) { _targetFovBoost -= amount; }

   UFUNCTION(BlueprintCallable, Category = "CameraModifier|OSE")
   static void TryApplySimpleFovBoost(UActorComponent* component, float amount, bool isAdd = true);

   UFUNCTION(BlueprintCallable, Category = "CameraModifier|OSE")
   static void TryApplyInstantFovBoost(UActorComponent* component, float amount);

protected:

   /// Allows modifying the camera in native code.
   virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;


private:

   /// Parameters to control the velocity-based field of view boost
   UPROPERTY(EditAnywhere, Category = "CameraModifier", meta = (AllowPrivateAccess = "true"))
   float _interpolationSpeed;

   UPROPERTY(Transient)
   float _currentFovBoost;

   UPROPERTY(Transient)
   float _targetFovBoost;
};
