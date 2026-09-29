// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestInfo.h"

// tat
#include "GameFramework/TATDifficulty.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Quests/Rewards/TATQuestReward.h"
#include "Quests/Rewards/Requirements/TATQuestRewardRequirementType.h"
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestTags.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestInfo)
DEFINE_LOG_CATEGORY_STATIC(LogTATQuestInfo, Log, All);

const FTATQuestObjectiveInfo* FTATContractInfo::GetObjective() const
{
   // Consider: should return dummy struct if not found to avoid callers having to deal with it?
   return Objective.GetPtr<FTATQuestObjectiveInfo>();
}

const FTATQuestObjectiveInfo& FTATContractInfo::GetObjectiveChecked() const
{
   return Objective.Get<FTATQuestObjectiveInfo>();
}

void FTATQuestInfo::GrantRewards(const FTATQuestRewardContext& context) const
{
   for (const FTATQuestReward& reward : Rewards)
   {
      reward.Grant(context);
   }
}

FText FTATContractInfo::GetObjectiveText(const UObject* worldContext) const
{
   if (const FTATQuestObjectiveInfo* objective = GetObjective())
   {
      return objective->GetObjectiveText(worldContext);
   }

   return FText();
}

#if WITH_EDITOR
void FTATQuestInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if (Title.IsEmpty())
   {
      reportError(INVTEXT("Quest has no title"));
   }

   // TODO: Check for Description once populated

   for (const FTATQuestReward& reward : Rewards)
   {
      if (!reward.IsValid())
      {
         reportError(INVTEXT("Quest has reward with no type"));
      }
   }
}
#endif

const FText& FTATQuestJournalEntry::ForCharacter(ETATCharacter character) const
{
   if (const FText* found = CharacterOverrides.Find(character))
   {
      return *found;
   }

   return DefaultJournalText;
}

uint32 FTATMissionInfo::GetUnlockedBonusRewardsBitmask(const FTATQuestRewardRequirementContext& context) const
{
   UE_CLOG(BonusRewards.Num() > 32, LogTATQuestInfo, Error, TEXT("BonusRewards count exceeds the 32 bits supported by bitmask! \
Either reduce number of rewards, or bump bitmask up to uint64"));
   uint32 bitmask = 0;
   for (int32 bonusRewardIndex = 0; bonusRewardIndex < BonusRewards.Num(); bonusRewardIndex++)
   {
      const FTATQuestConditionalReward& bonusReward = BonusRewards[bonusRewardIndex];
      if (bonusReward.RequirementsMet(context))
      {
         bitmask |= (1 << bonusRewardIndex);
      }
   }
   return bitmask;
}

void FTATMissionInfo::GrantBonusRewards(const FTATQuestRewardContext& context, uint32 unlockedBonusRewardsBitmask) const
{
   for (int32 bonusRewardIndex = 0; bonusRewardIndex < BonusRewards.Num(); bonusRewardIndex++)
   {
      if (IsBonusRewardUnlocked(unlockedBonusRewardsBitmask, bonusRewardIndex))
      {
         const FTATQuestConditionalReward& bonusReward = BonusRewards[bonusRewardIndex];
         bonusReward.GrantRewards(context);
      }
   }
}

// static
bool FTATMissionInfo::IsBonusRewardUnlocked(uint32 unlockedBonusRewardsBitmask, int32 rewardIndex)
{
   const int32 rewardIndexBitmask = (1 << rewardIndex);
   return (unlockedBonusRewardsBitmask & rewardIndexBitmask) != 0;
}

float FTATMissionInfo::GetMatchDurationForDifficulty(ETATDifficulty difficulty) const
{
   const int32 index = static_cast<int32>(difficulty);
   if (index >= 0 && index < UE_ARRAY_COUNT(MatchDurationsByDifficulty))
   {
      return MatchDurationsByDifficulty[index];
   }

   UE_LOG(LogTATQuestInfo, Warning, TEXT("Could not find a configured Match Duration for current difficulty (%s) for the mission (%s), defaulting to 90 minutes!"), *UEnum::GetValueAsString(difficulty), *QuestTag.ToString());
   return 5400.0f;
}

#if WITH_EDITOR

void FTATMissionInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   Super::Validate(reportError);

   if (QuestTag.IsValid() && !QuestTag.MatchesTag(TAG_Mission))
   {
      reportError(INVTEXT("QuestTag does not start with Mission"));
   }
}

void FTATContractInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   Super::Validate(reportError);

   if (QuestTag.IsValid() && !QuestTag.MatchesTag(TAG_Contract))
   {
      reportError(INVTEXT("QuestTag does not start with Contract"));
   }

   if (const FTATQuestObjectiveInfo* objective = Objective.GetPtr<FTATQuestObjectiveInfo>())
   {
      objective->Validate(reportError);
   }
   else
   {
      reportError(INVTEXT("Quest has no objective"));
   }

   // TODO: Validate that maps are specified for non-dev contracts
   // TODO: Validate contract order for non-dev contracts

   // For now, unconditionally allow having an empty intro encounter text, since
   // it might be part of a VS-style quest chain. Holding off a more elaborate
   // validation, since this style of chaining would likely be removed.
   /*if (IntroEncounterText.IsEmpty())
   {
      reportError(INVTEXT("Quest has no IntroEncounterText"));
   }*/

   if (OutroEncounterText.IsEmpty())
   {
      reportError(INVTEXT("Quest has no OutroEncounterText"));
   }

   if (IntroJournalEntry.DefaultJournalText.IsEmpty())
   {
      reportError(INVTEXT("Quest has no default IntroJournalEntry"));
   }

   if (CompleteJournalEntry.DefaultJournalText.IsEmpty())
   {
      reportError(INVTEXT("Quest has no default CompleteJournalEntry"));
   }
}
#endif
