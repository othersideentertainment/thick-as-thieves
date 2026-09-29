// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Lockpicking/TATLockpickingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockpickingSettings)
DEFINE_LOG_CATEGORY_STATIC(LogTATLockpickingSettings, Log, All);


#define LOCTEXT_NAMESPACE "TATLockpickingSettings"

UTATLockpickingSettings::UTATLockpickingSettings()
{
   InteractPrompts.LockpickPrompt = LOCTEXT("PickLockPrompt", "Pick Lock");
   InteractPrompts.LockLevelPrompt = LOCTEXT("LockLevelMessage", "Lock Level {LockLevel}");
   InteractPrompts.UnlockWithKeyPrompt = LOCTEXT("KeyUnlockPrompt", "Unlock with Key");
   InteractPrompts.LockWithKeyPrompt = LOCTEXT("KeyLockPrompt", "Lock with Key");
   InteractPrompts.LockWithoutKeyPrompt = LOCTEXT("LockPrompt", "Lock");
}

#undef LOCTEXT_NAMESPACE

