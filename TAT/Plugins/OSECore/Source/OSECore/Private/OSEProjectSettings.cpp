// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEProjectSettings.h"

// ose
#include "Character/OSECharacterMovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEProjectSettings)

FGameplayTag UOSEProjectSettings::GetTagForMovementMode(EMovementMode mode, uint8 customMode) const
{
   if (mode == MOVE_Custom)
   {
      return CustomMovementModeTags.FindRef(OSE::MovementUtils::GetCustomMovementType(mode, customMode));
   }
   else
   {
      return MovementModeTags.FindRef(mode);
   }
}

