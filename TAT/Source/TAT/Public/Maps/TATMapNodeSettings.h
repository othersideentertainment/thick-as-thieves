// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//tat 
#include "GameFramework/TATDifficulty.h"

// ue
#include "CoreMinimal.h"

#include "TATMapNodeSettings.generated.h"

UENUM(BlueprintType)
enum class ETATMatchMode : uint8
{
   Coop = 0,
   Solo = 1,
   Offline = 2,
   ListenServer = 3, //< Currently stubbed in to allow explicit selection on map. Does not necessarily imply only way to launch listen servers
   ListenServerPublic = 4, //< Hosting a public listen server
};

// A struct for defining the settings in a Map Node
USTRUCT(BlueprintType)
struct TAT_API FTATMapNodeSettings
{
   GENERATED_BODY()
public:
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MapNodeSettings)
   TSoftObjectPtr<UWorld> Map;
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = MapNodeSettings)
   FString MapName;
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = MapNodeSettings)
   FText MapDisplayName;
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = MapNodeSettings)
   ETATDifficulty Difficulty = ETATDifficulty::Easy;
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = MapNodeSettings)
   ETATMatchMode MatchMode = ETATMatchMode::Offline;
};
