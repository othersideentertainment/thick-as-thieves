// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "Developer/TATDevToolTypes.h"
#include "GameFramework/TATDifficulty.h"

// ose
#include "OSEGenericGraphSoftNodeHandle.h"

// ue4
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "TATEditorSettings.generated.h"

struct FTATMatchSettingsQueryContext;
class UTATMatchSettingsBase;
class UTATSceneVariantConfig;
class ATATWeatherPreset;
enum class ETATCharacter : uint8;

UENUM(BlueprintType)
enum class ETATEditorSettingsWeatherOverrideMode : uint8
{
   None,
   OverrideWeatherType         UMETA(Tooltip = "Override the weather type (the preset used will take into account any level-specific overrides)"),
   OverrideWeatherPreset       UMETA(Tooltip = "Override the weather preset class (use a specific preset regardless of level)"),
   UseLevelDefaultEditorPreset UMETA(Tooltip = "Always load the level's default editor preset (specified by TATWorldSettings.EditorDefaultWeatherPreset)"),

   //TODO: Add an override mode that uses the preset that's currently visible in the weather editor tool.
   //      This is trickier than it sounds because the TAT game module can't talk to the TATEditor module.
   //      We'll need a way to route this data over to the game module to use it (maybe just by having the editor tool notify something in the game module on change).
};

// Just a string wrapped in a struct. Needed to prevent GetOptions from being applied to it when used as a TMap value type
USTRUCT()
struct TAT_API FTATEditorSettingsMatchValue
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FString Value;
};

UENUM()
enum class ETATEditorSettingsCheatCommandContext : uint8
{
   Player = 0,
   World,
   Engine,
};

USTRUCT()
struct TAT_API FTATEditorSettingsCheatCommand
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Meta = (InlineEditConditionToggle))
   bool Enabled = true;

   UPROPERTY(EditAnywhere, Meta = (EditCondition = "Enabled"))
   FString Cheat;

   UPROPERTY(EditAnywhere)
   ETATEditorSettingsCheatCommandContext Context = ETATEditorSettingsCheatCommandContext::Player;

   UPROPERTY(EditAnywhere)
   bool RunOnClients = false;

   void RunCheat(UWorld* world, APlayerController* pc) const;
};

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "[TAT] Editor Per User Settings"))
class TAT_API UTATEditorSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // for bp
   UFUNCTION(BlueprintPure, Category = "TAT Editor Settings")
   static UTATEditorSettings* GetTATEditorSettings() { return GetMutableDefault<UTATEditorSettings>(); }

   // for C++
   static const UTATEditorSettings& Get() { return *GetDefault<UTATEditorSettings>(); }
   static UTATEditorSettings& GetMutable() { return *GetMutableDefault<UTATEditorSettings>(); }

   
   UFUNCTION(BlueprintCallable, Category = "TAT Editor Settings", meta= (DisplayName="SaveConfig"))
   void BP_SaveConfig()
   {
      SaveConfig();
   }

   // Character settings
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (Tooltip = "Use to force character selection for player 0"))
   ETATCharacter OverrideCharacter_Player0 = static_cast<ETATCharacter>(0);
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (Tooltip = "Use to force character selection for player 1"))
   ETATCharacter OverrideCharacter_Player1 = static_cast<ETATCharacter>(0);
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (Tooltip = "Use to force character selection for player 2"))
   ETATCharacter OverrideCharacter_Player2 = static_cast<ETATCharacter>(0);
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (Tooltip = "Use to force character selection for player 3"))
   ETATCharacter OverrideCharacter_Player3 = static_cast<ETATCharacter>(0);
   
   ETATCharacter GetOverrideCharacter(const UWorld* world) const;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (Categories = "Tool.Type", Tooltip = "Use to force gear selection for player 0"))
   bool OverrideGearLoadouts = false;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (EditCondition = "OverrideGearLoadouts", Categories = "Tool.Type", Tooltip = "Use to force gear selection for player 0"))
   TArray<FGameplayTag> OverrideGearLoadout_Player0;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (EditCondition = "OverrideGearLoadouts", Categories = "Tool.Type", Tooltip = "Use to force gear selection for player 1"))
   TArray<FGameplayTag> OverrideGearLoadout_Player1;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character", Meta = (EditCondition = "OverrideGearLoadouts", Categories = "Tool.Type", Tooltip = "Use to force gear selection for player 2"))
   TArray<FGameplayTag> OverrideGearLoadout_Player2;

   bool GetOverrideGearLoadout(const UWorld* world, TArray<FGameplayTag>& outGearLoadout) const;

   UPROPERTY(Config, EditAnywhere, Category = "Quests", meta = (InlineEditConditionToggle))
   bool ShouldOverrideMission = false;

   UPROPERTY(Config, EditAnywhere, Category = "Quests", meta = (EditCondition="ShouldOverrideMission", Categories = "Mission"))
   FGameplayTag MissionOverride;
   
   UPROPERTY(Config, EditAnywhere, Category = "Quests")
   bool OverrideContracts = false;

   UPROPERTY(Config, EditAnywhere, Category = "Quests", meta = (EditCondition="OverrideContracts", EditConditionHides, Categories = "Contract"))
   FGameplayTag ContractTag;
   
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (EditCondition = "OverrideContracts", EditConditionHides, Tooltip = "Use to force active quests in a match in PIE"))
   TArray<bool> PlayersOfficiallyOnContract;

   UPROPERTY(Config, EditAnywhere, Category = "Lobby")
   bool OverrideLobbySettings = false;
   
   UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (EditCondition="OverrideLobbySettings", EditConditionHides, Categories = "Map"))
   FGameplayTag LobbyMapTag;

   UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (EditCondition="OverrideLobbySettings", EditConditionHides))
   TOptional<ETATDifficulty> LobbyDifficulty;

   /// In-Game Dev Tool UI Settings
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Dev Tool UI")
   bool EnableDevToolUI = false;

   UPROPERTY(Config, EditAnywhere, Category = "Dev Tool UI", Meta = (EditCondition = "EnableDevToolUI"))
   FTATDevToolSubsystemSettings DevToolUISettings;

   // Cheats

   // Cheats that are automatically run when playing in the editor
   UPROPERTY(Config, EditAnywhere, Category = "Cheats", Meta = (TitleProperty = "Cheat"))
   TArray<FTATEditorSettingsCheatCommand> AutoExecCheats;

   // For AutoExecCheats, how long to show on-screen messages when the cheats are used
   UPROPERTY(Config, EditAnywhere, Category = "Cheats", Meta = (UIMin = 0, ClampMin = 0))
   float AutoExecCheatScreenMessageDuration = 4.0f;

   // World randomization seed override, -1 is "unset", anything else is the random seed
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   int WorldRandomizationSeed = INDEX_NONE;

   // Whether players should start invulnerable
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   bool StartInvulnerable = false;

   // Whether players should start undetectable
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   bool StartUndetectable = false;

   // Whether lockpicking should be disabled for the local player
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   bool DisableLockpickingMinigame = false;

   // Whether we should skip the intro title card / cutscene / etc flow when loading into the map
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   bool SkipMapIntroStates = true;

   /// Hides the player's stealth visualizer (the blue ring around the player)
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   bool HidePlayerStealthVisualization = false;

   // Whether we should override the weather when playing or simulating in the editor
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats")
   ETATEditorSettingsWeatherOverrideMode WeatherOverride = ETATEditorSettingsWeatherOverrideMode::None;

   /// Force this weather type when playing or simulating in the editor
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats", Meta = (Categories = "Weather.Type", EditCondition = "WeatherOverride == ETATEditorSettingsWeatherOverrideMode::OverrideWeatherType", EditConditionHides))
   FGameplayTag WeatherTypeOverride;

   /// Force this weather preset when playing or simulating in the editor
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cheats", Meta = (EditCondition = "WeatherOverride == ETATEditorSettingsWeatherOverrideMode::OverrideWeatherPreset", EditConditionHides))
   TSoftClassPtr<ATATWeatherPreset> WeatherPresetOverride;

   // Whether we should override the difficulty
   UPROPERTY(Config, EditAnywhere, Category = "Cheats", meta = (InlineEditConditionToggle))
   bool ShouldOverrideDifficulty = false;

   /// Force this difficulty when playing or simulating in the editor
   UPROPERTY(Config, EditAnywhere, Category = "Cheats", Meta = (EditCondition = "ShouldOverrideDifficulty"))
   ETATDifficulty DifficultyOverride = ETATDifficulty::Easy;

   // Default match settings applied at BeginPlay
   UPROPERTY(Config, EditAnywhere, Category = "Cheats", Meta = (GetOptions = "_GetMatchSettingsProperties", TitleProperty = "Value"))
   TMap<FString, FTATEditorSettingsMatchValue> DefaultMatchSettings;

   UPROPERTY(Config, EditAnywhere, Category = "Modular Quest System", Meta = (InlineEditConditionToggle))
   bool OverrideQuestGraphWorldTags = false;

   // Manually specify the WorldTags used in quest graph generation
   UPROPERTY(Config, EditAnywhere, Category = "Modular Quest System", Meta = (EditCondition = "OverrideQuestGraphWorldTags"))
   FGameplayTagContainer QuestGraphWorldTags;

   UPROPERTY(Config, EditAnywhere, Category = "Modular Quest System", Meta = (InlineEditConditionToggle))
   bool OverrideQuestGraphQuestTags = false;

   // Manually specify the initial QuestTags used in quest graph generation
   UPROPERTY(Config, EditAnywhere, Category = "Modular Quest System", Meta = (EditCondition = "OverrideQuestGraphQuestTags"))
   FGameplayTagContainer QuestGraphQuestTags;

   // Force these quest graph nodes to always be selected when possible.
   // When selecting the next quest graph node, if a node in this list is one of the possible choices,
   // it will always be selected (ignoring any edge requirements or weights).
   UPROPERTY(Config, EditAnywhere, Category = "Modular Quest System", Meta = (GraphType = "/Script/TAT.TATQuestGraphBase"))
   TArray<FOSEGenericGraphSoftNodeHandle> QuestGraphNodeOverrides;

   // overrides the scene variants selected in a level
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "SceneVariants")
   TArray<TSoftObjectPtr<UTATSceneVariantConfig>> SceneVariantOverrides;

   // List of weather presets to always show in at the top of the weather tool
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weather Editor Tool Settings")
   TArray<TSoftClassPtr<ATATWeatherPreset>> CustomWeatherPresets;

   // Always show the preset property editor panel in the weather tool by default?
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weather Editor Tool Settings")
   bool ShowPresetEditorDefault = false;

   // Link selection and visibility in the weather tool by default?
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weather Editor Tool Settings")
   bool AutoVisibilityDefault = true;

   //
   UPROPERTY(Config)
   TSoftClassPtr<ATATWeatherPreset> LastSelectedWeatherPresetInEditorTool;

   void ApplyDefaultMatchSettings(UTATMatchSettingsBase& matchSettings, const FTATMatchSettingsQueryContext& context) const;

private:
   // UI helper function for DefaultEditorMatchSettings
   UFUNCTION()
   TArray<FString> _GetMatchSettingsProperties() const;
};
