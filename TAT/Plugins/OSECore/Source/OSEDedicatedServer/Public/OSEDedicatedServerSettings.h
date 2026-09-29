// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h"

// ue4
#include "Engine/DeveloperSettings.h"

#include "OSEDedicatedServerSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Dedicated Server Settings"))
class OSEDEDICATEDSERVER_API UOSEDedicatedServerSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UOSEDedicatedServerSettings();

   // static access
   static const UOSEDedicatedServerSettings& Get() { return *GetDefault<UOSEDedicatedServerSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Dedicated Server|Init")
   TSoftClassPtr<UOSEDedicatedServerManagerBase> ServerManagerClass;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "Dedicated Server|Init")
   TSoftClassPtr<UOSEDedicatedServerManagerBase> DevelopmentServerManagerClass;
};
