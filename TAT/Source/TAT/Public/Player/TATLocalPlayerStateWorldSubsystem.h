// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"

#include "TATLocalPlayerStateWorldSubsystem.generated.h"

UCLASS()
class TAT_API UTATLocalPlayerStateWorldSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestRelatedLootChanged, TConstArrayView<FTATLootIdentifier>);
   FOnQuestRelatedLootChanged OnLocalQuestRelatedLootChanged;

   void SetQuestRelatedLoot(TConstArrayView<FTATLootIdentifier> lootIds);
   bool HasQuestRelatedLoot(FTATLootIdentifier lootId) const { return _questRelatedLootIds.Contains(lootId); }
   TConstArrayView<FTATLootIdentifier> GetQuestRelatedLoot() const { return _questRelatedLootIds; }

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnLocalQuestChanged, TConstArrayView<FGameplayTag>);
   FOnLocalQuestChanged OnLocalQuestChanged;

   // would include shared quest
   void SetLocalQuests(TConstArrayView<FGameplayTag> questTags);
   TConstArrayView<FGameplayTag> GetLocalQuests() const { return _localQuestTags; }


   // Convenience for contract, so quest-specific elements can subscribe
   // This does bake in assumptions that there is at most one and it won't change
   // mid-match once set, but this allows callers to have a simpler API. Not super
   // worried about changing if needed if users are modest.
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnLocalContractSet, FGameplayTag);
   FOnLocalContractSet OnLocalContractSet;
   const FGameplayTag& GetLocalContract() const { return _localContract; }

private:
   TArray<FTATLootIdentifier, TInlineAllocator<2>> _questRelatedLootIds;
   TArray<FGameplayTag, TInlineAllocator<2>> _localQuestTags;
   FGameplayTag _localContract;
};
