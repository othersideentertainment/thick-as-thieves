// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/OSEEnhancedInputTriggers.h"

// ue4
#include "EnhancedPlayerInput.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEEnhancedInputTriggers)

//---------------------------------------------------------------------------------------
// UOSEInputTriggerDirection
//---------------------------------------------------------------------------------------

UOSEInputTriggerDirection::UOSEInputTriggerDirection()
   : Super()
   , UpDirection(0.0f, 1.0f)
{

}

ETriggerState UOSEInputTriggerDirection::UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime)
{
   if (modifiedValue.IsNonZero())
   {
      const bool isTriggered = _GetIsTriggeredFromValue(modifiedValue);
      return isTriggered ? ETriggerState::Triggered : ETriggerState::None;
   }
   else
   {
      return TriggersAtZero ? ETriggerState::Triggered : ETriggerState::None;
   }
}

bool UOSEInputTriggerDirection::_GetIsTriggeredFromValue(const FInputActionValue& value) const
{
   const float x = value[0];
   const float y = value[1];
   FVector2D direction = { x, y };
   direction.Normalize();
   
   const float multiplier = (x >= 0.0f) ? 1.0f : -1.0f;
   const float dot = direction | UpDirection;
   float rotDeg = (UKismetMathLibrary::DegAcos(dot) * multiplier);

   const bool isTriggered = FMath::Abs(rotDeg) <= MaxAngleDistance;
   
   // leaving this around for debugging...
   //UE_LOG(LogTemp, Log, TEXT("%s: %.02f degrees : %s"), *GetName(), rotDeg, isTriggered ? TEXT("TRUE") : TEXT("FALSE"));

   return isTriggered;
}

ETriggerState UOSEInputTriggerDoublePress::UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime)
{
   ETriggerState triggerState = ETriggerState::None;
   const float currentTime = UGameplayStatics::GetRealTimeSeconds(playerInput);

   const bool pressedThisFrame = IsActuated(modifiedValue) && !IsActuated(_lastInputActionValue);
   if (pressedThisFrame)
   {
      const bool isDoublePress = currentTime - _timeLastPressed < DoublePressThresholdSeconds;
      if (isDoublePress)
      {
         triggerState = ETriggerState::Triggered;

         // Reset the time-last-pressed to prevent a subsequent third click from triggering another double-click event.
         // Double clicks must always come in pairs of 2 clicks.
         _timeLastPressed = 0.f;
      }
      else
      {
         _timeLastPressed = currentTime;
      }
   }

   _lastInputActionValue = modifiedValue;
   return triggerState;
}

ETriggerState UOSEModifierKeyTrigger::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime)
{
   if (IsActuated(ModifiedValue) && PlayerInput->IsPressed(ModifierKey))
   {
      return ETriggerState::Triggered;
   }

   return ETriggerState::None;
}

#if WITH_EDITOR
void UOSEModifierKeyTrigger::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
   Super::PostEditChangeProperty(PropertyChangedEvent);

   if (PropertyChangedEvent.GetPropertyName().IsEqual(GET_MEMBER_NAME_CHECKED(UOSEModifierKeyTrigger, ModifierKey)))
   {
      if (!ModifierKey.IsModifierKey())
      {
         ModifierKey = EKeys::LeftControl;
      }
   }
}
#endif
