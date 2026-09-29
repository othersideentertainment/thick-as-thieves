// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Quests/TATPlayerQuestSlot.h"
#include "Quests/TATQuestObjectiveTracker.h"

// ue
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "TATPlayerQuestComponent.generated.h"

class UTATRootPlayerObjective;
struct FTATQuestObjectiveInfo;
struct FTATLootIdentifier;

UCLASS(ClassGroup=(Custom))
class TAT_API UTATPlayerQuestComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATPlayerQuestComponent();
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveCompleteChanged, bool, isObjectiveComplete, ETATPlayerQuestSlot, questSlot);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveProgressChanged, int32, progress, ETATPlayerQuestSlot, questSlot);

protected:
   // Called when the game starts
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:

   void AuthorityInitializeQuest(int32 playerId);
   void AuthorityAddConnectionToContractGroups();
   void AuthorityForceObjectiveComplete(ETATPlayerQuestSlot questSlot);
   bool IsQuestCompleteForMatchEnd(ETATPlayerQuestSlot questSlot, bool escaped, const FMatchPersistentData& matchData) const;
   UFUNCTION(BlueprintPure, Category= "PlayerState|TAT|Quest")
   bool IsQuestObjectiveComplete(ETATPlayerQuestSlot questSlot) const;
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT|Quest")
   FGameplayTag GetActiveQuestTag(ETATPlayerQuestSlot questSlot) const;
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT|Quest")
   bool HasActiveQuest(ETATPlayerQuestSlot questSlot) const;
   UTATRootPlayerObjective* GetObjectiveForSlot(ETATPlayerQuestSlot slot) const;
   TConstArrayView<TObjectPtr<UTATRootPlayerObjective>> GetObjectives() const { return _objectives; }

   // may apply to shared quest
   TConstArrayView<FGameplayTag> GetActiveQuestTags() const;
   bool HasQuestRelatedLoot(FTATLootIdentifier lootId) const;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnQuestObjectiveCompleteChanged OnQuestObjectiveCompleteChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnQuestObjectiveProgressChanged OnQuestObjectiveProgressChanged;
   
   static constexpr int MaxQuestSlots = static_cast<int>(ETATPlayerQuestSlot::MAX);
   using FQuestLootResult = TConstArrayView<FTATLootIdentifier>;
   FQuestLootResult GetQuestRelatedLoot() const;

   void NotifyObjectiveComplete(bool complete, UTATRootPlayerObjective* objective);
   void NotifyObjectiveProgress(int32 progress, UTATRootPlayerObjective* objective);

private:
   void _TryStartObjectiveTracker(ETATPlayerQuestSlot slot, FGameplayTag questTag, const FTATQuestObjectiveInfo* objectiveInfo);
   void _BroadcastQuestObjectiveCompleteChanged(bool isComplete, ETATPlayerQuestSlot slot);
   UFUNCTION()
   void _OnRep_ActiveQuest();
   void _OnActiveQuestsSet();
   UFUNCTION()
   void _OnRep_RelatedLootIds();

   
   FTATQuestObjectiveState _GetObjectiveStateForSlot(ETATPlayerQuestSlot slot) const;

   UFUNCTION()
   void _OnRep_Objectives();

   bool _IsLocalPlayer() const;

   TArray<FGameplayTag> _activeQuestTags;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_Objectives)
   TArray<TObjectPtr<UTATRootPlayerObjective>> _objectives;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_RelatedLootIds)
   TArray<FTATLootIdentifier> _relatedLootIds;
};
