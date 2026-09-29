// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Input/OSEEnhancedInputComponent.h"

#include "TATEnhancedInputComponent.generated.h"

enum class EOSEInputHardwareType : uint8;

UCLASS()
class TAT_API UTATEnhancedInputComponent : public UOSEEnhancedInputComponent
{
   GENERATED_BODY()

public:
   /// Returns product of all defined multiplier values
   float GetCumulativeLookSpeedMultiplier(EOSEInputHardwareType hardwareType) const;

   /// Generates a uniquely-identified multiplier value, or overwrites a previously-set one
   void SetLookSpeedMultiplier(FName scalarId, float mouseScalar, float gamepadScalar);

   /// Clears a previously-set multiplier value, so it no longer affects input
   void ClearLookSpeedMultiplier(FName scalarId);

private:
   struct FLookSpeedMultiplierEntry 
   {
      FName Id;

      float MultiplierMouse = 1.f;
      float MultiplierGamepad = 1.f;

      FORCEINLINE bool operator==(FName id) const { return Id == id; }
   };

   /// Collection of tag-identified multiplier values to be incorporated into player look speed.
   TArray<FLookSpeedMultiplierEntry> _lookSpeedMultiplierEntries;
};
