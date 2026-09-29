// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "CharacterCustomization/TATCharacterLoadout.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Progression/TATPlayerExperience.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "SaveGame/TATSavedLoot.h"
#include "Tutorial/TATSavedFtueState.h"
#include "Upgrades/UpgradeCurrencies/TATUpgradeCurrencySource.h"

// ose
#include "Abilities/OSEUpgradeState.h"
#include "Identity/OSESaveGame.h"

// ue4
#include "GameplayTagContainer.h"

#include "TATSaveGame.generated.h"

class UTATAnalyticsManager;
class UTATGameInstance;
class UOSESaveGameSystem;
struct FMatchPersistentData;
struct FOSEGenericGraphNodeHandle;
enum class ETATContractState : uint8;
enum class ETATDifficulty : uint8;

UENUM(Blueprintable)
enum class ETATFeatureAvailabilityToPlayer : uint8
{
   // Player can access feature
   Available = 0,
   // Player can unlock feature from the Electrogram to make it Available
   Unlockable = 1,
   // Player can see the feature but cannot unlock nor access the feature yet
   Locked = 2,
   // Player cannot see feature at all
   Hidden = 3
};

UENUM(BlueprintType)
enum class ETATUnlockType : uint8
{
   RequiredLevel = 0,
   RequiredContract = 1
};

USTRUCT(BlueprintType)
struct TAT_API FTATUnlockCondition
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Conditions")
   ETATUnlockType UnlockType = ETATUnlockType::RequiredLevel;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Conditions", meta = (EditCondition = "UnlockType == ETATUnlockType::RequiredLevel", EditConditionHides))
   int32 RequiredLevel = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Conditions", meta = (EditCondition = "UnlockType == ETATUnlockType::RequiredContract", EditConditionHides))
   FGameplayTag RequiredContractTag;
};

USTRUCT(BlueprintType)
struct TAT_API FTATUnlockRequirement
{
   GENERATED_BODY()

   // Conditions to be met in order to unlock this feature
   // If no conditions are created here, feature will default to Available
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Requirement")
   TArray<FTATUnlockCondition> UnlockConditions;

   // If true, all unlock conditions must be met
   // If false, only one unlock condition needs to be met
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Requirement")
   bool MustMeetAllConditions = false;

   // If true, when a feature is determined to be locked, it will return as hidden instead
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Requirement")
   bool HiddenIfLocked = false;
};

USTRUCT(BlueprintType)
struct TAT_API FTATContractWithStatus
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FGameplayTag ContractTag;

   UPROPERTY(BlueprintReadOnly)
   ETATContractState State = ETATContractState::Unstarted;

   bool operator==(const FGameplayTag& tag) const;

   void AppendToString(FString& result) const;
};

// Struct used to track an exponential moving average of various gameplay accomplishments, bounded by the last N games (see UTATScoringSettings::RankingMatchCountMemory).
// Intended to be compiled into some cumulative ranking score for matchmaking purposes.
USTRUCT()
struct TAT_API FTATPlayerPerformanceRankingData
{
   GENERATED_BODY()

   UPROPERTY()
   float AvgLootValueCarriedOut = 0.f;

   UPROPERTY()
   float AvgLootValueStashed = 0.f;

   // Normalized completion rate for missions
   UPROPERTY()
   float MissionCompletionRate = 0.f;

   // Normalized completion rate for contract
   UPROPERTY()
   float ContractCompletionRate = 0.f;

   // Rate (0 <-> 1) of successful escape
   UPROPERTY()
   float SuccessfulEscapeRatioNormalized = 0.f;
};

USTRUCT()
struct TAT_API FTATCharacterProgression
{
   GENERATED_BODY()

   DECLARE_MULTICAST_DELEGATE(FOnDataChangedEvent);

   UPROPERTY()
   ETATCharacter Character = ETATCharacter::Character0;

   UPROPERTY()
   FTATPlayerPerformanceRankingData PerformanceRanking;

   UPROPERTY()
   int32 MatchCount = 0;

   // currencies used to upgrade characters (and maybe other stuff)
   UPROPERTY()
   FOSESerializedTagMap UpgradeCurrencies;

   // upgrades that are purchased/unlocked, but may not be equipped
   UPROPERTY(NotReplicated)
   FUpgradeState UnlockedUpgrades;

   // upgrades that are currently equipped (not including loadout skill)
   UPROPERTY(NotReplicated)
   TArray<FGameplayTag> EquippedUpgrades;

   UPROPERTY(NotReplicated)
   FGameplayTag EquippedLoadoutSkill;

   UPROPERTY()
   FTATSavedLootInventory Loot;

   //NB. This is the NEW character loadout (as of April 2024)
   UPROPERTY()
   FTATCharacterLoadout GearLoadout;
   UPROPERTY()
   FTATCharacterLoadout AbilityLoadout;
   UPROPERTY()
   FTATCharacterLoadout OutfitLoadout;

   UPROPERTY()
   int32 BadgesTowardNextRank = 0;

   FOnDataChangedEvent OnLootChanged;
   FTATUpgradeCurrencyChanged OnUpgradeCurrencyChanged;

   void GetUpgradeState(FUpgradeState& result, bool includeIntrinsicUpgrades = true) const;
   int32 GetUpgradeValue(FGameplayTag upgradeTag, int32 fallbackValue = 0, bool includeIntrinsicUpgrades = true) const;

   FTATCharacterLoadout* GetLoadout(ETATLoadoutType type);
   const FTATCharacterLoadout* GetLoadout(ETATLoadoutType type) const;

   void UpdatePerformanceRankingForMatch(const FMatchPersistentData& matchPersistentData);

   void AppendToDebugString(FString& result) const;
   void Reset();
};

USTRUCT()
struct TAT_API FTATPlayerProgression
{
   GENERATED_BODY()

   DECLARE_MULTICAST_DELEGATE(FOnDataChangedEvent);

   UPROPERTY()
   FTATPlayerExperience XP;

   UPROPERTY()
   int32 Money = 0;

   FOnDataChangedEvent OnMoneyChanged;

   UPROPERTY()
   TArray<FTATContractWithStatus> Contracts;

   // Now just a cache of a recent contract in the objective state.
   // May be removed.
   UPROPERTY(Transient)
   FGameplayTag LastActiveContract;

   // Tags of content that the player has unlocked
   // We may at some point want to segregate this by type, but
   // keeping it in a single place allows code to treat it uniformly
   UPROPERTY()
   TArray<FGameplayTag> UnlockedContent;

   // Tags of content that is available to unlock, and has been viewed
   // by the player
   UPROPERTY()
   TArray<FGameplayTag> SeenUnlockableContent;

   // what's the progression of each of our characters?
   // (not a TMap because we replicate this struct)
   UPROPERTY()
   TArray<FTATCharacterProgression> CharacterProgression;

   UPROPERTY()
   TOptional<ETATDifficulty> SelectedDifficulty;

   const FTATCharacterProgression* TryGetCharacterProgression(FTATCharacterSaveId character) const;

   void InitLastActiveContract();
   void AppendToDebugString(FString& result) const;
   void Reset();

   ETATContractState GetContractState(FGameplayTag tag) const;
   bool SetContractState(FGameplayTag contractTag, ETATContractState state);
};

USTRUCT()
struct TAT_API FTATPlayerSaveData
{
   GENERATED_BODY()

   // which character did we select most recently?
   UPROPERTY()
   ETATCharacter SelectedCharacter = ETATCharacter::Character0;

   // represents the progression state for this player
   UPROPERTY()
   FTATPlayerProgression PlayerProgression;

   UPROPERTY()
   FTATSavedFtueState FtueState;
};

USTRUCT()
struct TAT_API FTATUXSettings
{
   GENERATED_BODY()

   UPROPERTY()
   bool ShowHUDIcons = true; // #TODO: Move to game settings
};

UCLASS()
class TAT_API UTATSaveGame : public UOSESaveGame, public ITATUpgradeCurrencySource // TODO [JC 6/27/2024]: This interface is deprecated and should be removed, but some old inventory stuff still expects it
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Save Game", meta = (WorldContext = "contextObj"))
   static UTATSaveGame* GetTATSaveGame(const UObject* contextObj);

   virtual UWorld* GetWorld() const override;
   UTATAnalyticsManager* GetAnalyticsManager() const;
protected:
   virtual void _OnSaveDataLoaded() override;
   virtual void _OnSaveDataCreated() override;

   virtual int GetSaveCurrentVersion() const override;
   virtual int GetSaveCurrentClearVersion() const override;
   virtual void _FixupSaveVersion() override;

public:
   // Allow us to pretend that they aren't the same thing (until they really aren't)
   UFUNCTION(BlueprintPure, Category = "Save Game")
   static ETATCharacter GetCharacterType(FTATCharacterSaveId character);

   UFUNCTION(BlueprintPure, Category = "Save Game", meta = (CompactNodeTitle="IsValid"))
   static bool IsCharacterSaveIdValid(const FTATCharacterSaveId& characterSaveId);

   UFUNCTION(BlueprintCallable, Category = "Save Game")
   void SetSelectedCharacter(FTATCharacterSaveId character);

   UFUNCTION(BlueprintPure, Category = "Save Game")
   FTATCharacterSaveId GetSelectedCharacter() const;
   
   UFUNCTION(BlueprintCallable, Category = "Save Game")
   void SetHasShownAnalyticsPrompt();
   
   UFUNCTION(BlueprintPure, Category = "Save Game")
   bool HasShownAnalyticsPrompt() const { return _hasShownAnalyticsPrompt; };   
   
   void SetHasSentUnlocksAnalytics();
   bool HasSentUnlocksAnalytics() const { return _hasSentUnlocksAnalytics; }

   // Player

   const FTATPlayerProgression& GetPlayerProgression() const;

   UFUNCTION(BlueprintPure, Category = "Save Game|Player|Progression")
   const FTATPlayerExperience& GetXP() const;

   void SetXP(int32 totalXP);
   void AddXP(int32 amountXP);
   
   bool HasUnlockedContent(FGameplayTag unlockableTag) const;
   void AddUnlocks(TConstArrayView<FGameplayTag> unlockTags);
   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression", meta=(Categories="UnlockableCategory"))
   void AddUnlock(FGameplayTag unlockTag);
   void RemoveUnlocks(TConstArrayView<FGameplayTag> tagsToRemove);
   void RemoveUnlock(FGameplayTag tagToRemove);
   void ClearUnlocks();
   void MarkUnlocksSeen(TConstArrayView<FGameplayTag> unlockTags);
   void ClearSeenUnlocks();
   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   void ResetUnlocks();

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   const TArray<FTATContractWithStatus>& GetContracts() const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   void FindContractsInState(ETATContractState state, TArray<FGameplayTag>& contracts) const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   FGameplayTag GetLastActiveContract() const;

   UFUNCTION(BlueprintPure, Category = "Save Game|Player|Progression")
   ETATContractState GetContractState(FGameplayTag contractTag) const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   void SetContractState(FGameplayTag contractTag, ETATContractState state);

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   void ClearContracts();

   UFUNCTION(BlueprintPure, Category = "Save Game|Player|Progression")
   ETATFeatureAvailabilityToPlayer GetAvailabilityFromUnlockCondition(const FTATUnlockCondition& unlockCondition) const;

   UFUNCTION(BlueprintPure, Category = "Save Game|Player|Progression")
   ETATFeatureAvailabilityToPlayer GetAvailabilityFromUnlockRequirement(const FTATUnlockRequirement& unlockRequirement) const;

   UFUNCTION(BlueprintPure, Category = "Save Game|Player|Progression")
   int32 GetMoney() const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Player|Progression")
   void UpdateMoney(int moneyDelta);

   // Character

   const FTATCharacterProgression& GetCharacterProgression(FTATCharacterSaveId character) const;

   // use with care, don't use an excuse to spread code around
   void UpdateCharacterProgression(FTATCharacterSaveId character, EOSESavePriority priority, TFunctionRef<void (FTATCharacterProgression&)> mutator);

   UFUNCTION(BlueprintPure, Category = "Save Game|Character|Progression")
   virtual int32 GetUpgradeCurrency(FTATCharacterSaveId character, UPARAM(meta=(Categories = "UpgradeCurrency")) FGameplayTag currencyTag) const override final;

   void AddUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag, int32 amount);
   void RemoveUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag, int32 amount);
   void ClearUpgradeCurrencies(FTATCharacterSaveId character);

   UFUNCTION(BlueprintPure, Category = "Save Game|Character|Loadout")
   FTATCharacterLoadout GetCharacterLoadout(FTATCharacterSaveId character, ETATLoadoutType loadoutType, bool getDefaultLoadoutIfEmpty = true) const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Character|Loadout")
   void UpdateCharacterLoadout(FTATCharacterSaveId character, const FTATCharacterLoadout& newLoadout);

   UFUNCTION(BlueprintPure, Category = "Save Game|Character|Loadout")
   FTATCharacterLoadout GetGearLoadout(FTATCharacterSaveId character, bool getDefaultLoadoutIfEmpty = true) const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|Character|Loadout")
   void UpdateGearLoadout(FTATCharacterSaveId character, const FTATCharacterLoadout& newGearLoadout);

   void AddLoot(FTATCharacterSaveId character, const FTATSavedLootAddRequest& lootToAdd);
   bool RemoveLoot(FTATCharacterSaveId character, const FTATSavedLootRemoveRequest& lootToRemove);

   void SetUpgradeLevel(FTATCharacterSaveId character, FGameplayTag tag, int32 level, bool autoEquip = false);
   void EquipChildUpgrade(FTATCharacterSaveId character, FGameplayTag tag, FGameplayTag parentTag);
   void EquipLoadoutSkill(FTATCharacterSaveId character, FGameplayTag tag);

   // only for testing, probably
   UFUNCTION(BlueprintCallable, Category = "Save Game|Character|Progression")
   void ResetUpgradeProgressForTag(FTATCharacterSaveId character, FGameplayTag tag);

   /// Removes all unlocked upgrades for the specified character. Can be called from the UI (to allow for respecs).
   UFUNCTION(BlueprintCallable, Category = "Save Game|Character|Progression")
   void ResetUpgradeProgressForCharacter(FTATCharacterSaveId character);

   void ResetCharacter(FTATCharacterSaveId character);

   // Misc

   UFUNCTION(BlueprintPure, Category = "Save Game|FTUE")
   ETATSavedFtueState GetFtueState() const;

   UFUNCTION(BlueprintCallable, Category = "Save Game|FTUE")
   void SetFtueState(ETATSavedFtueState state);

   TOptional<ETATDifficulty> GetSelectedDifficulty() const;
   void SetSelectedDifficulty(ETATDifficulty difficulty);


   // ux save data
   UFUNCTION(BlueprintPure, Category = "Save Game|UX")
   bool AreHUDIconsVisible() const;

   void SetHUDIconsVisible(bool showIcons);

   const FGuid& GetPlayerAnalyticsId() const;

   // Events
   //
   // TODO: Add finer-grained events when needed, along with an RAII struct to buffer
   // events so that that changes for a single "transaction" are de-duped and fired
   // after all changes have been made (probably)
   //
   // NOTE: The save game may be unavailable on consoles if a profile changes as a result of
   //       a controller disconnection (or similar), so consumers of these events will have
   //       to account for that.
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProgressionUnexpectedlyChanged);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnProgressionUnexpectedlyChanged OnProgressionUnexpectedlyChanged;

   void DispatchProgressionUnexpectedlyChanged();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnXPProgressionChanged);

   // Fired if XP changes
   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnXPProgressionChanged OnXPProgressionChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContractsChanged);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnContractsChanged OnContractsChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastActiveContractSet, FGameplayTag, ContractTag);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnLastActiveContractSet OnLastActiveContractSet;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnlocksChanged);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnUnlocksChanged OnUnlocksChanged;

   FSimpleMulticastDelegate OnSeenUnlocksChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFtueChanged);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnFtueChanged OnFtueChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedCharacterChanged, ETATCharacter, newCharacter);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game|Events")
   FOnSelectedCharacterChanged OnSelectedCharacterChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterLoadoutChanged, const FTATCharacterLoadout&, newLoadout, ETATCharacter, character);

   UPROPERTY(Transient, BlueprintAssignable, Category = "Save Game| Events")
   FOnCharacterLoadoutChanged OnCharacterLoadoutChanged;

   virtual FTATUpgradeCurrencyChanged& GetUpgradeCurrencyChanged(FTATCharacterSaveId character) override final;

   FTATCharacterProgression::FOnDataChangedEvent& GetCharacterProgressionLootChangedEvent(const FTATCharacterSaveId& character);
   FTATPlayerProgression::FOnDataChangedEvent& GetPlayerProgressionMoneyChangedEvent();

private:
   FTATPlayerProgression& _GetPlayerProgressionMutable();
   FTATCharacterProgression& _GetCharacterProgressionMutable(FTATCharacterSaveId character);

   void _SetLastActiveContract(FGameplayTag contractTag);
   void _HandleContractStateChangeAnalytics(FGameplayTag contractTag, ETATContractState state) const;

   UPROPERTY()
   FTATPlayerSaveData _playerSaveData;

   UPROPERTY()
   FTATUXSettings _uxSaveData;

   UPROPERTY()
   FGuid _temporaryPlayerAnalyticsId; //< to be replaced by wizards persona id
   
   UPROPERTY()
   bool _hasShownAnalyticsPrompt { false };
   
   UPROPERTY()
   bool _hasSentUnlocksAnalytics { false }; // We want to back populate the unlocks players have already got (before 2026/05/27) - if this flag is set to false in the save, then send the data out
};
