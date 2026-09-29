// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/TATEnhancedInputComponent.h"

// ose
#include "Input/OSEInputFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnhancedInputComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATEnhancedInputComponent, Log, All);

float UTATEnhancedInputComponent::GetCumulativeLookSpeedMultiplier(EOSEInputHardwareType hardwareType) const
{
   float cumulativeMultiplier = 1.f;
   for (const FLookSpeedMultiplierEntry& entry : _lookSpeedMultiplierEntries)
   {
      switch (hardwareType)
      {
      case EOSEInputHardwareType::Gamepad:
         cumulativeMultiplier *= entry.MultiplierGamepad;
         break;
      case EOSEInputHardwareType::KeyboardMouse:
         cumulativeMultiplier *= entry.MultiplierMouse;
         break;
      default:
         checkNoEntry();
      }
   }
   return cumulativeMultiplier;
}

void UTATEnhancedInputComponent::SetLookSpeedMultiplier(FName scalarId, float mouseScalar, float gamepadScalar)
{
   if (!scalarId.IsValid())
   {
      UE_LOG(LogTATEnhancedInputComponent, Error, TEXT("SetLookSpeedMultiplier() called with invalid scalarId!"));
      return;
   }

   // Find or create scalar entry
   FLookSpeedMultiplierEntry* entry = _lookSpeedMultiplierEntries.FindByKey(scalarId);
   if (!entry)
   {
      const int32 addedIndex = _lookSpeedMultiplierEntries.Add(FLookSpeedMultiplierEntry{ .Id = scalarId });
      entry = &_lookSpeedMultiplierEntries[addedIndex];
   }
   entry->MultiplierMouse = mouseScalar;
   entry->MultiplierGamepad = gamepadScalar;
}

void UTATEnhancedInputComponent::ClearLookSpeedMultiplier(FName scalarId)
{
   if (!scalarId.IsValid())
   {
      UE_LOG(LogTATEnhancedInputComponent, Error, TEXT("ClearLookSpeedMultiplier() called with invalid scalarId!"));
      return;
   }
   _lookSpeedMultiplierEntries.RemoveAllSwap([&](const FLookSpeedMultiplierEntry& entry) { return entry.Id == scalarId; });
}
