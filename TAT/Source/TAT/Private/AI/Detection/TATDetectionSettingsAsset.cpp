// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/Detection/TATDetectionSettingsAsset.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDetectionSettingsAsset)

FTATDetectionSettings::FTATDetectionSettings()
{
   for (int idx = 0; idx < static_cast<int>(EAlertnessLevel::MAX); ++idx)
   {
      RampSettings.FindOrAdd(static_cast<EAlertnessLevel>(idx));
   }
}

