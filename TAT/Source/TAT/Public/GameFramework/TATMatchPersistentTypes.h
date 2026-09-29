// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "Loot/TATLootTypes.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "GameFramework/TATDifficulty.h"
#include "Progression/TATPlayerExperience.h"

// ose
#include "Player/OSEPlayerStats.h"

#include "TATMatchPersistentTypes.generated.h"

class ATATPlayerState;

USTRUCT(BlueprintType)
struct TAT_API FTATCachedPlayerInfo
{
   GENERATED_BODY()

   FTATCachedPlayerInfo() {}
   FTATCachedPlayerInfo(const ATATPlayerState* player);

   UPROPERTY(BlueprintReadOnly, Transient)
   FString PlayerName;

   UPROPERTY(BlueprintReadOnly, Transient)
   ETATCharacter CharacterType = ETATCharacter::None;

   bool IsValid() const { return !(PlayerName.IsEmpty() || CharacterType == ETATCharacter::None); }
};

UENUM(BlueprintType)
enum class EMatchCompletionState : uint8
{
   Unset,
   Escaped,
   Captured
};

USTRUCT(BlueprintType)
struct FMatchPersistentQuestResult
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly, Transient)
   FGameplayTag QuestTag;

   UPROPERTY(BlueprintReadOnly, Transient)
   bool IsObjectiveComplete = false;

   // Accomplices participate in the contract with their ally,
   // but are not officially on it, and don't get metagame rewards
   UPROPERTY(BlueprintReadOnly, Transient)
   bool IsAccomplice = false;

   // Indicates what levels of rewards the player unlocked (an index corresponding to FTATQuestRewardLevel::Rewards)
   // Reward levels are cumulative, i.e. unlocking a level grants all rewards in preceding levels
   // QUESTTODO: Remove?
   UPROPERTY(Transient)
   uint32 UnlockedBonusRewardsBitmask = 0;

   bool IsCompleteForMetagame() const { return IsObjectiveComplete && !IsAccomplice; }
};

// This is a temporary struct, that the server stashes away when they escape from a map
// this will ideally be instead sent to the client from the server via some backend service
USTRUCT(BlueprintType)
struct FMatchPersistentData
{
   GENERATED_BODY()

   // loot carried at end of match
   UPROPERTY(Transient)
   TArray<FTATLootIdentifier> CarriedLoot;

   // Loot that was taken by allies
   UPROPERTY(Transient)
   TArray<FTATLootIdentifier> AllyCarriedLoot;

   // Loot stashed at end of match
   UPROPERTY(Transient)
   TArray<FTATLootIdentifier> StashedLoot;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 OriginalCarriedLootValue = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 KeptCarriedLootValue = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 AllyCarriedLootValue = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 StashedLootValue = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 MoneyTaken = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   bool HasCoopAllies = false;
   
   UPROPERTY(BlueprintReadOnly, Transient)
   ETATCharacter CharacterType = ETATCharacter::None;
   
   UPROPERTY(BlueprintReadOnly, Transient)
   FTATCharacterSaveId CharacterSaveId;

   // We will most likely want to make our own enum for this, something with Escaped or Captured
   UPROPERTY(BlueprintReadOnly, Transient)
   EMatchCompletionState CompletionState = EMatchCompletionState::Unset;

   UPROPERTY(BlueprintReadOnly, Transient)
   FMatchPersistentQuestResult MissionResult;

   UPROPERTY(BlueprintReadOnly, Transient)
   FMatchPersistentQuestResult ContractResult;

   UPROPERTY(BlueprintReadOnly, Transient)
   FOSEPlayerStats PlayerStats;

   UPROPERTY(BlueprintReadOnly, Transient)
   bool WasKilledByPlayer = false;

   UPROPERTY(BlueprintReadOnly, Transient)
   FTATCachedPlayerInfo PlayerKnockedOutByInfo;

   UPROPERTY(BlueprintReadOnly, Transient)
   float MatchTimeInSeconds = 0.f;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 XPGained = 0.f;

   UPROPERTY(BlueprintReadOnly, Transient)
   TArray<FTATFinishedMatchXPGained> XPGainedArray;

   UPROPERTY(BlueprintReadOnly, Transient)
   ETATDifficulty MatchDifficulty = ETATDifficulty::Easy;

   /// Set to -1 if we didn't escape, otherwise set to 0, 1, 2, etc. based on where we placed in the order of escaped players
   /// e.g. 0 is we escaped first, 1 if we escaped second, etc.
   UPROPERTY(Transient)
   int32 EscapeOrderPlacement = -1;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 MatchRanking = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   bool IsFTUEMatch = false;

   // Whether matchmade with randos
   UPROPERTY(BlueprintReadOnly, Transient)
   bool WasMatchmade = false;

   int32 GetTotalValue() const { return KeptCarriedLootValue + AllyCarriedLootValue + StashedLootValue; }
   int CountAllLootOfType(const UObject* worldContext, ETATLootType type) const;

   void Reset();
};

USTRUCT(BlueprintType)
struct FMatchPersistentRankingPlayerData
{
   GENERATED_BODY()
public:
   UPROPERTY(BlueprintReadOnly, Transient)
   FString PlayerName;

   UPROPERTY(BlueprintReadOnly, Transient)
   ETATCharacter CharacterType = ETATCharacter::None;

   UPROPERTY(BlueprintReadOnly, Transient)
   int32 TotalLootValue = 0;
};

USTRUCT(BlueprintType)
struct FMatchPersistentXPGainedData
{
   GENERATED_BODY()
public:
   UPROPERTY(BlueprintReadOnly, Transient)
   int LevelBeforeXPGain = 0;

   UPROPERTY(BlueprintReadOnly, Transient)
   float LevelXPBeforeXPGain = 0.f;

   UPROPERTY(BlueprintReadOnly, Transient)
   TArray<FTATFinishedMatchXPGained> XPGainedArray;

   void Reset();
};
