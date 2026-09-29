// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATQuestObjectiveTracker.h"

// ue
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "TATPlayerObjective.generated.h"

enum class ETATQuestObjectiveProgressStyle : uint8;
enum class ETATPlayerQuestSlot : uint8;
struct FTATMinimalObjective;
struct FTATQuestObjectiveInfo;


struct FTATPlayerObjectiveInitContext
{
   TWeakObjectPtr<APlayerState> PlayerState;
   FGameplayTag QuestTag;
   TConstArrayView<TObjectPtr<UTATPlayerObjective>> ChildObjectives;
};


// A runtime-only replicated view of the state of an objective for a player
UCLASS(BlueprintType)
class UTATPlayerObjective : public UObject
{
   GENERATED_BODY()
public:
   // authority-only opaque id representing the data that the objective came from
   // so that progress can be shared. Not stable.
   uint64 AuthorityId = 0;
   
   // NOTE: Replicating text over the wire isn't cheap, but we can probably live with it given that the number of objectives is
   //       small and up-front
   UPROPERTY(BlueprintReadOnly, Transient, Replicated)
   FText ObjectiveText;

   UPROPERTY(BlueprintReadOnly, Transient, Replicated)
   ETATQuestObjectiveProgressStyle ProgressStyle;

   UPROPERTY(BlueprintReadOnly, Transient, Replicated)
   int32 TargetProgress;

   UFUNCTION(BlueprintPure)
   bool IsComplete() const { return _combinedState.IsComplete; }

   UFUNCTION(BlueprintPure)
   int32 GetProgress() const { return _combinedState.Progress; }

   UFUNCTION(BlueprintPure)
   bool IsCompleteSelf() const { return _selfState.IsComplete; }

   const FTATQuestObjectiveState& GetState() const { return _combinedState; }

   // UObject
   virtual bool IsSupportedForNetworking() const override { return true; }
#if UE_WITH_IRIS
   // Register replication fragments
   virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& context, UE::Net::EFragmentRegistrationFlags registrationFlags) override;
#endif // UE_WITH_IRIS
   

   void AuthorityInit(const FTATMinimalObjective& objective, const FTATPlayerObjectiveInitContext& initContext);
   void AuthoritySetAllyObjectiveState(const FTATQuestObjectiveState& state);
   
   void AuthorityCheatComplete();
   bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const;
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveCompleteChanged, bool, isObjectiveComplete, UTATPlayerObjective*, objective);
   UPROPERTY(BlueprintAssignable)
   FOnQuestObjectiveCompleteChanged OnCompleteChanged;
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveProgressChanged, int32, progress, UTATPlayerObjective*, objective);
   UPROPERTY(BlueprintAssignable)
   FOnQuestObjectiveProgressChanged OnProgressChanged;

   FSimpleMulticastDelegate OnAuthorityCompleteSelfChanged;

protected:
   void _SetSelfObjectiveState(const FTATQuestObjectiveState& state);
   UFUNCTION()
   void _OnRep_State();
   virtual void _HandleStateChanged(const FTATQuestObjectiveState& previous);
   
   UPROPERTY(Transient)
   FTATQuestObjectiveState _combinedState;

   // The state
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_State)
   FTATQuestObjectiveState _selfState;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_State)
   FTATQuestObjectiveState _allyState;
   
   UPROPERTY(Transient)
   TObjectPtr<UTATQuestObjectiveTracker> _authorityTracker = nullptr;
};


// A runtime-only replicated view of the state of an objective for a player
//
// This is a "root" objective which may have sub-objectives
UCLASS()
class UTATRootPlayerObjective : public UTATPlayerObjective
{
   GENERATED_BODY()

public:

   UPROPERTY(Transient, Replicated)
   ETATPlayerQuestSlot Slot;
   
   UPROPERTY(Transient, Replicated)
   FGameplayTag QuestTag;

   const TArray<TObjectPtr<UTATPlayerObjective>>& GetChildObjectives() const { return _childObjectives; }
   
   void AuthorityInitRoot(ETATPlayerQuestSlot slot, const FTATQuestObjectiveInfo& objectiveInfo, const FTATPlayerObjectiveInitContext& initContext);

private:
   UPROPERTY(Transient, Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
   TArray<TObjectPtr<UTATPlayerObjective>> _childObjectives;
   
   virtual void _HandleStateChanged(const FTATQuestObjectiveState& previous) override;
};
