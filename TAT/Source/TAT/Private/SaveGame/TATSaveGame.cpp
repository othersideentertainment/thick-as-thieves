// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/TATSaveGame.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Matchmaking/TATScoringSettings.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "Analytics/TATAnalyticsManager.h"

// ose
#include "Identity/OSESaveGameSystem.h"

// ue4
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSaveGame)

static const int kCurrentSaveVersion = 7; // bump this rev to modify existing save data in place after loading
static const int kCurrentClearVersion = 5; // bump this rev to completely wipe save data after loading

DEFINE_LOG_CATEGORY_STATIC(LogTATSaveGame, Log, All);

namespace SaveCvars
{
   // TODO (2022-03-16): just delete this variable
   static int32 UseAllUnlockedUpgrades = 0;
   FAutoConsoleVariableRef CVarEnableClearanceCheck(
      TEXT("TAT.Upgrade.UseAllUnlockedUpgrades"),
      UseAllUnlockedUpgrades,
      TEXT("Used all unlocked upgrades instead of just the equipped ones"),
      ECVF_Default);
}

bool FTATContractWithStatus::operator==(const FGameplayTag& tag) const
{
   return ContractTag == tag;
}

void FTATContractWithStatus::AppendToString(FString& result) const
{
   result.Appendf(TEXT("(%s: %s)"), *ContractTag.ToString(), *StaticEnum<ETATContractState>()->GetNameStringByValue(static_cast<int64>(State)));
}

////////////////////////////////////////////////////////////////
//                   FTATPlayerProgression
////////////////////////////////////////////////////////////////

const FTATCharacterProgression* FTATPlayerProgression::TryGetCharacterProgression(FTATCharacterSaveId character) const
{
   for (const FTATCharacterProgression& characterProg : CharacterProgression)
   {
      if (characterProg.Character == character.Character)
      {
         return &characterProg;
      }
   }
   return nullptr;
}

void FTATPlayerProgression::InitLastActiveContract()
{
   const int32 index = Contracts.FindLastByPredicate([](const FTATContractWithStatus& contract)
   {
      return contract.State == ETATContractState::Objective;
   });

   if (index != INDEX_NONE)
   {
      LastActiveContract = Contracts[index].ContractTag;
   }
}

void FTATPlayerProgression::AppendToDebugString(FString& result) const
{
   result.Append(TEXT("\nContracts:"));
   for (const FTATContractWithStatus& quest : Contracts)
   {
      result.Append(TEXT("\n"));
      quest.AppendToString(result);
   }
   result.Append(TEXT("\nUnlocks:"));
   for (const FGameplayTag& unlock : UnlockedContent)
   {
      result.Append(TEXT("\n"));
      unlock.GetTagName().AppendString(result);
   }
}

void FTATPlayerProgression::Reset()
{
   Money = 0;
   Contracts.Reset();
   LastActiveContract = FGameplayTag();
   UnlockedContent.Reset();
   SeenUnlockableContent.Reset();
   SelectedDifficulty = NullOpt;
}

ETATContractState FTATPlayerProgression::GetContractState(FGameplayTag tag) const
{
   if (const FTATContractWithStatus* quest = Contracts.FindByKey(tag))
   {
      return quest->State;
   }

   return ETATContractState::Unstarted;
}

bool FTATPlayerProgression::SetContractState(FGameplayTag contractTag, ETATContractState state)
{
   if (!contractTag.IsValid())
   {
      return false;
   }

   if (FTATContractWithStatus* quest = Contracts.FindByKey(contractTag))
   {
      if (quest->State != state)
      {
         quest->State = state;
         return true;
      }
   }
   else if (state != ETATContractState::Unstarted)
   {
      Contracts.Add({ contractTag, state });
      return true;
   }

   return false;
}

////////////////////////////////////////////////////////////////
//                   FTATCharacterProgression
////////////////////////////////////////////////////////////////

void FTATCharacterProgression::GetUpgradeState(FUpgradeState& result, bool includeIntrinsicUpgrades) const
{
   if (SaveCvars::UseAllUnlockedUpgrades)
   {
      result = UnlockedUpgrades;
   }
   else
   {
      result.Values.Empty(EquippedUpgrades.Num() + (EquippedLoadoutSkill.IsValid() ? 1 : 0));
      for (const FGameplayTag& tag : EquippedUpgrades)
      {
         result.Values.Add(tag, UnlockedUpgrades.GetValue(tag));
      }

      if (EquippedLoadoutSkill.IsValid())
      {
         result.Values.Add(EquippedLoadoutSkill, UnlockedUpgrades.GetValue(EquippedLoadoutSkill));
      }
   }

   // Add intrinsic upgrades, but don't override upgrades the character has unlocked (if any)
   if (includeIntrinsicUpgrades)
   {
      if (UTATCharactersMetadata* charactersMetadata = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset())
      {
         for (const auto& pair : charactersMetadata->GetCharacterMetadata(Character).IntrinsicUpgrades)
         {
            if (!result.Values.Contains(pair.Key))
            {
               result.Values.Add(pair);
            }
         }
      }
   }
}

int32 FTATCharacterProgression::GetUpgradeValue(FGameplayTag upgradeTag, int32 fallbackValue, bool includeIntrinsicUpgrades) const
{
   if (const int32* value = UnlockedUpgrades.Values.Find(upgradeTag))
   {
      return *value;
   }

   if (includeIntrinsicUpgrades)
   {
      if (UTATCharactersMetadata* charactersMetadata = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset())
      {
         if (const int32* value = charactersMetadata->GetCharacterMetadata(Character).IntrinsicUpgrades.Find(upgradeTag))
         {
            return *value;
         }
      }
   }

   return fallbackValue;
}

FTATCharacterLoadout* FTATCharacterProgression::GetLoadout(ETATLoadoutType type)
{
   if (type == ETATLoadoutType::Tool)
   {
      return &GearLoadout;
   }
   if (type == ETATLoadoutType::Ability)
   {
      return &AbilityLoadout;
   }
   if (type == ETATLoadoutType::Outfit)
   {
      return &OutfitLoadout;
   }

   return nullptr;
}

const FTATCharacterLoadout* FTATCharacterProgression::GetLoadout(ETATLoadoutType type) const
{
   return const_cast<FTATCharacterProgression*>(this)->GetLoadout(type);
}

void FTATCharacterProgression::UpdatePerformanceRankingForMatch(const FMatchPersistentData& matchPersistentData)
{
   checkf(MatchCount > 0, TEXT("Unexpected match count (%d)!"), MatchCount);

   // Incorporate the new value into the prior average (weighted towards performance in last N games)
   const UTATScoringSettings& scoringSettings = UTATScoringSettings::Get();
   const int32 clampedMatchCountRange = FMath::Min(MatchCount, scoringSettings.RankingMatchCountMemory);
   check(clampedMatchCountRange > 0);
   const float alpha = 1.f / clampedMatchCountRange;
   auto updateAverageValue = [&](float& originalAverage, float latestValue, const TCHAR* valueName)
   {
      const float newAverage = FMath::Lerp(originalAverage, latestValue, alpha);
      UE_LOG(LogTATSaveGame, Verbose, TEXT("%s updated (%f -> %f)"), valueName, originalAverage, newAverage);
      originalAverage = newAverage;
   };

   updateAverageValue(PerformanceRanking.AvgLootValueCarriedOut, matchPersistentData.KeptCarriedLootValue, GET_MEMBER_NAME_STRING_CHECKED(FTATPlayerPerformanceRankingData, AvgLootValueCarriedOut));
   updateAverageValue(PerformanceRanking.AvgLootValueStashed, matchPersistentData.StashedLootValue, GET_MEMBER_NAME_STRING_CHECKED(FTATPlayerPerformanceRankingData, AvgLootValueStashed));

   const bool didEscape = matchPersistentData.CompletionState == EMatchCompletionState::Escaped;
   updateAverageValue(PerformanceRanking.SuccessfulEscapeRatioNormalized, didEscape ? 1.f : 0.f, GET_MEMBER_NAME_STRING_CHECKED(FTATPlayerPerformanceRankingData, SuccessfulEscapeRatioNormalized));

   // Only update quest-completion averages if a quest was actually selected
   if (matchPersistentData.MissionResult.QuestTag.IsValid())
   {
      updateAverageValue(PerformanceRanking.MissionCompletionRate, matchPersistentData.MissionResult.IsObjectiveComplete ? 1.f : 0.f, GET_MEMBER_NAME_STRING_CHECKED(FTATPlayerPerformanceRankingData, MissionCompletionRate));
   }
   if (matchPersistentData.ContractResult.QuestTag.IsValid())
   {
      updateAverageValue(PerformanceRanking.ContractCompletionRate, matchPersistentData.ContractResult.IsObjectiveComplete ? 1.f : 0.f, GET_MEMBER_NAME_STRING_CHECKED(FTATPlayerPerformanceRankingData, ContractCompletionRate));
   }
}

void FTATCharacterProgression::AppendToDebugString(FString& result) const
{
   result.Appendf(TEXT("Character: %s"), *StaticEnum<ETATCharacter>()->GetDisplayNameTextByValue((int64)Character).ToString());
   for (const auto& pair : UpgradeCurrencies.Values)
   {
      result.Appendf(TEXT("\nUpgradeCurrencies[%s]: %d"), *pair.Key.ToString(), pair.Value);
   }
   result.Appendf(TEXT("\nMatchCount: %d"), MatchCount);

   TStringBuilder<96> gearLoadoutArray;
   for (int32 i = 0; i < GearLoadout.Entries.Num(); i++)
   {
      if (i > 0)
      {
         gearLoadoutArray.Append(TEXT(", "));
      }
      gearLoadoutArray.Append(*GearLoadout.Entries[i].LoadoutTag.ToString());
   }
   result.Appendf(TEXT("\nLoadout[%d]: [ %s ]"), GearLoadout.Entries.Num(), *gearLoadoutArray);

   result.Append(TEXT("\n"));
   Loot.AppendToString(result);
}

void FTATCharacterProgression::Reset()
{
   const ETATCharacter characterToRestore = Character;
   const FOnDataChangedEvent lootChangedEventToRestore = OnLootChanged;
   const FTATUpgradeCurrencyChanged upgradeCurrencyChangedEventToRestore = OnUpgradeCurrencyChanged;
   TArray<FGameplayTag, TInlineAllocator<8>> previousCurrencyTags;
   previousCurrencyTags.Reserve(UpgradeCurrencies.Values.Num());
   for (const auto& pair : UpgradeCurrencies.Values)
   {
      previousCurrencyTags.Add(pair.Key);
   }

   *this = FTATCharacterProgression();
   Character = characterToRestore;
   OnLootChanged = lootChangedEventToRestore;
   OnUpgradeCurrencyChanged = upgradeCurrencyChangedEventToRestore;

   OnLootChanged.Broadcast();
   for (FGameplayTag currencyTag : previousCurrencyTags)
   {
      OnUpgradeCurrencyChanged.Broadcast(currencyTag, 0);
   }
}

////////////////////////////////////////////////////////////////
//                   UTATSaveGame
////////////////////////////////////////////////////////////////

UTATSaveGame* UTATSaveGame::GetTATSaveGame(const UObject* contextObj)
{
   if (IsValid(contextObj))
   {
      if (UWorld* world = contextObj->GetWorld())
      {
         if (UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(contextObj))
         {
            return Cast<UTATSaveGame>(saveGameSystem->GetSaveData());
         }
      }
      else
      {
         UE_LOG(LogTATSaveGame, Warning, TEXT("Context object has no world!  Cannot look up save game."));
      }
   }
   else
   {
      UE_LOG(LogTATSaveGame, Warning, TEXT("Cannot look up save game from null/invalid context object!"));
   }
   return nullptr;
}

UWorld* UTATSaveGame::GetWorld() const
{
   if (_saveGameSystem != nullptr)
   {
      return _saveGameSystem->GetWorld();
   }
   return nullptr;
}

UTATAnalyticsManager* UTATSaveGame::GetAnalyticsManager() const
{
   const auto world = GetWorld();
   if (world == nullptr)
      return nullptr;

   const auto gameInstance = world->GetGameInstance();
   if (gameInstance == nullptr)
      return nullptr;
        
   return gameInstance->GetSubsystem<UTATAnalyticsManager>();
}

void UTATSaveGame::_OnSaveDataLoaded()
{
   // ensure character save data map exists for all characters (support for newly added characters)
   for (int charIdx = 0; charIdx < int(ETATCharacter::MAX); ++charIdx)
   {
      ETATCharacter character = ETATCharacter(charIdx);

      bool charProgressionExists = false;
      for (const FTATCharacterProgression& charProgression : _playerSaveData.PlayerProgression.CharacterProgression)
      {
         if (charProgression.Character == character)
         {
            charProgressionExists = true;
            break;
         }
      }

      if (!charProgressionExists)
      {
         FTATCharacterProgression newCharProgression = { character };
         _playerSaveData.PlayerProgression.CharacterProgression.Add(newCharProgression);
      }
   }

   for (FTATCharacterProgression& charProgression : _playerSaveData.PlayerProgression.CharacterProgression)
   {
      const FTATCharacterDataContext context{ this, FTATCharacterSaveId{ charProgression.Character } };

      charProgression.Loot.Init();

      // Make sure all customized loadouts are still valid
      if (!UTATCharacterMetadataFunctionLibrary::IsValidLoadoutForCharacter(context, ETATLoadoutType::Tool, charProgression.GearLoadout))
      {
         //TODO: We should bubble up an event into the UI here to notify the player that their saved loadout is no longer valid and has been reset.
         UE_LOG(LogTATSaveGame, Warning, TEXT("Character %s does not have a valid loadout. It will be reset to the default"),
                *StaticEnum<ETATCharacter>()->GetNameStringByValue(static_cast<int64>(charProgression.Character)));

         const bool loadedDefaultLoadout = UTATCharacterMetadataFunctionLibrary::GetDefaultLoadoutForCharacter(context, ETATLoadoutType::Tool, charProgression.GearLoadout);
         if (!ensure(loadedDefaultLoadout))
         {
            charProgression.GearLoadout = FTATCharacterLoadout{};
         }
      }
   }

   _playerSaveData.PlayerProgression.InitLastActiveContract();

   auto initTags = [] (TArray<FGameplayTag>& outTags, TConstArrayView<FGameplayTag> tagsToAdd)
   {
      for (FGameplayTag tag : tagsToAdd)
      {
         outTags.AddUnique(tag);
      }
   };
   // make sure initially unlocked content is unlocked
   // There is an argument that this is better not even being written,
   // but this is slightly simpler. That may not be the case post-prologue
   // where the remedies for being out of sync are less straightforward to
   // address.
   // NOTE: We do want to make sure we are very constrained in how much configuration data
   //       the save game has access to, so that it doesn't eat business logic in a way that
   //       becomes trickier to untangle. This initialization seems like a reasonable exception.
   initTags(_playerSaveData.PlayerProgression.UnlockedContent, UTATProjectSettings::Get().InitialUnlockedContent);
   if (_playerSaveData.FtueState.State == ETATSavedFtueState::Complete)
   {
      // If past the ftue, make sure content unlocked by the ftue is unlocked
      initTags(_playerSaveData.PlayerProgression.UnlockedContent, UTATProjectSettings::Get().PostFtueUnlockedContent);
   }
}

void UTATSaveGame::_OnSaveDataCreated()
{
   _temporaryPlayerAnalyticsId = FGuid::NewGuid();
   _OnSaveDataLoaded();
   _MarkDirty(EOSESavePriority::LowPriority);
}

int UTATSaveGame::GetSaveCurrentVersion() const
{
   // bump this rev to modify existing save data in place after loading
   return kCurrentSaveVersion;
}

int UTATSaveGame::GetSaveCurrentClearVersion() const
{
   // bump this rev to completely wipe save data after loading
   return kCurrentClearVersion;
}

void UTATSaveGame::_FixupSaveVersion()
{
   if (_saveVersion < 3)
   {
      // v3, reset player save data to wipe upgrades etc from previous builds. TODO: integrate a kClearVersion that we can bump instead of writing this code for each clear
      _playerSaveData = FTATPlayerSaveData();
   }
   if (_saveVersion < 6)
   {
      // v6, generate player id
      _temporaryPlayerAnalyticsId = FGuid::NewGuid();
   }
   _playerSaveData.PlayerProgression.XP.FixLevelVersions();
   _saveVersion = GetSaveCurrentVersion();
}

ETATCharacter UTATSaveGame::GetCharacterType(FTATCharacterSaveId character)
{
   return character.Character;
}

bool UTATSaveGame::IsCharacterSaveIdValid(const FTATCharacterSaveId& characterSaveId)
{
   return characterSaveId.IsValid();
}

void UTATSaveGame::SetSelectedCharacter(FTATCharacterSaveId character)
{
   if (_playerSaveData.SelectedCharacter != character.Character)
   {
      _playerSaveData.SelectedCharacter = character.Character;
      _MarkDirty(EOSESavePriority::LowPriority);
      OnSelectedCharacterChanged.Broadcast(character.Character);
   }
}

FTATCharacterSaveId UTATSaveGame::GetSelectedCharacter() const
{
   return FTATCharacterSaveId::FromCharacterType(_playerSaveData.SelectedCharacter);
}

void UTATSaveGame::SetHasShownAnalyticsPrompt()
{
   _hasShownAnalyticsPrompt = true;
   _MarkDirty(EOSESavePriority::HighPriority);
}

void UTATSaveGame::SetHasSentUnlocksAnalytics()
{
   _hasSentUnlocksAnalytics = true;
   _MarkDirty(EOSESavePriority::HighPriority);
}

const FTATPlayerProgression& UTATSaveGame::GetPlayerProgression() const
{
   return _playerSaveData.PlayerProgression;
}

const FTATPlayerExperience& UTATSaveGame::GetXP() const
{
   return GetPlayerProgression().XP;
}

void UTATSaveGame::SetXP(int32 totalXP)
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   
   const int previousLevel = playerData.XP.Level;
   playerData.XP.SetXP(totalXP);
   if (playerData.XP.Level != previousLevel)
   {
      if (UTATAnalyticsManager* analyticsManager = GetAnalyticsManager())
      {
         analyticsManager->HandlePlayerLevelChanged(playerData.XP.Level);
      }
   }
   
   OnXPProgressionChanged.Broadcast();
   _MarkDirty(EOSESavePriority::HighPriority);
}

void UTATSaveGame::AddXP(int32 amountXP)
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   const int previousLevel = playerData.XP.Level;
   playerData.XP.AddXP(amountXP);
   if (playerData.XP.Level != previousLevel)
   {
      if (UTATAnalyticsManager* analyticsManager = GetAnalyticsManager())
      {
         analyticsManager->HandlePlayerLevelChanged(playerData.XP.Level);
      }
   }
   
   OnXPProgressionChanged.Broadcast();
   _MarkDirty(EOSESavePriority::HighPriority);
}

bool UTATSaveGame::HasUnlockedContent(FGameplayTag unlockableTag) const
{
   return unlockableTag.IsValid() && GetPlayerProgression().UnlockedContent.Contains(unlockableTag);
}

void UTATSaveGame::AddUnlocks(TConstArrayView<FGameplayTag> unlockTags)
{
   bool dirty = false;
   bool seenDirty = false;
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   for (const FGameplayTag& tag : unlockTags)
   {
      if (!playerData.UnlockedContent.Contains(tag))
      {
         playerData.UnlockedContent.Add(tag);
         dirty = true;

         if (playerData.SeenUnlockableContent.RemoveSingleSwap(tag))
         {
            seenDirty = true;
         }
      }
   }

   if (dirty)
   {
      _MarkDirty(EOSESavePriority::HighPriority);
      OnUnlocksChanged.Broadcast();

      if (seenDirty)
      {
         OnSeenUnlocksChanged.Broadcast();
      }
   }
}

void UTATSaveGame::AddUnlock(FGameplayTag unlockTag)
{
   AddUnlocks(TConstArrayView<FGameplayTag>({unlockTag}));
}

void UTATSaveGame::RemoveUnlocks(TConstArrayView<FGameplayTag> tagsToRemove)
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   const bool dirty = playerData.UnlockedContent.RemoveAll([tagsToRemove](const FGameplayTag& tag) { return tagsToRemove.Contains(tag);}) > 0;
   if (dirty)
   {
      _MarkDirty(EOSESavePriority::HighPriority);
      OnUnlocksChanged.Broadcast();
   }
}

void UTATSaveGame::RemoveUnlock(FGameplayTag tagToRemove)
{
   RemoveUnlocks(TConstArrayView<FGameplayTag>({tagToRemove}));
}

void UTATSaveGame::ClearUnlocks()
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   if (!playerData.UnlockedContent.IsEmpty())
   {
      playerData.UnlockedContent.Reset();
      _MarkDirty(EOSESavePriority::HighPriority);
      OnUnlocksChanged.Broadcast();
   }
}

void UTATSaveGame::MarkUnlocksSeen(TConstArrayView<FGameplayTag> unlockTags)
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   bool dirty = false;
   for (const FGameplayTag& tag : unlockTags)
   {
      if (!playerData.SeenUnlockableContent.Contains(tag))
      {
         playerData.SeenUnlockableContent.Add(tag);
         dirty = true;
      }
   }

   if (dirty)
   {
      _MarkDirty(EOSESavePriority::LowPriority);
      OnSeenUnlocksChanged.Broadcast();
   }
}

void UTATSaveGame::ClearSeenUnlocks()
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   if (!playerData.SeenUnlockableContent.IsEmpty())
   {
      playerData.SeenUnlockableContent.Reset();
      OnSeenUnlocksChanged.Broadcast();
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

void UTATSaveGame::ResetUnlocks()
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   if (!playerData.UnlockedContent.IsEmpty())
   {
      // NOTE: be careful about how many uses of design data there are
      playerData.UnlockedContent = UTATProjectSettings::Get().InitialUnlockedContent;
      _MarkDirty(EOSESavePriority::HighPriority);
      OnUnlocksChanged.Broadcast();
   }
   ClearSeenUnlocks();
}

const TArray<FTATContractWithStatus>& UTATSaveGame::GetContracts() const
{
   return GetPlayerProgression().Contracts;
}

void UTATSaveGame::FindContractsInState(ETATContractState state, TArray<FGameplayTag>& contracts) const
{
   const FTATPlayerProgression& playerProgression = GetPlayerProgression();

   contracts.Reset();
   for (const FTATContractWithStatus& contract : playerProgression.Contracts)
   {
      if (contract.State == state)
      {
         contracts.Add(contract.ContractTag);
      }
   }
}

FGameplayTag UTATSaveGame::GetLastActiveContract() const
{
   return GetPlayerProgression().LastActiveContract;
}

ETATContractState UTATSaveGame::GetContractState(FGameplayTag contractTag) const
{
   return GetPlayerProgression().GetContractState(contractTag);
}

void UTATSaveGame::_HandleContractStateChangeAnalytics(const FGameplayTag contractTag, const ETATContractState state) const
{
   if (UTATAnalyticsManager* analyticsManager = GetAnalyticsManager())
   {
      FTATAnalyticsCustomFields analyticsFields;
      const FString stateString = UEnum::GetValueAsString(state);
      analyticsFields.Set(TEXT("State"), stateString);
      analyticsFields.Set(TEXT("Contract"), contractTag);
      analyticsManager->OnDesignEventWithCustomFields(TEXT("ContractStateChange"), analyticsFields);
   }
}

void UTATSaveGame::SetContractState(FGameplayTag contractTag, ETATContractState state)
{
   FTATPlayerProgression& playerProgression = _GetPlayerProgressionMutable();
   if (playerProgression.SetContractState(contractTag, state))
   {
      _HandleContractStateChangeAnalytics(contractTag, state);
      _MarkDirty(EOSESavePriority::HighPriority);
      if (GetLastActiveContract() == contractTag && state != ETATContractState::Objective)
      {
         // If our Last Active Contract moved out of the Objective state,
         // then clear it since only Contracts in the Objective state should be Active
         _SetLastActiveContract(FGameplayTag());
      }
      else if (state == ETATContractState::Objective)
      {
         _SetLastActiveContract(contractTag);
      }
      OnContractsChanged.Broadcast();
   }
}

void UTATSaveGame::ClearContracts()
{
   FTATPlayerProgression& playerProgression = _GetPlayerProgressionMutable();
   if (playerProgression.Contracts.Num())
   {
      playerProgression.Contracts.Reset();
      playerProgression.LastActiveContract = FGameplayTag();
      OnLastActiveContractSet.Broadcast(FGameplayTag());
      OnContractsChanged.Broadcast();
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

ETATFeatureAvailabilityToPlayer UTATSaveGame::GetAvailabilityFromUnlockCondition(const FTATUnlockCondition& unlockCondition) const
{
   switch (unlockCondition.UnlockType)
   {
      case ETATUnlockType::RequiredLevel:
         if (GetXP().Level >= unlockCondition.RequiredLevel)
         {
            return ETATFeatureAvailabilityToPlayer::Available;
         }
         return ETATFeatureAvailabilityToPlayer::Locked;
      case ETATUnlockType::RequiredContract:
         ETATContractState contractState = GetContractState(unlockCondition.RequiredContractTag);
         switch (contractState)
         {
            case ETATContractState::Unstarted:
               return ETATFeatureAvailabilityToPlayer::Locked;
            case ETATContractState::Intro:
            case ETATContractState::Objective:
               return ETATFeatureAvailabilityToPlayer::Unlockable;
            case ETATContractState::Outro:
            case ETATContractState::Complete:
               return ETATFeatureAvailabilityToPlayer::Available;
         }
   }

   return ETATFeatureAvailabilityToPlayer::Hidden;
}

ETATFeatureAvailabilityToPlayer UTATSaveGame::GetAvailabilityFromUnlockRequirement(const FTATUnlockRequirement& unlockRequirement) const
{
   // Default to available in case there are no unlock conditions
   ETATFeatureAvailabilityToPlayer availabilityFromUnlockRequirement = ETATFeatureAvailabilityToPlayer::Available;

   if (unlockRequirement.UnlockConditions.Num() > 0)
   {
      // If we need to meet all conditions, assume available until proven otherwise
      // If we only need to meet one condition, assume hidden until proven otherwise
      if (!unlockRequirement.MustMeetAllConditions)
      {
         availabilityFromUnlockRequirement = ETATFeatureAvailabilityToPlayer::Hidden;
      }

      for (FTATUnlockCondition unlockCondition : unlockRequirement.UnlockConditions)
      {
         ETATFeatureAvailabilityToPlayer availabilityFromUnlockCondition = GetAvailabilityFromUnlockCondition(unlockCondition);

         if (unlockRequirement.MustMeetAllConditions)
         {
            // A Greater value here means it is further away from being available (Hidden > Locked > Unlockable > Available)
            // If we must meet all conditions, then the highest condition availability will represent our requirement availability
            if (availabilityFromUnlockCondition > availabilityFromUnlockRequirement)
            {
               availabilityFromUnlockRequirement = availabilityFromUnlockCondition;
            }
         }
         else
         {
            // A Lesser value here means it is closer to being available (Available < Unlockable < Locked < Hidden)
            // If we only need to meet one condition, then the lowest condition availability will represent our required availability
            if (availabilityFromUnlockCondition < availabilityFromUnlockRequirement)
            {
               availabilityFromUnlockRequirement = availabilityFromUnlockCondition;
            }
         }
      }
   }

   if (availabilityFromUnlockRequirement == ETATFeatureAvailabilityToPlayer::Locked && unlockRequirement.HiddenIfLocked)
   {
      availabilityFromUnlockRequirement = ETATFeatureAvailabilityToPlayer::Hidden;
   }

   return availabilityFromUnlockRequirement;
}

const FTATCharacterProgression& UTATSaveGame::GetCharacterProgression(FTATCharacterSaveId character) const
{
   // in save data this is always init'd, should never fail the lookup
   const FTATCharacterProgression* charProgression = _playerSaveData.PlayerProgression.TryGetCharacterProgression(character);
   check(charProgression);
   return *charProgression;
}

void UTATSaveGame::UpdateCharacterProgression(FTATCharacterSaveId character, EOSESavePriority priority, TFunctionRef<void(FTATCharacterProgression&)> mutator)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   mutator(characterData);
   _MarkDirty(priority);
}

int32 UTATSaveGame::GetUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag) const
{
   return GetCharacterProgression(character).UpgradeCurrencies.GetValue(currencyTag);
}

void UTATSaveGame::AddUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag, int32 amount)
{
   if (currencyTag.IsValid())
   {
      FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
      const int32 newValue = (characterData.UpgradeCurrencies.Values.FindOrAdd(currencyTag) += amount);
      if (amount != 0)
      {
         _MarkDirty(EOSESavePriority::LowPriority);
      }

      // TODO: be able to defer/buffer this event
      characterData.OnUpgradeCurrencyChanged.Broadcast(currencyTag, newValue);
   }
}

void UTATSaveGame::RemoveUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag, int32 amount)
{
   AddUpgradeCurrency(character, currencyTag, -amount);
}

void UTATSaveGame::ClearUpgradeCurrencies(FTATCharacterSaveId character)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   if (characterData.UpgradeCurrencies.Values.Num() > 0)
   {
      characterData.UpgradeCurrencies.Values.Reset();
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

int32 UTATSaveGame::GetMoney() const
{
   return GetPlayerProgression().Money;
}

void UTATSaveGame::UpdateMoney(int moneyDelta)
{
   if (moneyDelta != 0)
   {
      FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
      playerData.Money += moneyDelta;
      playerData.OnMoneyChanged.Broadcast();
      _MarkDirty(EOSESavePriority::HighPriority);
   }
}

FTATCharacterLoadout UTATSaveGame::GetCharacterLoadout(FTATCharacterSaveId character, ETATLoadoutType loadoutType, bool getDefaultLoadoutIfEmpty) const
{
   const FTATCharacterProgression& progression = GetCharacterProgression(character);
   const FTATCharacterDataContext context{ const_cast<UTATSaveGame*>(this), character };

   const FTATCharacterLoadout* currentLoadout = progression.GetLoadout(loadoutType);
   if (currentLoadout == nullptr)
   {
      UE_LOG(LogTATSaveGame, Error, TEXT("GetCharacterLoadout got unsupported loadout type %s (%i)"),
             *StaticEnum<ETATLoadoutType>()->GetNameStringByValue(static_cast<int64>(loadoutType)),
             static_cast<int32>(loadoutType));
      return FTATCharacterLoadout{ loadoutType, {} };
   }

   FTATCharacterLoadout characterLoadout = *currentLoadout;

   // July 2024: Temporary compatibility hack for loading data that predates the Type field
   characterLoadout.Type = loadoutType;

   bool customizedLoadoutIsEmpty = characterLoadout.IsEmpty();
   if (!customizedLoadoutIsEmpty)
   {
      // If the player has customized a loadout, make sure that data is still valid.
      // It might be invalid if the player cleared or changed their upgrades, the available loadout changed, or the requirements for a loadout tag changed.
      if (!UTATCharacterMetadataFunctionLibrary::IsValidLoadoutForCharacter(context, loadoutType, characterLoadout))
      {
         UTATCharacterMetadataFunctionLibrary::MakeCharacterLoadoutValid(context, characterLoadout);

         // recheck if the loadout is empty since it likely changed
         customizedLoadoutIsEmpty = characterLoadout.IsEmpty();
      }

      // If we have a valid, non-empty loadout, we're done
      if (!customizedLoadoutIsEmpty)
      {
         return characterLoadout;
      }
   }

   // Return an empty loadout if requested
   if (!getDefaultLoadoutIfEmpty && customizedLoadoutIsEmpty)
   {
      return FTATCharacterLoadout{ loadoutType, {} };
   }

   // Get default loadout from character metadata
   characterLoadout.Entries.Reset();

   // Note on check: Reading from an invalid character isn't desirable as such, but this is
   //                hitting an ensure for the wrong reasons. Once characterSaveId is no 
   //                longer 1:1 with characterId, we will need to stricter, at very least
   //                with writes (reads I could see either way)
   if (character.IsValid())
   {
      const bool foundDefaultLoadout = UTATCharacterMetadataFunctionLibrary::GetDefaultLoadoutForCharacter(context, loadoutType, characterLoadout);
      ensureMsgf(foundDefaultLoadout, TEXT("Could not find default loadout for %s"), *character.ToString());
   }
   return characterLoadout;
}

void UTATSaveGame::UpdateCharacterLoadout(FTATCharacterSaveId character, const FTATCharacterLoadout& newLoadout)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   FTATCharacterLoadout* savedLoadout = characterData.GetLoadout(newLoadout.Type);
   if (savedLoadout == nullptr)
   {
      // we don't have storage for a loadout of the requested type
      UE_LOG(LogTATSaveGame, Error, TEXT("UpdateCharacterLoadout got a loadout with an unsupported type %s (%i)"),
             *StaticEnum<ETATLoadoutType>()->GetNameStringByValue(static_cast<int64>(newLoadout.Type)),
             static_cast<int32>(newLoadout.Type));
      return;
   }
   if (*savedLoadout != newLoadout)
   {
      *savedLoadout = newLoadout;
      _MarkDirty(EOSESavePriority::LowPriority);
      OnCharacterLoadoutChanged.Broadcast(newLoadout, character.Character);
   }
}

FTATCharacterLoadout UTATSaveGame::GetGearLoadout(FTATCharacterSaveId character, bool getDefaultLoadoutIfEmpty) const
{
   return GetCharacterLoadout(character, ETATLoadoutType::Tool, getDefaultLoadoutIfEmpty);
}

void UTATSaveGame::UpdateGearLoadout(FTATCharacterSaveId character, const FTATCharacterLoadout& newGearLoadout)
{
   UpdateCharacterLoadout(character, newGearLoadout);
}

void UTATSaveGame::AddLoot(FTATCharacterSaveId character, const FTATSavedLootAddRequest& lootToAdd)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   characterData.Loot.Add(lootToAdd);
   characterData.OnLootChanged.Broadcast();
   _MarkDirty(EOSESavePriority::LowPriority);
}

bool UTATSaveGame::RemoveLoot(FTATCharacterSaveId character, const FTATSavedLootRemoveRequest& lootToRemove)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);

   if (lootToRemove.IsEmpty())
   {
      UE_LOG(LogTATSaveGame, Warning, TEXT("RemoveLoot called without any loot specified on character %s! No removal applied."),
             *StaticEnum<ETATCharacter>()->GetNameStringByValue(static_cast<int64>(characterData.Character)));
      return true;
   }

   if (characterData.Loot.Remove(lootToRemove))
   {
      characterData.OnLootChanged.Broadcast();
      _MarkDirty(EOSESavePriority::LowPriority);
      return true;
   }
   else
   {
      UE_LOG(LogTATSaveGame, Error, TEXT("Attempted to remove loot that character %s did not have! No removal applied."),
             *StaticEnum<ETATCharacter>()->GetNameStringByValue(static_cast<int64>(characterData.Character)));
      return false;
   }
}

void UTATSaveGame::SetUpgradeLevel(FTATCharacterSaveId character, FGameplayTag tag, int32 level, bool autoEquip)
{
   if (!tag.IsValid())
   {
      UE_LOG(LogTATSaveGame, Error, TEXT("SetUpgradeLevel got an invalid upgrade tag"));
      return;
   }
   if (level <= 0)
   {
      UE_LOG(LogTATSaveGame, Error, TEXT("SetUpgradeLevel got invalid upgrade level %d (with upgrade tag = '%s')"), level, *tag.ToString());
      return;
   }
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   characterData.UnlockedUpgrades.Values.FindOrAdd(tag) = level;
   if (autoEquip)
   {
      characterData.EquippedUpgrades.AddUnique(tag);
   }
   _MarkDirty(EOSESavePriority::LowPriority);
}

void UTATSaveGame::EquipChildUpgrade(FTATCharacterSaveId character, FGameplayTag tag, FGameplayTag parentTag)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   if (ensure(characterData.UnlockedUpgrades.HasTag(tag)))
   {
      check(tag.MatchesTag(parentTag));
      characterData.EquippedUpgrades.RemoveAllSwap([parentTag](const FGameplayTag& equippedTag)
      {
         return equippedTag.MatchesTag(parentTag);
      });
      characterData.EquippedUpgrades.Add(tag);
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

void UTATSaveGame::EquipLoadoutSkill(FTATCharacterSaveId character, FGameplayTag tag)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   if (!tag.IsValid() || ensure(characterData.UnlockedUpgrades.HasTag(tag)))
   {
      characterData.EquippedLoadoutSkill = tag;
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

void UTATSaveGame::ResetUpgradeProgressForTag(FTATCharacterSaveId character, FGameplayTag tag)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   characterData.UnlockedUpgrades.Values.Remove(tag);
   characterData.EquippedUpgrades.RemoveSingleSwap(tag);
   if (characterData.EquippedLoadoutSkill == tag)
   {
      characterData.EquippedLoadoutSkill = FGameplayTag::EmptyTag;
   }
   _MarkDirty(EOSESavePriority::LowPriority);
}

void UTATSaveGame::ResetUpgradeProgressForCharacter(FTATCharacterSaveId character)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   characterData.UnlockedUpgrades.Values.Reset();
   characterData.EquippedUpgrades.Reset();
   characterData.EquippedLoadoutSkill = FGameplayTag::EmptyTag;
   _MarkDirty(EOSESavePriority::LowPriority);
}

void UTATSaveGame::ResetCharacter(FTATCharacterSaveId character)
{
   _GetPlayerProgressionMutable().Reset();
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(character);
   characterData.Reset();

   OnContractsChanged.Broadcast();
   _MarkDirty(EOSESavePriority::LowPriority);
}

ETATSavedFtueState UTATSaveGame::GetFtueState() const
{
   return _playerSaveData.FtueState.State;
}

void UTATSaveGame::SetFtueState(ETATSavedFtueState state)
{
   if (state != _playerSaveData.FtueState.State)
   {
      _playerSaveData.FtueState.State = state;
      _MarkDirty(EOSESavePriority::HighPriority);
      OnFtueChanged.Broadcast();
   }
}

TOptional<ETATDifficulty> UTATSaveGame::GetSelectedDifficulty() const
{
   return GetPlayerProgression().SelectedDifficulty;
}

void UTATSaveGame::SetSelectedDifficulty(ETATDifficulty difficulty)
{
   FTATPlayerProgression& progression = _GetPlayerProgressionMutable();
   if (progression.SelectedDifficulty != difficulty)
   {
      progression.SelectedDifficulty = difficulty;
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

bool UTATSaveGame::AreHUDIconsVisible() const
{
   return _uxSaveData.ShowHUDIcons;
}

void UTATSaveGame::SetHUDIconsVisible(bool showIcons)
{
   if (_uxSaveData.ShowHUDIcons != showIcons)
   {
      _uxSaveData.ShowHUDIcons = showIcons;
      _MarkDirty(EOSESavePriority::LowPriority);
   }
}

const FGuid& UTATSaveGame::GetPlayerAnalyticsId() const
{
   return _temporaryPlayerAnalyticsId;
}

void UTATSaveGame::DispatchProgressionUnexpectedlyChanged()
{
   OnProgressionUnexpectedlyChanged.Broadcast();
}

FTATUpgradeCurrencyChanged& UTATSaveGame::GetUpgradeCurrencyChanged(FTATCharacterSaveId character)
{
   return _GetCharacterProgressionMutable(character).OnUpgradeCurrencyChanged;
}

FTATCharacterProgression::FOnDataChangedEvent& UTATSaveGame::GetCharacterProgressionLootChangedEvent(const FTATCharacterSaveId& characterSaveId)
{
   FTATCharacterProgression& characterData = _GetCharacterProgressionMutable(characterSaveId);
   return characterData.OnLootChanged;
}

FTATPlayerProgression::FOnDataChangedEvent& UTATSaveGame::GetPlayerProgressionMoneyChangedEvent()
{
   FTATPlayerProgression& playerData = _GetPlayerProgressionMutable();
   return playerData.OnMoneyChanged;
}

FTATPlayerProgression& UTATSaveGame::_GetPlayerProgressionMutable()
{
   return const_cast<FTATPlayerProgression&>(this->GetPlayerProgression());
}

FTATCharacterProgression& UTATSaveGame::_GetCharacterProgressionMutable(FTATCharacterSaveId character)
{
   return const_cast<FTATCharacterProgression&>(this->GetCharacterProgression(character));
}

void UTATSaveGame::_SetLastActiveContract(FGameplayTag contractTag)
{
   FTATPlayerProgression& playerProgression = _GetPlayerProgressionMutable();
   if (playerProgression.LastActiveContract != contractTag)
   {
      playerProgression.LastActiveContract = contractTag;
      OnLastActiveContractSet.Broadcast(contractTag);
   }
}
