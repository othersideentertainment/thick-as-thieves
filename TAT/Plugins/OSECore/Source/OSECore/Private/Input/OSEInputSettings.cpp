// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/OSEInputSettings.h"

#include "Input/OSEInputFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEInputSettings)

//---------------------------------------------------------------------------------------
// FOSEGamepadInputSettings
//---------------------------------------------------------------------------------------

float FOSEGamepadInputSettings::GetMoveSensitivityValue() const
{
   const TMap<EOSEInputSensitivityType, float>& moveSensitivityMap = UOSEInputDeveloperSettings::Get().GamepadMoveSensitivity;
   if (moveSensitivityMap.Contains(MoveSensitivity))
   {
      return moveSensitivityMap[MoveSensitivity];
   }
   // ASSUMPTION: 1.0f is the default
   return 1.0f;
}

float FOSEGamepadInputSettings::GetLookSensitivityValue() const
{
   const TMap<EOSEInputSensitivityType, float>& lookSensitivityMap = UOSEInputDeveloperSettings::Get().GamepadLookSensitivity;
   if (lookSensitivityMap.Contains(LookSensitivity))
   {
      return lookSensitivityMap[LookSensitivity];
   }
   // ASSUMPTION: 1.0f is the default
   return 1.0f;
}

//---------------------------------------------------------------------------------------
// FOSEGamepadMouseKeyboardSettings
//---------------------------------------------------------------------------------------

float FOSEGamepadMouseKeyboardSettings::GetLookSensitivityValue() const
{
   const TMap<EOSEInputSensitivityType, float>& lookSensitivityMap = UOSEInputDeveloperSettings::Get().MouseLookSensitivity;
   if (lookSensitivityMap.Contains(LookSensitivity))
   {
      return lookSensitivityMap[LookSensitivity];
   }
   // ASSUMPTION: 1.0f is the default
   return 1.0f;
}

//---------------------------------------------------------------------------------------
// UOSEInputDeveloperSettings
//---------------------------------------------------------------------------------------

UOSEInputDeveloperSettings::UOSEInputDeveloperSettings()
{
   for(int idx = 0; idx < int(EOSEInputSensitivityType::MAX); ++idx)
   {
      MouseLookSensitivity.Add(EOSEInputSensitivityType(idx), 1.0f);
   }
   for (int idx = 0; idx < int(EOSEInputSensitivityType::MAX); ++idx)
   {
      GamepadLookSensitivity.Add(EOSEInputSensitivityType(idx), 1.0f);
   }
   for (int idx = 0; idx < int(EOSEInputSensitivityType::MAX); ++idx)
   {
      GamepadMoveSensitivity.Add(EOSEInputSensitivityType(idx), 1.0f);
   }
}

bool UOSEInputSettings::IsSprintToggleSetForInputHardwareType(EOSEInputHardwareType inputHardwareType) const
{
   switch (inputHardwareType)
   {
      case EOSEInputHardwareType::Gamepad: return Gamepad.SprintToggle;
      case EOSEInputHardwareType::KeyboardMouse: return MouseKeyboard.SprintToggle;
      default:
         checkNoEntry();
         return false;
   }
}

