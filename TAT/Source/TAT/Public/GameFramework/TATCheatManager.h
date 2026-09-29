// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// ose
#include "GameFramework/OSECheatManager.h"

#include "TATCheatManager.generated.h"

enum class ETATSavedFtueState : uint8;
enum class ETATCharacter : uint8;

class UTATItemInfo;
class ATATPlayerState;
class UToolComponent;
class UTATHubInventoryComponent;
struct FTATLootIdentifier;
class ATATWeatherManager;
struct FTATLootInfo;

UCLASS()
class TAT_API UTATCheatManager : public UOSECheatManager
{
   GENERATED_BODY()
   
public:
   static const FString kTATCheatPrefix;

   // from UCheatManager
   virtual void InitCheatManager() override;
   virtual bool ProcessConsoleExec(const TCHAR* cmd, FOutputDevice& ar, UObject* executor) override;

   UFUNCTION(Exec, BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void KnockOutPlayer(int playerNumber, float numSeconds = -1.0f);
   virtual void KnockOutPlayer_Implementation(int playerNumber, float numSeconds = -1.0f) { };

   /// By default, toggle invulnerability. If 0 or 1, clear and set invulnerability respectively. Player number starts at 0.
   UFUNCTION(Exec, BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SetPlayerInvulnerable(int playerNumber, int value = -1);
   virtual void SetPlayerInvulnerable_Implementation(int playerNumber, int value = -1) { };

   /// By default, toggle undetectability. If 0 or 1, clear or set it respectively. Player number starts at 0.
   UFUNCTION(Exec, BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SetPlayerUndetectable(int playerNumber, int value = -1);
   virtual void SetPlayerUndetectable_Implementation(int playerNumber, int value = -1) { };

   /// Enables/disables stamina consumption for all players. If 1 or 0, enable/disable it respectively. Default value (-1) toggles consumption on/off
   UFUNCTION(Exec, BlueprintAuthorityOnly)
   void SetStaminaConsumptionEnabled(int value = -1);

   /// Enables/disables stamina consumption for the specified player. If 1 or 0, enable/disable it respectively. Default value (-1) toggles consumption on/off
   UFUNCTION(Exec, BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SetPlayerStaminaConsumptionEnabled(int playerNumber, int value = -1);
   void SetPlayerStaminaConsumptionEnabled_Implementation(int playerNumber, int value = -1) { };

   UFUNCTION(Exec, BlueprintNativeEvent, Category = "Cheat Manager|TAT")
   void MaterialInfo();
   virtual void MaterialInfo_Implementation() { };

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void MissionSetRandomSeed(int32 randomSeed);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintKnownClues();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintPlayerStats();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintSessionStats();

   UFUNCTION(Exec)
   void DeleteSaveAndQuit(bool force);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UnlockUpgrade(FString upgradeTagName, int level = 1, bool autoEquip = true);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void EquipChildUpgrade(FString upgradeTagName);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void EquipLoadoutUpgrade(const FGameplayTag& upgradeTag);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void RemoveUpgrades();
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void RemoveUpgradesWithRefund();
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void RemoveUpgrade(FString upgradeTagName);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ListUnlockedUpgrades();
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ListAllUpgrades();
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ForceRefreshUpgradeState();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void AddMoney(int32 amount);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ClearMoney();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void SetXP(int32 totalXP);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void AddXP(int32 amountXP);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintCurrentXP();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void AddUpgradeCurrency(FString currency, int32 amount = 1);
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ClearAllUpgradeCurrency();

   // Adds a tool to the current character's toolset
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void AddTool(const FString& toolName);

   // Removes a tool from the current character's toolset
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RemoveTool(const FString& toolName);

   // Refills ammo just for the currently equipped tool (if there is one)
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RefillAmmoForCurrentTool();

   // Refills ammo for all tools the current player has
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RefillAllAmmo();

   // Adds an item to the current character inventory in a mission
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void AddItem(FName itemName, int32 quantity = 1);

   // Adds a loot item to the current character's saved loot inventory
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void AddSavedLoot(FName lootTag, int32 quantity = 1);

   // Adds the desired number of loot items to the stash
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void AddStashedLoot(FName lootTag, int32 quantity = 1);

   // clears save for currently selected character. Use with care
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ClearCurrentCharacterProgression();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintCurrentCharacterProgression();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ForceVariant(const FString& variantName);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ClearVariantOverrides();

   // Spawns an NPC in front of you.
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SpawnNPC(const FString& className);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void PretendWasMatchmade();

   // Find NPC types containing the query string that can be spawned with the SpawnNPC cheat.
   // If the query string is empty, finds all NPC types.
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FindNPCTypes(const FString& query);

   // Spawns a loot actor in front of the player. Loot tags are defined in loot data tables.
   // See also: Project Settings -> TAT Loot Settings -> Loot Data Table
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SpawnLoot(FName lootTag);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void DropMajorLoot();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void LootPinata();

   UFUNCTION(Exec)
   void TutorialJump(int stepNumber);
   
   UFUNCTION(Exec, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void AddToken(const FString& tokenClassName);

   // Removes all inventory tokens
   UFUNCTION(Exec, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ClearTokens();

   UFUNCTION(Exec, Category = "Cheat Manager|TAT")
   void FreezeInteractTargeting();

   // Removes all inventory tokens
   UFUNCTION(Exec, Category = "Cheat Manager|TAT")
   void ClearInteractTargetingFreeze();

   // Selects contract in lobby
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractSelect(const FString& questTag);

   // Selects mission in lobby
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void MissionSelect(const FString& questTag);

   // puts quest in the active state
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractIntro(const FString& questTag);

   // puts quest in the active state
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractTrigger(const FString& questTag);
   
   // puts quest in the outro state, if it can be
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractOutro(const FString& questTag);

   // puts quest in the unstarted state
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractForget(const FString& questTag);

   // puts quest in the completed state without changing anything else
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractComplete(const FString& questTag);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PrintObjective();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ContractClearAll();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void CompleteObjective();

   UFUNCTION(Exec, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void DumpQuestGraph();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ChangeCharacter(int characterNum);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ToggleAIDebugHUD();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ToggleDisableLockpickingMinigame();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category="Cheat Manager|TAT")
   void ForceSoloEscape();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category="Cheat Manager|TAT")
   void ForceSoloCaught();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category="Cheat Manager|TAT")
   void ForceTeamEscape();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category="Cheat Manager|TAT")
   void ForceTeamCaught();
   
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category="Cheat Manager|TAT")
   void ForceEndMatch();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void EscapeOpen(bool force = false);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void EscapeOpenAll(bool force = false);

   // Sets the current phase time to the specifed time value.
   // If there is a colon in the time parameter, it's parsed as MM:SS, otherwise it's parsed as an integer (number of seconds).
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SetPhaseTimer(const FString& time);

   UFUNCTION(Exec, Category = "Cheat Manager|TAT")
   void PrintPhase();

   UFUNCTION(Exec, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void StartEndgame(const FString& possibleTime);

   UFUNCTION(Exec, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void StartEndgameTimer(const FString& possibleTime);

   virtual void DamageTarget(float damageAmount) override;

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void DamageSelf(float damageAmount, FName damageType = NAME_None);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RepairTarget();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RevealCombination();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ForceUnlock();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void Toast(const FString& text, FName toastId = NAME_None);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ToastBroadcast(const FString& text, FName toastId = NAME_None, bool reliable = false);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void SpawnThiefVisionIndicator(FName indicatorId, float deduplicateDistance = 0.0f, int32 count = 1);

   // Removes all indicators being managed by the thief vision system that were caused by the current player
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RemoveMyThiefVisionIndicators();

   // Removes all indicators being managed by the thief vision system
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void RemoveAllThiefVisionIndicators();

   // Toggle the glyph indicator debug mode
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void GlyphDebug();

   // Sets a new weather type. Not networked - only changes weather for the local player
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void WeatherType(const FString& weatherTypeTag);

   // Sets a new weather preset class. Not networked - only changes weather for the local player
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void WeatherPreset(const FString& presetClassName);

   // Asks the weather manager to recapture the level depthmap
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void RefreshLevelDepthmap();

   // Asks the weather manager to toggle the temporal trace debug visualization
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void DebugTemporalTraces();

   // Set or get a match setting by name.
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void MatchSetting(FName propName, const FString& newValue = TEXT(""));

   // Dump all match settings to the console (server)
   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ListMatchSettings();

   // Dump all match settings to the console (reading the values from the local client, not the server)
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ListMatchSettingsLocal();

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void PowerSurge(float durationSeconds = 3.0f);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void PowerNetworkDebugger();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void DebugHUDIndicators();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FtueSetSavedState(ETATSavedFtueState state);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FtuePrintSavedState();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FtueReset();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FtueSkip();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UnlockAdd(const FString& unlockableTagString);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UnlockRemove(const FString& unlockableTagString);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UnlockResetAll();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UnlockResetSeen();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UserSettingSetFloat(const FString& settingTag, float newValue);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void UserSettingSetBool(const FString& settingTag, bool newValue);

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ZZZ_CrashTheGame_NullPtr();
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void ZZZ_CrashTheGame_Assert();

   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void FastForward(const FString& duration, float rate=30);

   UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Cheat Manager|TAT")
   void ResetRespawnTotem();

   /// Toggle the dev tools UI
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void DevTools();

   /// Toggle a specific dev tool by name
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void DevTool(const FString& name);
   
   /// Gets the name of all registered dev tools
   UFUNCTION(Exec, BlueprintCallable, Category = "Cheat Manager|TAT")
   void DevToolsList();

protected:
   
   UFUNCTION(BlueprintImplementableEvent)
   void _GrantPostFtueState();

   virtual bool DoGameSpecificBugItLog(FOutputDevice& outputFile) override;

protected:
   void _OnFastForwardProgress();
   void _OnFastForwardComplete();
   TArray<APlayerController*> _CollectControllersOnPlayerTeam() const;

   ATATPlayerState* _GetOwningPlayerState() const;

   TSubclassOf<UTATItemInfo> _LoadItemForName(FName name) const;

   /// Iterates over all blueprint assets with a name that contains query as a substring (case-insensitive).
   /// Calls the callback for each class. If the callback returns false, stops iterating.
   /// Useful for cheats that search for blueprints of a specific type, or for finding a blueprint to spawn using fuzzy name matching.
   static void _ForEachBlueprintClassMatchingQuery(UClass* baseClass, const FString& query, TFunctionRef<bool(UClass*)> callback, TConstArrayView<FString> topLevelDirs = {});

   template<typename T>
   static void _ForEachBlueprintClassMatchingQuery(const FString& query, TFunctionRef<bool(TSubclassOf<T>)> callback, TConstArrayView<FString> topLevelDirs = {})
   {
      return _ForEachBlueprintClassMatchingQuery(T::StaticClass(), query, [callback](UClass* cls) -> bool { return callback(TSubclassOf<T>{ cls }); }, topLevelDirs);
   }

   static UClass* _FindBlueprintClassByName(const FString& name, UClass* baseClass);
   
   template<typename T>
   static TSubclassOf<T> _FindBlueprintClassByName(const FString& name)
   {
      return _FindBlueprintClassByName(name, T::StaticClass());
   }
   static UObject* _FindAssetByName(FStringView name, UClass* assetClass);
   template<typename T>
   static T* _FindAssetByName(FStringView name)
   {
      return Cast<T>(_FindAssetByName(name, T::StaticClass()));
   }

   TSubclassOf<UToolComponent> _LoadToolByName(const FString& toolName) const;

   const FTATLootInfo* _FindLootInfoByTag(FName lootTag);

   TSoftObjectPtr<class UTATSceneVariantConfig> _FindVariantForName(const FString& variantName) const;

   FString _GetPlayerName(APlayerController* pc = nullptr) const;

   // Cheat helper function that spawns an actor in front of the player.
   AActor* _SpawnActorForPlayer(UClass* actorClass, float dropDistance = 0.0f) const;

   // Does a line trace forward and returns the actor that the player is looking at
   AActor* _LineTracePlayerViewTarget(
      float maxDistance = 500.0f,
      ECollisionChannel collisionChannel = ECC_WorldDynamic,
      FHitResult* outHitResult = nullptr,
      bool drawDebug = false,
      const FCollisionQueryParams& queryParams = FCollisionQueryParams::DefaultQueryParam,
      const FCollisionResponseParams& responseParams = FCollisionResponseParams::DefaultResponseParam) const;

   // Does a sphere overlap, calls the predicate for each result, and returns the first actor for which the predicate returns true.
   AActor* _FindNearbyActorMatchingPredicate(float maxDistance, TFunctionRef<bool(AActor*)> predicate, ECollisionChannel collisionChannel = ECC_WorldDynamic, ECollisionResponse collisionResponse = ECR_Block) const;

   // Parses a duration value that may be in the form "MM:SS" or "SS" and return a value in seconds.
   static TOptional<int32> _ParseDurationToSeconds(const FString& text);

   // Finds a gameplay tag with the expected prefix.
   // If tagName is not valid or does not have the prefix, it will try again with the prefix inserted (this allows players to omit tag prefixes in cheats).
   static TOptional<FGameplayTag> _FindGameplayTag(const FString& tagName, FStringView tryWithPrefix = {}, TOptional<FGameplayTag> requiredParentTag = NullOpt, FString* outError = nullptr);

   UTATHubInventoryComponent* _GetHubInventory() const;
   void _PrintToConsole(const FString& string, bool isError = false);
   FORCEINLINE void _PrintErrorToConsole(const FString& string) { _PrintToConsole(string, true); }

   FTimerHandle _fastForwardTimerHandle;
   FTimerHandle _fastForwardProgressHandle;
};
