// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"

// tat
#include "TATMatchSettingsBase.h"
#include "TATMatchSettingsPropertyDef.h"
#include "GameFramework/TATDifficulty.h"
#include "Loot/TATLootTypes.h"

#include "TATMatchSettings.generated.h"

UENUM(BlueprintType)
enum class ERespawnDuration : uint8
{
   Zero = 0          UMETA(DisplayName = "0"),
   Five = 5          UMETA(DisplayName = "5"),
   Ten = 10          UMETA(DisplayName = "10"),
   Fifteen = 15      UMETA(DisplayName = "15"),
   Twenty = 20       UMETA(DisplayName = "20"),
   Thirty = 30       UMETA(DisplayName = "30"),
   FourtyFive = 45   UMETA(DisplayName = "45"),
   Sixty = 60        UMETA(DisplayName = "60"),
   
};

UENUM(BlueprintType)
enum class ERespawnCount : uint8
{
   Zero = 0          UMETA(DisplayName = "0"),
   One = 1           UMETA(DisplayName = "1"),
   Two = 2           UMETA(DisplayName = "2"),
   Three = 3         UMETA(DisplayName = "3"),
   Unlimited = 255   UMETA(DisplayName = "Unlimited")
};

UENUM(BlueprintType)
enum class ERespawnHealthPctRecovered : uint8
{
   None = 0             UMETA(DisplayName = "0% - Don't use this"),
   OneQuarter = 25      UMETA(DisplayName = "25%"),
   Half = 50            UMETA(DisplayName = "50%"),
   ThreeQuarters = 75   UMETA(DisplayName = "75%"),
   Full = 100           UMETA(DisplayName = "100%")
};

///
/// Player-facing match settings
///
UCLASS(BlueprintType, Blueprintable)
class TAT_API UTATMatchSettings : public UTATMatchSettingsBase
{
   GENERATED_BODY()

   static constexpr bool kRandomizeDefaultWeatherType = true;
   static constexpr bool kRandomizeDefaultMatchQuest = true;

public:
   UTATMatchSettings();

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   FGameplayTag WeatherType;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef WeatherType_Metadata = MakePropertyMetadata(
      TEXT("Weather Type"),
      TEXT("The weather conditions during the match"),
      FName("WeatherType"),
      kRandomizeDefaultWeatherType);

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   bool ThiefVisionFootstepsWhileWalking = true;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef ThiefVisionFootstepsWhileWalking_Metadata = MakePropertyMetadata(
      TEXT("Footstep Indicators (Walking)"),
      TEXT("Should thief vision indicators be generated when walking?"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   bool ThiefVisionFootstepsWhileCrouching = false;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef ThiefVisionFootstepsWhileCrouching_Metadata = MakePropertyMetadata(
      TEXT("Footstep Indicators (Crouching)"),
      TEXT("Should thief vision indicators be generated when moving while crouched?"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   bool DisplayAreasWithPlayersThatHaveQuestLoot = true;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef DisplayAreasWithPlayersThatHaveQuestLoot_Metadata = MakePropertyMetadata(
      TEXT("Display Areas With Players That Have Quest Loot"),
      TEXT("Should players with Quest Loot be exposed on the map (within a general area)?"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   bool DisplaySealingStashes = true;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef DisplaySealingStashes_Metadata = MakePropertyMetadata(
      TEXT("Display Sealing Stashes on Map"),
      TEXT("Should Loot Stashes that are currently being sealed be displayed on the map?"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings|Respawn")
   ERespawnDuration RespawnDuration;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef RespawnDuration_Metadata = MakePropertyMetadata(
      TEXT("Respawn Duration"),
      TEXT("The amount of time it takes to respawn after getting KO'd in a match"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings|Respawn")
   ERespawnCount RespawnCount;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef RespawnCount_Metadata = MakePropertyMetadata(
      TEXT("Respawn Count"),
      TEXT("The amount of times a player is able to respawn after getting KO'd in a match"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings|Respawn")
   ERespawnHealthPctRecovered RespawnHealthPctRecovered;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef RespawnHealthPctRecovered_Metadata = MakePropertyMetadata(
      TEXT("Respawn Health Percent Recovered"),
      TEXT("The percentage of health restored after a player respans"));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="TAT Match Settings|Respawn")
   bool SpectateOnRespawn = true;
   FTATMatchSettingsPropertyDef SpectateOnRespawn_Metadata = MakePropertyMetadata(
      TEXT("Spectate on Respawn"),
      TEXT("Whether players should go into spectator mode while waiting to respawn."));

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   ETATLootDropSettings LootDropSettings;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef LootDropSettings_Metadata = MakePropertyMetadata(
      TEXT("Loot Drop Rule"),
      TEXT("Determine the kind of loot that is dropped on KO"));

   // NOTE: Does not account for editor overrides, so don't directly use in match
   //       Instead use `TATDifficulty::GetDifficultyForMatch`. (Revisit applying
   //       this setting if that become burdensome)
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings")
   ETATDifficulty Difficulty = ETATDifficulty::Normal;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef Difficulty_Metadata = MakePropertyMetadata(
      TEXT("Difficulty"),
      TEXT("The difficulty of the match"));

   /// NOTE: This is not the source of truth for active missions! When reading from this during a match, use the TATActiveQuestSubsystem + friends, as this can be overridden by editor-prefs (see TATActiveQuestSubsystem::Initialize())
   /// TODO: consider moving this somewhere more solid
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT Match Settings", Meta = (Categories = "Mission"))
   FGameplayTag Mission;
   UPROPERTY(Transient)
   FTATMatchSettingsPropertyDef Mission_Metadata = MakePropertyMetadata(
      TEXT("Mission"),
      TEXT("The mission to play on the selected map"),
      FName("Mission"),
      kRandomizeDefaultMatchQuest);

};
