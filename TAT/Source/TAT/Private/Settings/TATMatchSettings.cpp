// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATMatchSettings.h"

#include "TATGameInstance.h"
#include "AI/Alertness/TATAlertnessGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchSettings)

DEFINE_LOG_CATEGORY_STATIC(LogTATMatchSettings, Log, All);

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DefaultWeatherType, "Weather.Type.MidnightClear")

UTATMatchSettings::UTATMatchSettings()
{
   WeatherType = TAG_DefaultWeatherType;
}

