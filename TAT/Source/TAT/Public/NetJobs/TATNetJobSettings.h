// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DeveloperSettings.h"

#include "TATNetJobSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] NetJob Settings"))
class TAT_API UTATNetJobSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // C++ access only
   static const UTATNetJobSettings& Get() { return *GetDefault<UTATNetJobSettings>(); }

   UPROPERTY(Config, EditAnywhere, Category = "Analytics")
   FString AnalyticsURL;
};
