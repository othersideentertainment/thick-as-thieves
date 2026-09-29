// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/TATEnhancedInputModifiers.h"

// ue4
#include "EnhancedPlayerInput.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Input/OSEInputFunctionLibrary.h"
#include "Input/OSEInputSettings.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Player/OSEPlayerController.h"

// tat
#include "Input/TATEnhancedInputComponent.h"
#include "Tools/TATCustomSensitivityToolInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnhancedInputModifiers)

DEFINE_LOG_CATEGORY_STATIC(LogTATInputModifiers, Log, All);

namespace TATEnhancedInputModifiersHelpers
{
   const UTATEnhancedInputComponent* GetEnhancedInputComponent(const UEnhancedPlayerInput* playerInput)
   {
      check(playerInput);
      const APlayerController* controller = playerInput->GetOuterAPlayerController();
      check(controller);
      if (const APawn* pawn = controller->GetPawn())
      {
         return Cast<UTATEnhancedInputComponent>(pawn->InputComponent);
      }
      return nullptr;
   }
}

//------------------------------------------------------------------------------------------------------------------------
// UTATAnalogDrawingInputModifier
//------------------------------------------------------------------------------------------------------------------------
FInputActionValue UTATAnalogDrawingInputModifier::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   // Only modify values coming from a gamepad
   const AOSEPlayerController* playerController = CastChecked<AOSEPlayerController>(playerInput->GetOuterAPlayerController());
   if (playerController->GetCurrentInputHardwareType() != EOSEInputHardwareType::Gamepad)
   {
      return currentValue;
   }

   // Only modify 2D analog values
   if (currentValue.GetValueType() != EInputActionValueType::Axis2D)
   {
      return currentValue;
   }

   const AOSECharacterBase* character = Cast<AOSECharacterBase>(playerController->GetPawn());
   if (!IsValid(character))
   {
      return currentValue;
   }

   const TScriptInterface<IToolSetInterface> toolSet = character->GetToolSetInterface();
   if (!toolSet)
   {
      return currentValue;
   }

   // If no tool equipped or current tool doesn't implement sensitivity interface, return value unchanged
   const UToolComponent* currentTool = toolSet->GetCurrentTool();
   if (!IsValid(currentTool) || !currentTool->GetClass()->ImplementsInterface(UTATCustomSensitivityTool::StaticClass()))
   {
      return currentValue;
   }

   // Apply scalar while actively using tool
   FInputActionValue responseValue = currentValue;
   const bool isActivelyUsingTool = ITATCustomSensitivityTool::Execute_GetIsToolActive(currentTool);
   if (isActivelyUsingTool)
   {
      responseValue *= SensitivityScalarWhileToolActive;
   }

   // Check for sensitivity level, applying additional scalar if found
   const int currentSensitivityLevel = ITATCustomSensitivityTool::Execute_GetCurrentGamepadSensitivityLevel(currentTool);
   if (const float* sensitivityScalar = SensitivityScalarMap.Find(currentSensitivityLevel))
   {
      return responseValue * *sensitivityScalar;
   }

   UE_LOG(LogTATInputModifiers, Warning, TEXT("UTATAnalogDrawingInputModifier - tool %s implementing ITATCustomSensitivityTool returned an unexpected sensitivity value of %d not found in SensitivityScalarMap!")
      , *currentTool->GetName()
      , currentSensitivityLevel);
   return responseValue;
}

FInputActionValue UTATEnhancedInputModifierMouseLookSensitivityMultiplier::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   // Don't try and scale bools
   if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
   {
      if (!_playerInputComponent)
      {
         _playerInputComponent = TATEnhancedInputModifiersHelpers::GetEnhancedInputComponent(playerInput);
      }
      UE_CLOG(_playerInputComponent == nullptr, LogTATInputModifiers, Error, TEXT("UTATEnhancedInputModifierMouseLookSensitivityMultiplier applied to player without a UTATEnhancedInputComponent!"));
      const float scalarValue = _playerInputComponent ? _playerInputComponent->GetCumulativeLookSpeedMultiplier(EOSEInputHardwareType::Gamepad) : 1.f;
      return currentValue.Get<FVector>() * scalarValue;
   }
   return currentValue;
}

FInputActionValue UTATEnhancedInputModifierGamepadLookSensitivityMultiplier::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   // Don't try and scale bools
   if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
   {
      if (!_playerInputComponent)
      {
         _playerInputComponent = TATEnhancedInputModifiersHelpers::GetEnhancedInputComponent(playerInput);
      }
      UE_CLOG(_playerInputComponent == nullptr, LogTATInputModifiers, Error, TEXT("UTATEnhancedInputModifierGamepadLookSensitivityMultiplier applied to player without a UTATEnhancedInputComponent!"));
      const float scalarValue = _playerInputComponent ? _playerInputComponent->GetCumulativeLookSpeedMultiplier(EOSEInputHardwareType::KeyboardMouse) : 1.f;
      return currentValue.Get<FVector>() * scalarValue;
   }
   return currentValue;
}
