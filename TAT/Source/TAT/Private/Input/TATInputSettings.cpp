// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/TATInputSettings.h"

// ue
#include "Curves/CurveFloat.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInputSettings)

UTATInputDeveloperSettings::UTATInputDeveloperSettings()
{

}

UCurveFloat* UTATInputDeveloperSettings::GetMouseLookSensitivityCurve()
{
   return Cast<UCurveFloat>(MouseLookSensitivityCurve.TryLoad());
}

UCurveFloat* UTATInputDeveloperSettings::GetGamepadLookSensitivityCurve()
{
   return Cast<UCurveFloat>(GamepadLookSensitivityCurve.TryLoad());
}

float FTATMouseKeyboardSettings::GetLookSensitivityValue() const
{
   const UCurveFloat* sensitivityCurve = UTATInputDeveloperSettings::Get()->GetMouseLookSensitivityCurve();

   if (IsValid(sensitivityCurve))
   {
      return sensitivityCurve->GetFloatValue(LookSensitivityTime);
   }

   return 1.0f;
}

float FTATGamepadSettings::GetLookSensitivityValue() const
{
   const UCurveFloat* sensitivityCurve = UTATInputDeveloperSettings::Get()->GetGamepadLookSensitivityCurve();

   if (IsValid(sensitivityCurve))
   {
      return sensitivityCurve->GetFloatValue(LookSensitivityTime);
   }

   return 1.0f;
}
