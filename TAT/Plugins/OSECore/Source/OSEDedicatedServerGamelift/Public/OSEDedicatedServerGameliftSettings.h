// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DeveloperSettings.h"

#include "OSEDedicatedServerGameliftSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Dedicated Server Gamelift Settings"))
class OSEDEDICATEDSERVERGAMELIFT_API UOSEDedicatedServerGameliftSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UOSEDedicatedServerGameliftSettings();

   // static access
   static const UOSEDedicatedServerGameliftSettings& Get() { return *GetDefault<UOSEDedicatedServerGameliftSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Dedicated Server|Login")
   FString PlayerSessionIdOptionsKey;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Dedicated Server|Login")
   FString GameSessionIdOptionsKey;
};
