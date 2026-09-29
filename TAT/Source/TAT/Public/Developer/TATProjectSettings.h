// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATWorldTypes.h"
#include "Maps/TATMapPreviewData.h"
#include "UI/TATSystemMessageScreen.h"

// ose
#include "Abilities/OSEHeldActionCues.h"

// ue
#include "Engine/CollisionProfile.h"
#include "Engine/DeveloperSettings.h"
#include "Fonts/SlateFontInfo.h"
#include "GameplayTagContainer.h"

#include "TATProjectSettings.generated.h"

class UDataTable;
class UPaperSprite;
class UUserWidget;
class UTATSeamlessTravelLoadingScreenWidget;
class UTATCheatManager;
class UGameplayEffect;
class UTATQuestNoteScreen;
class UTexture;
class UTATMatchSettingsBase;
class UTATCharactersMetadata;
class UTATPostMatchFeedbackTipDataAsset;
class UTATTransientMapActorDataAsset;
class UTATMapSpriteDataAsset;
class UTATSystemMessageActivatableWidget;

enum class ETATTeamCharacterType : uint8;
enum class ETATContractState : uint8;

USTRUCT(BlueprintType)
struct TAT_API FTATNoiseStimGlyphSettings
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Stim Glyph Settings")
   bool Enabled = true;

   /// Indicator type to spawn for this noise stim event
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Stim Glyph Settings", Meta = (EditCondition = "Enabled", Categories = "Indicator"))
   FGameplayTag IndicatorType;

   /// Minimum distance in Unreal units between two glyphs of this type.
   /// If this noise stim occurs within this distance of an existing glyph of this type, the existing glyph's lifespan will be reset instead of spawning a new one.
   /// Set to zero to disable this behavior and always spawn a new glyph.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Stim Glyph Settings", Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float DeduplicateDistance = 100.0f;

   FORCEINLINE bool IsEnabledAndValid() const { return Enabled && IndicatorType.IsValid(); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATMapTypeSettings
{
   GENERATED_BODY()

   /// Should toolsets automatically add default tools on spawn for player pawns? (eg. inventory tool, map, etc.)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Type Settings")
   bool AllowDefaultPlayerTools = false;

   /// Should we automatically add the gear/tool loadout for the player's class when they spawn? (eg. blackjack, knockout gas, etc.)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Type Settings")
   bool AddDefaultPlayerGearLoadoutOnSpawn = false;

   /// Should maps of this type use map intro states (Loading, TitleCard, Cutscene, etc.)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Type Settings")
   bool UseMapIntroStates = false;

   /// Should allow active quests in a match (or should this be allow matches?)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Type Settings")
   bool AllowActiveQuests = false;

   // Should the map run scene variants and spawners?
   UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Map Type Settings")
   bool RunMapVariation = false;
};

USTRUCT(BlueprintType)
struct TAT_API FTATMapSettings
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings", meta=(Categories="Map"))
   FGameplayTag MapTag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UWorld> Map;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultModes, meta = (MetaClass = "/Script/Engine.GameModeBase"))
   FSoftClassPath GameMode;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FString MapName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText MapDisplayName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText MapFlavorText;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UTexture> SplashImage;

   // Set of quests which can be played on this map. Entries cannot overlap across multiple maps!
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Mission"))
   TArray<FGameplayTag> Missions;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MapPreview")
   TSoftObjectPtr<UStaticMesh> MapPreviewMesh;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MapPreview")
   FVector MeshScaling = FVector(1.0f);

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MapPreview")
   FVector2D CoordinatesOnCityMap = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FTATMapSettingsHandle
{
   GENERATED_BODY()

   UPROPERTY()
   int32 MapIndex = INDEX_NONE;

   bool IsValid() const { return MapIndex != INDEX_NONE; }
   const FTATMapSettings* Get() const;
};

// just config for icon/name for a one-off quest reward type
USTRUCT()
struct TAT_API FTATRewardDisplayConfig
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<UPaperSprite> Icon;

   UPROPERTY(EditAnywhere)
   FText Name;
};

USTRUCT()
struct TAT_API FTATMatchSettingsUpgradeConfig
{
   GENERATED_BODY()

   /// The name of the match settings property to check.
   /// If this property is defined and is set to true, the upgrade tag will be added to all players for that match.
   UPROPERTY(EditAnywhere)
   FName MatchSettingsBoolPropertyName;

   UPROPERTY(EditAnywhere)
   FGameplayTag UpgradeTag;

   UPROPERTY(EditAnywhere, Meta = (UIMin = 1, ClampMin = 1))
   int32 UpgradeLevel = 1;

   bool IsValid() const { return MatchSettingsBoolPropertyName != NAME_None && UpgradeTag.IsValid(); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATDisplayedClueFactCategory
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (Categories="ClueFactCategory"))
   FGameplayTag CategoryTag;

   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   FText CategoryTitle;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Project Settings"))
class TAT_API UTATProjectSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATProjectSettings();

   // bp access
   UFUNCTION(BlueprintPure, Category = "TAT Project Settings")
   static UTATProjectSettings* GetTATSettings() { return GetMutableDefault<UTATProjectSettings>(); }

   // C++ access
   static const UTATProjectSettings& Get() { return *GetDefault<UTATProjectSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Localization")
   TSoftObjectPtr<UStringTable> SettingsStringTable;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Hub")
   FName HubMapName;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Hub")
   bool EnableThievesDen = false;

   /// When a player enters their den, create a new session that other players can join
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Hub", meta = (EditCondition = "EnableThievesDen"))
   bool CreateJoinableSessionInThievesDen = false;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Hub", meta = (EditCondition = "EnableThievesDen"))
   FName ThievesDenMapName;

   // Whether the player can choose to host a listen server directly
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Hub")
   bool AllowManualListenServer = true;

   // Whether to show the tutorial on the main menu even after it has been completed
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial")
   bool ShowTutorialMapAfterComplete = false;

   // Whether to show the "normal" play button once
   // NOTE: This should be removed once the tutorial is completable, and players that want to skip can skip with a cheat
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial")
   bool ShowNormalPlayIfTutorialUnstarted = false;


   // The desired contract states after the FTUE is over
   // Initially tried to just does this in BP, but BP call to cheat was failing in main menu
   UPROPERTY(Config, EditDefaultsOnly, Category = "Tutorial", meta = (Categories="Contract"))
   TMap<FGameplayTag, ETATContractState> PostFtueContracts;

   // The minimum money for a player to have in the thieves den tutorial
   // If they resume with less than this, it will be topped up to this amount
   UPROPERTY(Config, BlueprintReadOnly, EditDefaultsOnly, Category = "Tutorial")
   int FtueMinThievesDenMoney = 0;

   // game mode to use if forcing pvp
   UPROPERTY(Config, EditAnywhere, Category = DefaultModes, meta = (MetaClass = "/Script/Engine.GameModeBase"))
   FSoftClassPath PvpGameMode;

   // Slightly hacky tutorial hookup for tutorial that play when an unlock becomes available after a match
   UPROPERTY(Config, EditDefaultsOnly, Category = Tutorial, meta = (Categories = "UnlockableCategory"))
   TMap<FGameplayTag, TSoftClassPtr<AActor>> PostMatchUnlockTutorials;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Maps", meta=(TitleProperty="{MapName}"))
   TArray<FTATMapSettings> Maps;

   /// Per-map-type settings
   UPROPERTY(Config, EditDefaultsOnly, Category = "Maps", meta = (ArraySizeEnum = "/Script/TAT.ETATMapType"))
   FTATMapTypeSettings MapTypeSettings[static_cast<uint32>(ETATMapType::MAX)] = {};

   /// Render target to use when auto-generating the in-game map for the primary map boundary actor
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
   TSoftObjectPtr<UTextureRenderTarget2D> AutoGeneratedMapRenderTarget;

   /// Render targets to use when auto-generating in-game maps for secondary map boundary actors
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
   TArray<TSoftObjectPtr<UTextureRenderTarget2D>> AutoGeneratedMapRenderTargets_Secondary;

   /// Map material to use with the above render target when auto-generating in-game maps
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
   TSoftObjectPtr<UMaterialInterface> AutoGeneratedMapMaterial;

   /// The max distance (in cm) the local player needs to be from an Actor in order for it to show on the Map and Compass
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Map and Compass", meta = (Units = "cm"))
   float MaxDistanceToVisibleMapAndCompassActors = 2000.0f;

   // How far above/below (in cm) an actor needs to be relative to the player before they get culled from the Map and Compass
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Map and Compass", meta = (Units = "cm"))
   float VerticalDisplacementThreshold = 600.f;

   UFUNCTION(BlueprintCallable, DisplayName = "Find Map Settings", Category = "Maps")
   FTATMapSettings BP_FindMapSettings(const FSoftObjectPath& map) const;
   const FTATMapSettings* FindMapSettings(const FSoftObjectPath& map) const;
   const FTATMapSettings* FindMapSettings(const UWorld* world) const;
   const FTATMapSettings* FindMapSettings(const FGameplayTag& mapTag) const;

   UFUNCTION(BlueprintCallable, DisplayName = "Find Map Settings For Tag [Handle]", Category = "Maps")
   static FTATMapSettingsHandle BP_GetMapSettingHandleForTag(const FGameplayTag& map);
   UFUNCTION(BlueprintCallable, DisplayName = "Find Map Settings For Path [Handle]", Category = "Maps")
   static FTATMapSettingsHandle BP_GetMapSettingHandleForPath(const FSoftObjectPath& map);

   UFUNCTION(BlueprintPure, DisplayName="Is Valid", Category = "Maps")
   static bool MapHandle_IsValid(const FTATMapSettingsHandle& mapHandle);
   UFUNCTION(BlueprintPure, DisplayName="Get Map Tag", Category = "Maps")
   static FGameplayTag MapHandle_GetMapTag(const FTATMapSettingsHandle& mapHandle);
   UFUNCTION(BlueprintPure, DisplayName="Get Map Display Name", Category = "Maps")
   static FText MapHandle_GetDisplayName(const FTATMapSettingsHandle& mapHandle);
   
   const FTATMapTypeSettings& GetMapTypeSettings(ETATMapType mapType) const;
   const FTATMapTypeSettings* GetMapTypeSettingsForCurrentWorld(const UObject* worldContextObject) const;
   const FTATMapTypeSettings& GetMapTypeSettingsForCurrentWorldChecked(const UObject* worldContextObject) const;

   UFUNCTION(BlueprintCallable, DisplayName = "Get Map Type Settings (ETATMapType)", Category = "Maps")
   static bool BP_GetMapTypeSettings_MapType(ETATMapType mapType, FTATMapTypeSettings& settings);

   UFUNCTION(BlueprintCallable, DisplayName = "Get Map Type Settings (Current Map)", Category = "Maps", meta = (WorldContext = "worldContextObject"))
   static bool BP_GetMapTypeSettings_CurrentWorld(const UObject* worldContextObject, FTATMapTypeSettings& settings);

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Characters")
   TSoftObjectPtr<UTATCharactersMetadata> DefaultCharacterMetadata;

   // Tags for content that starts unlocked at the start of the game
   // NOTE: this may not be in cases where content can directly configure that they do not need to be unlocked
   UPROPERTY(Config, EditAnywhere, Category = "Unlocks", meta=(Categories="UnlockableCategory"))
   TArray<FGameplayTag> InitialUnlockedContent;

   // Tags for content that is unlocked as part of the ftue, and thus should be unlocked
   // if skipping the FTUE, or retroactively if the ftue is already complete.
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Unlocks", meta=(Categories="UnlockableCategory"))
   TArray<FGameplayTag> PostFtueUnlockedContent;
   
   UPROPERTY(Config, EditAnywhere, Category = "Unlocks")
   TSoftObjectPtr<class UTATUnlockableContentDataAsset> UnlockableContentData;

   UPROPERTY(Config, EditAnywhere, Category = "Loading")
   TSoftClassPtr<UUserWidget> LoadingScreenWidget;

   UPROPERTY(Config, EditAnywhere, Category = "Match")
   TSoftClassPtr<UTATMatchSettingsBase> DefaultMatchSettingsClass;

   UPROPERTY(Config, EditAnywhere, Category = "Match", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATMatchSettingsGameplayTagGroup"))
   TSoftObjectPtr<UDataTable> MatchSettingsGameplayTagGroups;

   /// Allow using match settings fields to add upgrade tags to all players during a match
   UPROPERTY(Config, EditAnywhere, Category = "Match")
   TArray<FTATMatchSettingsUpgradeConfig> MatchSettingsUpgrades;

   UPROPERTY(Config, EditAnywhere, Category = "Match")
   FName DifficultyPropertyName { FName("Difficulty") };
   
   TSubclassOf<UTATMatchSettingsBase> GetMatchSettingsClass() const;

   /// How much time to delay the "End Match" RPC to avoid races with other end-of-match replication
   UPROPERTY(Config, EditAnywhere, Category = "Match")
   float EndMatchRPCDelaySeconds = 3.0f;

   /// How long to wait in the game mode after the end of the match (and sending end match RPCs) before handling match cleanup
   UPROPERTY(Config, EditAnywhere, Category = "Match|Cleanup", meta = (Units = "seconds"))
   float CleanupDelayAfterMatchEndSeconds = 15.0f;

   /// Failsafe duration, after which dedicated servers will force-end a match and shut down.
   /// Set to 0 to disable.
   UPROPERTY(Config, EditAnywhere, Category = "Match|Cleanup", meta = (Units = "min"))
   float MaximumDedicatedServerMatchDurationMinutes = 0.0f;

   /// Should dedicated servers automatically exit at the end of match-end cleanup?
   UPROPERTY(Config, EditAnywhere, Category = "Match|Cleanup")
   bool AutoExitDedicatedServerProcess = false;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "PostMatch")
   TSoftObjectPtr<UTATPostMatchFeedbackTipDataAsset> PostMatchFeedbackTipsData;

   UPROPERTY(Config, EditAnywhere, Category = "World Map")
   TSoftObjectPtr<UTATTransientMapActorDataAsset> TransientMapActorData = nullptr;
   
   // Collection of tag-identified sprite representations used to show the associated actor on the map
   UPROPERTY(Config, EditAnywhere, Category = "World Map")
   TSoftObjectPtr<UTATMapSpriteDataAsset> MapSpriteData = nullptr;

   UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "Interact", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATInteractPromptStyle"))
   TSoftObjectPtr<UDataTable> InteractPromptStyleDataTable;
   
   UPROPERTY(Config, EditAnywhere, Category = "Quests", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATMissionInfo"))
   TSoftObjectPtr<UDataTable> MissionDataTable;

#if WITH_EDITORONLY_DATA
   // Data table which is allowed to have editor-only missions that won't get pulled into the build (for test levels, etc)
   // Should also include regular missions via CDT
   UPROPERTY(Config, EditAnywhere, Category = "Quests", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATMissionInfo"))
   TSoftObjectPtr<UDataTable> EditorMissionDataTable;
#endif

   const TSoftObjectPtr<UDataTable>& GetDataTableForMissions() const
   {
#if WITH_EDITOR
      return UTATProjectSettings::Get().EditorMissionDataTable;
#else
      return UTATProjectSettings::Get().MissionDataTable;
#endif
   };
   
   UPROPERTY(Config, EditAnywhere, Category = "Screens")
   TSoftClassPtr<UTATSystemMessageScreen> SystemMessageScreen;
   UPROPERTY(Config, EditAnywhere, Category = "Screens")
   TSoftClassPtr<UTATSystemMessageActivatableWidget> SystemMessageActivatableWidget;

   UPROPERTY(Config, EditAnywhere, Category = "Quests", Meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATContractInfo"))
   TSoftObjectPtr<UDataTable> ContractDataTable;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quests|Contract Toasts")
   FText ContractStartedToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quests|Contract Toasts", meta = (Categories="Toast.Type"))
   FGameplayTag ContractStartedToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quests|Contract Toasts")
   FText ContractCompletedToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quests|Contract Toasts", meta = (Categories="Toast.Type"))
   FGameplayTag ContractCompletedToastTag;

   // Replacement keywords are {Item}
   // TODO: possible wrinkles with string table loading in packaged builds.
   //       Since the lockpick settings work, it may be a matter of something else needing to load the string table
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Objective Text")
   FText DefaultStealItemObjectiveText;
   
   // Replacement keywords are {Amount}
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Objective Text")
   FText DefaultLootValueObjectiveText;

   UPROPERTY(Config, EditAnywhere, Category = "Quests|Screens")
   TSoftClassPtr<UTATQuestNoteScreen> QuestNoteScreen;
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Screens")
   TSoftClassPtr<UTATQuestNoteScreen> FairyContractOutroScreen;
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Screens")
   TSoftClassPtr<class UTATQuestCompleteScreen> MissionCompleteScreen;
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Screens")
   TSoftClassPtr<class UTATQuestCompleteScreen> ContractCompleteScreen;
   UPROPERTY(Config, EditAnywhere, Category = "Quests|Screens")
   TSoftClassPtr<class UTATContractJournalScreen> QuestJournalScreen;

   UPROPERTY(Config, EditAnywhere, Category = "XP|Screens")
   TSoftClassPtr<class UTATEndMatchXPScreen> EndMatchXPScreen;

   UPROPERTY(Config, EditAnywhere, Category = "Quests|Rewards")
   FTATRewardDisplayConfig MoneyReward;

#if WITH_EDITORONLY_DATA
   // Whether to require clue location tags (and names) for relevant clue and loot spawners
   // TODO: remove this setting once enabled
   UPROPERTY(Config, EditAnywhere, Category = "Clues")
   bool RequireClueLocationTags = false;
#endif

   // The order to show clue fact categories in
   // NOTE: It occurs to me that this doesn't scale if many of these categories end up being
   //       one-off or quests-specific. But there is a plausible path to migrate from this
   //       without touching the core fact data.
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Clues")
   TArray<FTATDisplayedClueFactCategory> DisplayedClueFactCategories;

   // Toast type to use for clue facts shared by an ally
   UPROPERTY(Config, EditAnywhere, Category = "Clues", meta = (Categories="Toast.Type"))
   FGameplayTag SharedClueToastTag;

   // text for toast when clue facts shared by an ally
   // if IncludeFactWithSharedClueToast = true, will use {Fact} format string
   UPROPERTY(Config, EditAnywhere, Category = "Clues")
   FText SharedClueToastText;

   UPROPERTY(Config, EditAnywhere, Category = "Clues")
   bool IncludeFactWithSharedClueToast = false;

   // The default widget used when picking up loot with attached clues
   UPROPERTY(Config, EditAnywhere, Category = "Clues")
   TSoftClassPtr<class UTATReadableClueWidget> DefaultLootClueWidget;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag ConditionDownedTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag PlayingSyncedAnimationTag;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag VulnerableToTakedownTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag SuspiciousActionStatusTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag MovementDisabledStatusTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag LookDisabledStatusTag;
   // Tag to block actual move input, but not fake look input generated by code
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag DisableDirectMoveInputTag;
   // Tag to block actual look input, but not fake look input generated by code
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag DisableDirectLookInputTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag HiddenFromViewStatusTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag OnlyOverlapCapsuleStatusTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag InvulnerableStatusTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Statuses")
   FGameplayTag TrustClientMovementTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Stats", meta = (Categories = "PlayerStats"))
   FGameplayTag LootPickedUpStatTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stats", meta = (Categories = "PlayerStats"))
   FGameplayTag PocketsPickedStatTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stats", meta = (Categories = "PlayerStats"))
   FGameplayTag PlayerDetectedStatTag;
   /// Used to track the time remaining in the match when a player successfully escapes
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stats", meta = (Categories = "PlayerStats"))
   FGameplayTag PlayerEscapeTimeRemainingStatTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag Lockpick;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag SuppressInteractionTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag DisableAbilitiesTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag DisguiseActiveTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag UsingMonocularTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag AstralProjectionTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   float AIAggressiveTargetFallOffTimer = 5.0f;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Ability")
   FGameplayTag UnconsciousEffectDurationTag;

   /// Noise stims that have a tag in this container will spawn the specified glyph indicator.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators", meta = (GameplayTagFilter = "AI.Stim.Hearing"))
   TMap<FGameplayTag, FTATNoiseStimGlyphSettings> NoiseStimGlyphIndicatorSettings;

   /// The list of valid indicators that can be used for the Thief Vision ability.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators", Meta = (RowType="/Script/TAT.TATClientProxyIndicatorConfig"))
   TSoftObjectPtr<UDataTable> ThiefVisionIndicatorsDataTable;

   /// Status tag added to characters with thief vision enabled
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   FGameplayTag ThiefVisionStatusTag;

   /// Distance past the normal visible distance that the client proxy indicator will continue to be visible
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   float ClientProxyIndicatorHysteresisDistance = 150.0f;

   /// Extra distance past the max visible distance that this indicator will be replicated (but not necessarily visible).
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   float ClientProxyIndicatorReplicationBufferDistance = 300.0f;

   /// Default max distance for client proxy indicators (eg. Thief Vision).
   /// If a client proxy indicator does not have a max distance configured, it will use this value.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   float DefaultClientProxyIndicatorMaxVisibleDistance = 2000.0f;
   
   /// Fireflies: Radius to search for targets
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   float ThiefVisionFireflySearchRadius = 2500.0f;

   /// Fireflies: Extra range to give targets who are already shown
   UPROPERTY(Config, EditDefaultsOnly, Category = "Indicators")
   float ThiefVisionFireflyShownRangeBoost = 100.0f;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Tools")
   FGameplayTagContainer WeaponToolTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Tools")
   FGameplayTag AmmoCostActivationFailureTag;

   // tags that will cause the current tools to be visually stowed
   UPROPERTY(Config, EditDefaultsOnly, Category = "Tools|Stow")
   FGameplayTagContainer AutoStowTags;

   // Loose gameplay tags added when auto-stowed
   UPROPERTY(Config, EditDefaultsOnly, Category = "Tools|Stow")
   FGameplayTagContainer AddedTagsWhenAutoStowed;

   // Loose gameplay tags added when auto-stowed
   UPROPERTY(Config, EditDefaultsOnly, Category = "Tools|Stow")
   FGameplayTagContainer AbilitiesToCancelWhenAutoStowed;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Loadouts", Meta = (Categories = "Loadout.Slot"))
   TMap<FGameplayTag, FText> LoadoutSlotCategoryDisplayNames;

   // Collision channels to ignore on character capsules when they are lying down
   UPROPERTY(Config, EditDefaultsOnly, Category = "Collision")
   TArray<TEnumAsByte<ECollisionChannel>> CapsuleChannelsToIgnoreWhenDown;

   FCollisionResponseContainer GetLyingDownCollisionMask() const;


   // tags that disallow prevent a character from being a valid takedown target (above the per-asset stuff)
   // TODO: There are likely further restructuring of synced animation data so that it does not require
   //       as much error-prone duplication
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
   FGameplayTagContainer InvalidTakedownTargetTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Inventory")
   FCollisionProfileName ItemDropTraceProfile;

   // Category of legacy-inventory items dropped on player KO
   // Basically for keys
   UPROPERTY(Config, EditDefaultsOnly, Category = "Inventory", meta = (Categories = "Item"))
   FGameplayTag LegacyItemsToDropOnPlayerKO;

   // Which collision profiles to allow on the parts of doors and windows that open
   // Used for validation, e.g. to ensure that astral projection pawns can pass through them
   UPROPERTY(Config, EditDefaultsOnly, Category = "Collision")
   TArray<FCollisionProfileName> AllowedCollisionProfilesForOpenableDoorsAndWindows;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Teams")
   TMap<ETATTeamCharacterType, uint8> TeamAssignments;

   UFUNCTION(BlueprintPure, Category = "Teams")
   static uint8 GetTeamAssignmentForCharacterType(ETATTeamCharacterType characterType);

   UPROPERTY(Config, EditAnywhere, Category = "Cheats")
   TSoftClassPtr<UTATCheatManager> ProjectCheatManagerSubclass;

   // Colors that can be used to indicate the player
   UPROPERTY(Config, EditDefaultsOnly, Category = "Players")
   TArray<FLinearColor> PlayerColorPool;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Players")
   bool ShouldPlayerBeDownedByNonPlayerSources { false };

   UPROPERTY(Config, EditDefaultsOnly, Category = "Teleport")
   FGameplayTagContainer TeleportPawnBlockedTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Teleport")
   TArray<FName> TeleportComponentBlockedTags;

   /// The list of possible toasts (short, temporary messages in the UI) and their configurations
   UPROPERTY(Config, EditDefaultsOnly, Category = "Toast")
   TSoftObjectPtr<UDataTable> ToastConfigsDataTable;

   /// How long the toast messages should be displayed in the UI for
   UPROPERTY(Config, EditDefaultsOnly, Category = "Toast")
   float ToastDuration = 2.0f;

   /// The maximum number of simultaneous toast messages that can appear on-screen at the same time
   UPROPERTY(Config, EditDefaultsOnly, Category = "Toast")
   int32 MaxNumSimultaneousToastMessages = 2;

   /// Skip playing toast sound effects within this number of seconds from the last toast notification with a sound effect
   UPROPERTY(Config, EditDefaultsOnly, Category = "Toast")
   float MinSecondsBetweenToastSFX = 1.0f;

   /// What angle we consider to be "facing" for purposes of sneak attacks coming from outside that angle
   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat")
   float SneakAttackFacingAngleWidth = 100.0f;

   float GetCosineSneakAttackFacingHalfAngle() const;

   // Note on trap config: I am open to this being somewhere else, but it
   // didn't seem worth making its own config for. They are not on the actor
   // as design indicated that they should not vary between types of traps,
   // and may not have a common BP base class.

   UPROPERTY(Config, EditAnywhere, Category = "Traps")
   float TrapDisarmHoldDuration = 3;

   UPROPERTY(Config, EditAnywhere, Category = "Traps")
   float TrapRearmHoldDuration = 3;

   UPROPERTY(Config, EditAnywhere, Category = "Traps")
   FOSEHeldActionCues TrapDisarmHeldActionCues;

   UPROPERTY(Config, EditAnywhere, Category = "Traps")
   FOSEHeldActionCues TrapRearmHeldActionCues;

   UPROPERTY(Config, EditAnywhere, Category = "Traps", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag TrapDisarmInteractAnimation;

   UPROPERTY(Config, EditAnywhere, Category = "Traps", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag TrapRearmInteractAnimation;

   // Breakables

   // Gameplay tags that all breakables actors have (even if invulnerable, probably)
   UPROPERTY(Config, EditAnywhere, Category = "Breakables")
   FGameplayTagContainer BreakableIntrinsicTags;

   // Default hearing stim when a breakable is broken
   UPROPERTY(Config, EditAnywhere, Category = Breakables, meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag DefaultBreakableHearingStim;

   UPROPERTY(Config, EditAnywhere, Category = "Breakables|Repair")
   TSoftClassPtr<UGameplayEffect> RepairBreakableEffect;

   UPROPERTY(Config, EditAnywhere, Category = "Breakables|Repair")
   FOSEHeldActionCues BreakableRepairHeldActionCues;

   UPROPERTY(Config, EditAnywhere, Category = "Breakables|Repair", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag BreakableRepairInteractAnimation;

   /// Length of endgame (in seconds) when it's triggered for any reason
   UPROPERTY(Config, EditAnywhere, Category = "Match|Endgame", meta = (ArraySizeEnum="/Script/TAT.ETATDifficulty", Units="s", ClampMin="0"))
   float EndgameDurationsByDifficulty[static_cast<int>(ETATDifficulty::MAX)] = {1200.0f, 600.0f, 480.0f };

   const float GetEndgameDurationForDifficulty(ETATDifficulty difficulty) const;

   /// How much stashed loot would trigger the endgame
   /// Counts cumulative loot value for all stashes that have completed sealing (across players)
   UPROPERTY(Config, EditAnywhere, Category = "Match|Endgame")
   int32 StashedLootValueToTriggerEndgame = 5000;

   /// Time remaining in a match when it is considered "essentially complete" for reward purposes
   /// (e.g. counting stashes as sealed)
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (Units = "seconds"))
   float MatchEssentiallyCompleteThreshold = 30.0f;

   /// Tag that marks a player as eligible for an end of match escape
   /// (if they have the tag at the end of the match)
   UPROPERTY(Config, EditAnywhere, Category = "Match")
   FGameplayTag EligibleForEndOfMatchEscapeTag;

   void AppendEffectsToPreload(TArray<FSoftObjectPath>& outPathsToLoad) const;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Attitudes")
   bool UseIndividualAttitudes { true };

   UFUNCTION(BlueprintPure, Category = "Attitudes")
   static bool ShouldUseIndividualAttitudes();

   UPROPERTY(Config, EditDefaultsOnly, Category = "AI|Light Detection")
   bool UseLightDetection { true };

   UFUNCTION(BlueprintPure, Category = "Attitudes")
   static bool ShouldUseLightDetection();
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "AI|Avoidance Groups")
   uint32 DownedNPCAvoidanceGroup { 3 };

   UPROPERTY(Config, EditDefaultsOnly, Category = "Navigation")
   FName NavModifiableDynamicComponentTag = TEXT("NavModifiable_Dynamic");

   UPROPERTY(Config, EditDefaultsOnly, Category = "Navigation")
   FName NavModifiableStaticComponentTag = TEXT("NavModifiable_Static");

   // Display watermark with username and userID over game
   UPROPERTY(Config, EditDefaultsOnly, Category = "Watermark")
   bool ShowWatermark { false };

   UPROPERTY(Config, EditDefaultsOnly, Category = "Watermark", meta = (EditCondition = "ShowWatermark", EditConditionHides))
   FSlateFontInfo WatermarkFontInfo;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "Fonts")
   FSlateFontInfo RichTextInputFont;

   // Additional scale applied to input icons on mobile (steam deck)
   // NOTE: They already inherit some scaling from the text itself, so this if an additional scale
   UPROPERTY(Config, EditDefaultsOnly, Category = "Fonts")
   float RichTextIconMobileScale = 1.f;

   // The default vertical offset of respawn markers above a player start
   UPROPERTY(Config, EditDefaultsOnly, Category = "Respawn", meta=(Units="cm"))
   float PlayerStartRespawnMarkerZOffset = 200.f;
   
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText JoinSessionToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag JoinSessionToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText JoinSessionFailedToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag JoinSessionFailedToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText IncompleteTutorialInviteToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag IncompleteTutorialInviteToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText JoinSessionFailedNotFoundToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag JoinSessionFailedNotFoundToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText TravelToSessionToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag TravelToSessionToastTag;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session")
   FText TravelToSessionFailedToastMessage;
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sessions|Join Session", meta = (Categories="Toast.Type"))
   FGameplayTag TravelToSessionFailedToastTag;

private:
   // Used to cache cos(SneakAttackFacingAngleWidth / 2) since we will use that in calculations
   mutable float _cachedCosineSneakAttackFacingHalfAngle = 0.0f;
   mutable float _cachedSneakAttackFacingAngleWidth = -1.0f;
};
