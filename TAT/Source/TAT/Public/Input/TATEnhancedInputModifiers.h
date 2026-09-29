// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATInputSettings.h"

// ue4
#include "CoreMinimal.h"
#include "InputModifiers.h"

#include "TATEnhancedInputModifiers.generated.h"

class UTATEnhancedInputComponent;

//------------------------------------------------------------------------------------------------------------------------
// UTATAnalogDrawingInputModifier
// - NOTE: Intended to only be assigned to a Gamepad Thumbstick input action (ideally one controlling where the player looks).
// - Intended for use with tools implementing ITATCustomSensitivityTool.
// 
// - Ideal use cases:
//     - Dampening analog input when zoomed in with binoculars
//     - Scaling input down while drawing on a surface, shooting, scanning, etc. with analog stick (to increase precision)
//------------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[TAT] Gamepad Analog Drawing Dampener"))
class UTATAnalogDrawingInputModifier final : public UInputModifier
{
   GENERATED_BODY()

public:
   // Mapping of gamepad sensitivity level (returned by ITATCustomSensitivityTool::GetCurrentGamepadSensitivityLevel()) -> applied sensitivity scalar.
   UPROPERTY(EditInstanceOnly, Category = Settings)
   TMap<int, float> SensitivityScalarMap;

   // Sensitivity scalar applied while tool is being "actively" used (i.e. drawing, shooting, scanning) rather than just equipped.
   UPROPERTY(EditInstanceOnly, Category = Settings, meta = (ClampMin = 0.f, UIMin = 0.f, UIMax = 2.0f))
   float SensitivityScalarWhileToolActive = 0.5f;

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//------------------------------------------------------------------------------------------------------------------------
// UTATEnhancedInputModifierMouseLookSensitivity
// - Multiplies mouse look sensitivity by the player defined sensitivity setting.
//------------------------------------------------------------------------------------------------------------------------
UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[TAT] Setting: Mouse Look Sensitivity"))
class UTATEnhancedInputModifierMouseLookSensitivity final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      // Don't try and scale bools
      if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
      {
         return currentValue.Get<FVector>() * UTATInputSettings::GetTATInputSettings()->MouseKeyboard.GetLookSensitivityValue();
      }
      return currentValue;
   }
};

//------------------------------------------------------------------------------------------------------------------------
// UTATEnhancedInputModifierMouseLookSensitivityMultiplier
// - Multiplies UTATEnhancedInputComponent's cumulative look-speed-multipliers into the input.
// - Useful if you want to be able to dynamically scale the look speed at runtime, on top of the player-defined 
//   sensitivity settings
//------------------------------------------------------------------------------------------------------------------------
UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[TAT] Setting: Mouse Look Sensitivity Multiplier"))
class UTATEnhancedInputModifierMouseLookSensitivityMultiplier final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;

   UPROPERTY(Transient)
   TObjectPtr<const UTATEnhancedInputComponent> _playerInputComponent = nullptr;
};

//------------------------------------------------------------------------------------------------------------------------
// UTATEnhancedInputModifierGamepadLookSensitivity
// - Multiplies gamepad look sensitivity by the player defined sensitivity setting.
//------------------------------------------------------------------------------------------------------------------------
UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[TAT] Setting: Gamepad Look Sensitivity"))
class UTATEnhancedInputModifierGamepadLookSensitivity final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      // Don't try and scale bools
      if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
      {
         return currentValue.Get<FVector>() * UTATInputSettings::GetTATInputSettings()->Gamepad.GetLookSensitivityValue();
      }
      return currentValue;
   }
};

//------------------------------------------------------------------------------------------------------------------------
// UTATEnhancedInputModifierGamepadLookSensitivityMultiplier
// - Multiplies UTATEnhancedInputComponent's cumulative look-speed-multipliers into the input.
// - Useful if you want to be able to dynamically scale the look speed at runtime, on top of the player-defined 
//   sensitivity settings
//------------------------------------------------------------------------------------------------------------------------
UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[TAT] Setting: Gamepad Look Sensitivity Multiplier"))
class UTATEnhancedInputModifierGamepadLookSensitivityMultiplier final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;

   UPROPERTY(Transient)
   TObjectPtr<const UTATEnhancedInputComponent> _playerInputComponent = nullptr;
};
