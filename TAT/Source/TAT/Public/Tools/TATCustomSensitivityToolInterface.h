// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "UObject/Interface.h"

#include "TATCustomSensitivityToolInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Tools")
class UTATCustomSensitivityTool : public UInterface
{
   GENERATED_BODY()
};

//------------------------------------------------------------------------------------------------------------------------
// ITATCustomSensitivityTool
// - Interface to be implemented by tools which require context-sensitive input scaling
//     - (i.e. various "sensitivity" levels that scale inversely with zooming in on a sniper scope)
// 
// - Implementing tools should assign a UTATAnalogDrawingInputModifier to input actions triggering behavior
//   (see IMC_Monocular for an example).
//------------------------------------------------------------------------------------------------------------------------
class TAT_API ITATCustomSensitivityTool
{
   GENERATED_BODY()

public:
   // Returns an index indicating the current sensitivity level. For tools that allow zooming in/out (or otherwise require discrete sensitivity levels), 0 indicates the "default" sensitivity.
   UFUNCTION(BlueprintNativeEvent, Category = "Tools|Custom Gamepad Sensetivity")
   int GetCurrentGamepadSensitivityLevel() const;
   int GetCurrentGamepadSensitivityLevel_Implementation() const { return -1; }

   // Indicates whether tool is being "actively" used (i.e. drawing, shooting, scanning) rather than just equipped.
   UFUNCTION(BlueprintNativeEvent, Category = "Tools|Custom Gamepad Sensetivity")
   bool GetIsToolActive() const;
   bool GetIsToolActive_Implementation() const { return false; }
};
