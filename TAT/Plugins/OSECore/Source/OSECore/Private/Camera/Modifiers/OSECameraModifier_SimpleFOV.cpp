// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/Modifiers/OSECameraModifier_SimpleFOV.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraModifier_SimpleFOV)


UOSECameraModifier_SimpleFOV::UOSECameraModifier_SimpleFOV()
{
   _interpolationSpeed = 10;
}

// Allows modifying the camera in native code.
void UOSECameraModifier_SimpleFOV::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
   _currentFovBoost = FMath::FInterpTo(_currentFovBoost, _targetFovBoost, DeltaTime, _interpolationSpeed);
   NewFOV += _currentFovBoost;
}

// static
void UOSECameraModifier_SimpleFOV::TryApplySimpleFovBoost(UActorComponent* component, float amount, bool isAdd /*= true*/)
{
   auto modifier = FindModifierForComponent<UOSECameraModifier_SimpleFOV>(component);
   if (modifier)
   {
      if (isAdd)
      {
         modifier->AddFovBoost(amount);
      }
      else
      {
         modifier->RemoveFovBoost(amount);
      }
   }
}

void UOSECameraModifier_SimpleFOV::TryApplyInstantFovBoost(UActorComponent* component, float amount)
{
   auto modifier = FindModifierForComponent<UOSECameraModifier_SimpleFOV>(component);
   if (modifier)
   {
      modifier->_currentFovBoost += amount;
   }
}

