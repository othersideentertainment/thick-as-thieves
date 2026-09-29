// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/TATLocalPlayerStateWorldSubsystem.h"

// tat
#include "Quests/TATQuestTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLocalPlayerStateWorldSubsystem)

void UTATLocalPlayerStateWorldSubsystem::SetQuestRelatedLoot(TConstArrayView<FTATLootIdentifier> lootIds)
{
   if (lootIds != _questRelatedLootIds)
   {
      _questRelatedLootIds = lootIds;
      OnLocalQuestRelatedLootChanged.Broadcast(MakeArrayView(_questRelatedLootIds));
   }
}

void UTATLocalPlayerStateWorldSubsystem::SetLocalQuests(TConstArrayView<FGameplayTag> questTags)
{
   if (questTags != _localQuestTags)
   {
      _localQuestTags = questTags;
      OnLocalQuestChanged.Broadcast(_localQuestTags);

      if (!_localContract.IsValid())
      {
         if (const FGameplayTag* found = _localQuestTags.FindByPredicate([](const FGameplayTag& tag) { return tag.MatchesTag(TAG_Contract); }))
         {
            _localContract = *found;
            OnLocalContractSet.Broadcast(_localContract);
            OnLocalContractSet.Clear();
         }
      }
   }
}
