// (c) 2021-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue5
#include "Engine/DeveloperSettings.h"

#include "TATSquadAlarmStationSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Squad Alarm Station Settings"))
class TAT_API UTATSquadAlarmStationSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // for bp
   //UFUNCTION(BlueprintPure, Category = "TAT Squad Alarm Station Settings")
   //static UTATSquadAlarmStationSettings* GetOSEProjectSettings() { return GetMutableDefault<UTATSquadAlarmStationSettings>(); }

   // for C++
   static const UTATSquadAlarmStationSettings& Get() { return *GetDefault<UTATSquadAlarmStationSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Alarm")
   FOSEAITokenInfo SquadAlarmStationToken;

   /// How long until the alarm is re-useable again.
   UPROPERTY(Config, EditAnywhere, Category = "Alarm Settings")
   float AlarmCooldown = 10.0;

   /// How long to hold to trigger the alarm.
   UPROPERTY(Config, EditAnywhere, Category = "Alarm Settings")
   float HoldDuration = 1.0f;
};
