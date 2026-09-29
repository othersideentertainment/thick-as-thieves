// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Upgrades/TATUpgradeType.h"

// ose
#include "OSEGenericGraphNode.h"

// ue5
#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "Internationalization/Text.h"

#include "TATProgressionSettings.generated.h"

struct FScalableFloat;
struct FTATPlayerStatInfo;
class UTATUpgradeCurrencyMetadataAsset;

USTRUCT(BlueprintType)
struct TAT_API FTATUpgradeEditorNodeStyle
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Editor Node Style")
   EOSEGenericGraphNodeStyle Style = EOSEGenericGraphNodeStyle::Flat;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Editor Node Style")
   FLinearColor Color = FLinearColor::White;;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Progression Settings"))
class TAT_API UTATProgressionSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATProgressionSettings();

   static const UTATProgressionSettings& Get();

   UFUNCTION(BlueprintPure, DisplayName = "Get Progression Settings", Category = "Progression Settings", meta = (CompactNodeTitle = "Progression Settings"))
   static const UTATProgressionSettings* BP_GetProgressionSettings();

   UFUNCTION(BlueprintCallable, Category = "Progression", meta=(Categories = "PlayerStats", ExpandBoolAsExecs = "ReturnValue"))
   static bool GetPlayerStatInfo(FGameplayTag statTag, FTATPlayerStatInfo& statInfo);

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Upgrades")
   TSoftObjectPtr<UTATUpgradeCurrencyMetadataAsset> UpgradeCurrencyMetadata;

   /// [EDITOR-ONLY] The upgrade graph editor node style to use for each progression type
   UPROPERTY(Config, EditDefaultsOnly, Category = "Upgrades|Editor", meta = (ArraySizeEnum = "/Script/TAT.ETATUpgradeProgressionType"))
   FTATUpgradeEditorNodeStyle UpgradeEditorProgressionNodeStyles[static_cast<uint32>(ETATUpgradeProgressionType::MAX)] = {};

   UPROPERTY(Config, EditAnywhere, Category = "Progression", meta=(RequiredAssetDataTags = "RowStructure=/Script/TAT.TATPlayerStatInfo"))
   TSoftObjectPtr<UDataTable> PlayerStatDataTable;

   ////////////////////////////////
   // Player XP
   ////////////////////////////////

   UPROPERTY(Config, EditAnywhere, Category = "XP", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATXPGainInfo", DisplayName = "XP Data Table"))
   TSoftObjectPtr<UDataTable> XPDataTable;

   UPROPERTY(Config, EditAnywhere, Category = "XP", DisplayName = "XP To Level")
   FScalableFloat XPToLevel;
   
   UFUNCTION(BlueprintCallable)
   static int32 GetXPForNextLevel(int level);
   
   UFUNCTION(BlueprintCallable)
   static int32 GetTotalXPRequiredForLevel(int level);
   
   // Experience needed for level up
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   int32 LevelUpXP = 500;
   // the last level that the player can reach
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   int32 MaximumLevel = 25;
   // XP gain per minute playing (time in game)
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "XP Per Minute"))
   float XPPerMinute = 5.5f;
   // maximum experience that the player can gain for time in game
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float MaximumTimeInGameXP = 25.f;
   // Win is defined as reaching the exit in the time
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float WinXP = 60.f;
   // Completing the primary mission of the map
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float MissionXP = 13.f;
   // Completing the additional tasks in the map
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float ContractsXP = 13.f;
   // Queueing in open matchmaking (“play anything”)
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float OpenQueueXP = 20.f;
   // XP for every major loot stolen
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float MajorLootXP = 15.f;
   // XP for every minor loot stolen
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP")
   float MinorLootXP = 0.5f;
   // In case that plays coop, both players get the XP of the loot, but the looting player gets a bonus on that XP
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "Looting Player XP Multiplier"))
   float LootingPlayerXPMultiplier = 1.1f;
   // multiplies all the XP gained when finished the level at easy difficulty
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "Easy Difficulty XP Multiplier"))
   float EasyDifficultyXPMultiplier = 1.f;
   // multiplies all the XP gained when finished the level at normal difficulty
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "Normal Difficulty XP Multiplier"))
   float NormalDifficultyXPMultiplier = 1.25f;
   // multiplies all the XP gained when finished the level at hard difficulty
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "Hard Difficulty XP Multiplier"))
   float HardDifficultyXPMultiplier = 1.5f;
   // multiplies all the XP gained when finished the level if played on coop
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (DisplayName = "Coop XP Multiplier"))
   float CoopXPMultiplier = 1.1f;

   // XP categories that level-up xp is added to in the FTUE
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP|FTUE", meta = (Categories="XP"))
   TArray<FGameplayTag> FtueLevelUpXPCategories;
   // If true, the player will the amount to level up on top of regular xp in the FTUE
   // If false, it will only add enough additional xp to total to the level up xp
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "XP|FTUE")
   bool FtueLevelUpXpIsOnTopOfRegularXp = false;
};
