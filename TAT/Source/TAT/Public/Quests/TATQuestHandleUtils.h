// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Quests/TATQuestHandle.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATQuestHandleUtils.generated.h"

struct FTATCharacterSaveId;
class UTATSaveGame;
class UAkAudioEvent;
class UTexture2D;
struct FMatchPersistentQuestResult;
struct FTATQuestBonusRewardResult;
struct FTATQuestHandle;
struct FTATQuestReward;
enum class ETATCharacter : uint8;


UCLASS()
class TAT_API UTATQuestHandleUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, meta=(WorldContext="worldContext", CallableWithoutWorldContext, Categories="Mission,Contract"), Category = "Quest|Handle")
   static FTATQuestHandle CreateQuestHandle(FGameplayTag questTag, const UObject* worldContext);

   UFUNCTION(BlueprintPure, meta=(DisplayName = "Equal (QuestHandle)", CompactNodeTitle = "==", Keywords = "== equal"), Category="Quest|Handle")
   static bool EqualEqual_TATQuestHandleTATQuestHandle(const FTATQuestHandle& a, const FTATQuestHandle& b);
   UFUNCTION(BlueprintPure, meta=(DisplayName = "Not Equal (QuestHandle)", CompactNodeTitle = "!=", Keywords = "!= not equal"), Category="Quest|Handle")
   static bool NotEqual_TATQuestHandleTATQuestHandle(const FTATQuestHandle& a, const FTATQuestHandle& b);

   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FGameplayTag GetQuestTag(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static bool IsValid(const FTATQuestHandle& questHandle);

   /// Returns the Duration of the Mission (in seconds).
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static UPARAM(DisplayName="Seconds") float GetQuestDuration(const FTATQuestHandle& questHandle, ETATDifficulty difficulty);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestTitle(const FTATQuestHandle& questHandle);
   /// Difficulty param only matters if you are trying to display the correct match or endgame durations.  Irrelevant for Contracts and whatnot.
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestDescription(const FTATQuestHandle& questHandle, ETATDifficulty difficulty);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestPostMatchSuccessText(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestPostMatchFailureText(const FTATQuestHandle& questHandle);

   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static const TArray<FTATQuestReward>& GetQuestRewards(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintCallable, Category="Quest|Handle")
   static TArray<FTATQuestBonusRewardResult> GetQuestBonusRewardsResults(const FTATQuestHandle& questHandle, const FMatchPersistentQuestResult& questResult, bool unlockedRewardsOnly = false);

   UFUNCTION(BlueprintPure, Category="Quest|Handle", meta = (WorldContext = "worldContext"))
   static FText GetQuestObjectiveText(const FTATQuestHandle& questHandle, UObject* worldContext);

   // NOTE: Will likely be replaced once encounters have different types
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestIntroEncounterText(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestOutroEncounterText(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static TSoftObjectPtr<UAkAudioEvent> GetQuestOutroEncounterVO(const FTATQuestHandle& questHandle);
   UFUNCTION(BlueprintPure, Category = "Quest|Handle")
   static FText GetQuestOutroEncounterFailedText(const FTATQuestHandle& questHandle);

   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestIntroJournalText(const FTATQuestHandle& questHandle, ETATCharacter character);
   UFUNCTION(BlueprintPure, Category="Quest|Handle")
   static FText GetQuestCompleteJournalText(const FTATQuestHandle& questHandle, ETATCharacter character);

   UFUNCTION(BlueprintPure, Category = "Quest|Handle")
   static TSoftObjectPtr<UTexture2D> GetQuestDisplayImage(const FTATQuestHandle& questHandle);

   // Grants quest reward directly
   //
   // Use with *caution*, as this can bypass quest flow invariants
   // This has been exposed to BP to handle rewards for a fake tutorial contract,
   // and any other use should be regarded with suspicion.
   UFUNCTION(BlueprintCallable, Category = "Quest|Handle", DisplayName="Grant Quest Rewards (CAUTION!)")
   static void GrantQuestRewards_Caution(const FTATQuestHandle& questHandle, UTATSaveGame* saveGame, const FTATCharacterSaveId& character);
};
