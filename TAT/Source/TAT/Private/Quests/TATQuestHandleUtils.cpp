// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/TATQuestHandleUtils.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Online/TATGameState.h"
#include "SaveGame/TATSaveGame.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestHandle.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/Rewards/TATQuestBonusRewardResult.h"
#include "Quests/Rewards/TATQuestRewardContext.h"
#include "TATGameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestHandleUtils)

FTATQuestHandle UTATQuestHandleUtils::CreateQuestHandle(FGameplayTag questTag, const UObject* worldContext)
{
   FTATQuestHandle handle;

   const UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::LogAndReturnNull);
   if (world)
   {
      const UTATQuestDataSubsystem* questSubsystem = UGameInstance::GetSubsystem<UTATQuestDataSubsystem>(world->GetGameInstance());
      if (questSubsystem)
      {
         questSubsystem->FindQuestHandle(questTag, handle);
      }
   }

   return handle;
}

bool UTATQuestHandleUtils::EqualEqual_TATQuestHandleTATQuestHandle(const FTATQuestHandle& a, const FTATQuestHandle& b)
{
   return a == b;
}

bool UTATQuestHandleUtils::NotEqual_TATQuestHandleTATQuestHandle(const FTATQuestHandle& a, const FTATQuestHandle& b)
{
   return a != b;
}

FGameplayTag UTATQuestHandleUtils::GetQuestTag(const FTATQuestHandle& questHandle)
{
   return questHandle.GetQuestTag();
}

bool UTATQuestHandleUtils::IsValid(const FTATQuestHandle& questHandle)
{
   return questHandle.IsValid();
}

float UTATQuestHandleUtils::GetQuestDuration(const FTATQuestHandle& questHandle, ETATDifficulty difficulty)
{
   if (const FTATMissionInfo* mission = questHandle.GetMission())
   {
      return mission->GetMatchDurationForDifficulty(difficulty);
   }

   return 0.0f;
}

FText UTATQuestHandleUtils::GetQuestTitle(const FTATQuestHandle& questHandle)
{
   if (const FTATMinimalQuestInfo* quest = questHandle.GetMinimalQuest())
   {
      return quest->GetTitle();
   }
   else
   {
      return INVTEXT("<UNKNOWN>");
   }
}

FText UTATQuestHandleUtils::GetQuestDescription(const FTATQuestHandle& questHandle, ETATDifficulty difficulty)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      // Divide by 60.0f to convert seconds into minutes
      float matchDuration = GetQuestDuration(questHandle, difficulty) / 60.0f;
      float endgameDuration = UTATProjectSettings::Get().GetEndgameDurationForDifficulty(difficulty) / 60.0f;
      FNumberFormattingOptions formatOptions;
      formatOptions.MaximumFractionalDigits = 0;
      FText formattedQuestDescription = FText::Format(quest->Description, FFormatNamedArguments{
         {"MatchDurationInMinutes", FText::AsNumber(matchDuration, &formatOptions)},
         {"EndgameDurationInMinutes", FText::AsNumber(endgameDuration, &formatOptions)} });
      return formattedQuestDescription;
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestPostMatchSuccessText(const FTATQuestHandle& questHandle)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      return quest->PostMatchSuccessText;
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestPostMatchFailureText(const FTATQuestHandle& questHandle)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      return quest->PostMatchFailureText;
   }

   return FText();
}

const TArray<FTATQuestReward>& UTATQuestHandleUtils::GetQuestRewards(const FTATQuestHandle& questHandle)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      return quest->Rewards;
   }
   else
   {
      static const TArray<FTATQuestReward> kEmptyRewards;
      return kEmptyRewards;
   }
}

TArray<FTATQuestBonusRewardResult> UTATQuestHandleUtils::GetQuestBonusRewardsResults(const FTATQuestHandle& questHandle, const FMatchPersistentQuestResult& questResult, bool unlockedRewardsOnly /*= false*/)
{
   TArray<FTATQuestBonusRewardResult> results;
   if (const FTATMissionInfo* mission = questHandle.GetMission())
   {
      const TArray<FTATQuestConditionalReward>& bonusRewards = mission->BonusRewards;
      for (int32 rewardIndex = 0; rewardIndex < bonusRewards.Num(); rewardIndex++)
      {
         const bool unlocked = FTATMissionInfo::IsBonusRewardUnlocked(questResult.UnlockedBonusRewardsBitmask, rewardIndex);
         if (unlockedRewardsOnly && !unlocked)
         {
            continue;
         }

         const FTATQuestConditionalReward& bonusReward = bonusRewards[rewardIndex];
         results.Emplace(bonusReward, unlocked);
      }
   }
   return results;
}

FText UTATQuestHandleUtils::GetQuestObjectiveText(const FTATQuestHandle& questHandle, UObject* worldContext)
{
   if (const FTATMinimalQuestInfo* quest = questHandle.GetMinimalQuest())
   {
      return quest->GetObjectiveText(worldContext);
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestIntroEncounterText(const FTATQuestHandle& questHandle)
{
   if (const FTATContractInfo* quest = questHandle.GetContract())
   {
      return quest->IntroEncounterText;
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestOutroEncounterText(const FTATQuestHandle& questHandle)
{
   if (const FTATContractInfo* quest = questHandle.GetContract())
   {
      return quest->OutroEncounterText;
   }

   return FText();
}

TSoftObjectPtr<UAkAudioEvent> UTATQuestHandleUtils::GetQuestOutroEncounterVO(const FTATQuestHandle& questHandle)
{
   if (const FTATContractInfo* contract = questHandle.GetContract())
   {
      return contract->OutroEncounterVO;
   }

   return {};
}

FText UTATQuestHandleUtils::GetQuestOutroEncounterFailedText(const FTATQuestHandle& questHandle)
{
   if (const FTATContractInfo* quest = questHandle.GetContract())
   {
      return quest->ObjectiveFailedEncounterText;
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestIntroJournalText(const FTATQuestHandle& questHandle, ETATCharacter character)
{
   if (const FTATContractInfo* quest = questHandle.GetContract())
   {
      return quest->IntroJournalEntry.ForCharacter(character);
   }

   return FText();
}

FText UTATQuestHandleUtils::GetQuestCompleteJournalText(const FTATQuestHandle& questHandle, ETATCharacter character)
{
   if (const FTATContractInfo* quest = questHandle.GetContract())
   {
      return quest->CompleteJournalEntry.ForCharacter(character);
   }

   return FText();
}

TSoftObjectPtr<UTexture2D> UTATQuestHandleUtils::GetQuestDisplayImage(const FTATQuestHandle& questHandle)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      return quest->ObjectiveDisplayImage;
   }
   return TSoftObjectPtr<UTexture2D>();
}

void UTATQuestHandleUtils::GrantQuestRewards_Caution(const FTATQuestHandle& questHandle, UTATSaveGame* saveGame, const FTATCharacterSaveId& character)
{
   if (const FTATQuestInfo* quest = questHandle.GetQuest())
   {
      return quest->GrantRewards(FTATQuestRewardContext {
         .SaveGame = saveGame,
         .Character = character,
      });
   }
}
