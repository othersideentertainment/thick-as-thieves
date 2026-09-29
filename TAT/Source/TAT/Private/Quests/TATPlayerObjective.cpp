// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATPlayerObjective.h"

// tat
#include "Quests/TATPlayerQuestComponent.h"
#include "Quests/TATQuestObjective.h"

// ue
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerObjective)


void UTATPlayerObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   
   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_InitialOnly;
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ObjectiveText, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ProgressStyle, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TargetProgress, params);
   
   params.Condition = COND_None;
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _selfState, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _allyState, params);
}

#if UE_WITH_IRIS
void UTATPlayerObjective::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& context, UE::Net::EFragmentRegistrationFlags registrationFlags)
{
   // Build descriptors and allocate PropertyReplicaitonFragments for this object
   UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, context, registrationFlags);
}
#endif // UE_WITH_IRIS

void UTATPlayerObjective::AuthorityInit(const FTATMinimalObjective& objective, const FTATPlayerObjectiveInitContext& initContext)
{
   AuthorityId = objective.Id;
   ObjectiveText = objective.ObjectiveText;
   ProgressStyle = objective.ProgressStyle;
   TargetProgress = objective.TargetProgress;
   
   FTATObjectiveTrackerPayload trackerPayload = objective.Tracker;
   // for now punt on async load of tracker, since all are defined in code anyways
   check(trackerPayload.TrackerClass.IsValid() || trackerPayload.TrackerClass.IsNull());
   if (TSubclassOf<UTATQuestObjectiveTracker> trackerClass = trackerPayload.TrackerClass.Get())
   {
      UTATQuestObjectiveTracker* tracker = NewObject<UTATQuestObjectiveTracker>(this, trackerClass);
      _authorityTracker = tracker;
      tracker->OnProgressChanged.BindUObject(this, &ThisClass::_SetSelfObjectiveState);
      tracker->Initialize(FTATObjectiveTrackerContext {
         .PlayerState = initContext.PlayerState.Get(),
         .Params = trackerPayload.Params.Get(),
         .QuestTag = initContext.QuestTag,
         .ChildObjectives = initContext.ChildObjectives,
      });
   }
}



void UTATPlayerObjective::AuthorityCheatComplete()
{
   if(_authorityTracker)
   {
      _authorityTracker->CheatComplete();
   }
}

bool UTATPlayerObjective::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   return _authorityTracker && _authorityTracker->IsCompleteForMatchEnd(escaped, matchData);
}

void UTATPlayerObjective::AuthoritySetAllyObjectiveState(const FTATQuestObjectiveState& state)
{
   if (_allyState != state)
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _allyState, this);
      _allyState = state;
      _OnRep_State();
   }
}

void UTATPlayerObjective::_SetSelfObjectiveState(const FTATQuestObjectiveState& state)
{
   if(_selfState != state)
   {
      const FTATQuestObjectiveState oldState = _selfState;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _selfState, this);
      _selfState = state;
      _OnRep_State();

      if (oldState.IsComplete != _selfState.IsComplete)
      {
         OnAuthorityCompleteSelfChanged.Broadcast();
      }
   }
}

void UTATPlayerObjective::_OnRep_State()
{
   FTATQuestObjectiveState newCombinedState = {
      .IsComplete = _selfState.IsComplete || _allyState.IsComplete,
      // NOTE: not supporting combining of progress, so just implement in such a way that it works if ally progress is always 0
      .Progress = _selfState.Progress + _allyState.Progress,
   };
   if (newCombinedState != _combinedState)
   {
      const FTATQuestObjectiveState previous = _combinedState;
      _combinedState = newCombinedState;
      _HandleStateChanged(previous);
   }
}

void UTATPlayerObjective::_HandleStateChanged(const FTATQuestObjectiveState& oldState)
{
   if(_combinedState.IsComplete != oldState.IsComplete)
   {
      OnCompleteChanged.Broadcast(_combinedState.IsComplete, this);
   }

   if(_combinedState.Progress != oldState.Progress)
   {
      OnProgressChanged.Broadcast(_combinedState.Progress, this);
   }
}

void UTATRootPlayerObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   
   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_InitialOnly;
   DOREPLIFETIME_WITH_PARAMS_FAST(UTATRootPlayerObjective, Slot, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UTATRootPlayerObjective, QuestTag, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UTATRootPlayerObjective, _childObjectives, params);
}

void UTATRootPlayerObjective::AuthorityInitRoot(ETATPlayerQuestSlot slot, const FTATQuestObjectiveInfo& objectiveInfo,
   const FTATPlayerObjectiveInitContext& initContext)
{
   Slot = slot;
   QuestTag = initContext.QuestTag;

   objectiveInfo.PopulateChildObjectives(this, [this, &initContext] (const FTATMinimalObjective& childInfo)
   {
      UTATPlayerObjective* child = NewObject<UTATPlayerObjective>(this);
      child->AuthorityInit(childInfo, initContext);
      _childObjectives.Add(child);
   });

   {
      // pass children to own tracker init
      FTATPlayerObjectiveInitContext selfInitContext = initContext;
      selfInitContext.ChildObjectives = _childObjectives;
      AuthorityInit(objectiveInfo.ToMinimalObjective(this), selfInitContext);
   }
}

void UTATRootPlayerObjective::_HandleStateChanged(const FTATQuestObjectiveState& oldState)
{
   Super::_HandleStateChanged(oldState);

   if(_combinedState.IsComplete != oldState.IsComplete)
   {
      if(auto* component = Cast<UTATPlayerQuestComponent>(GetOuter()))
      {
         component->NotifyObjectiveComplete(_combinedState.IsComplete, this);
      }
   }

   if(_combinedState.Progress != oldState.Progress)
   {
      if(auto* component = Cast<UTATPlayerQuestComponent>(GetOuter()))
      {
         component->NotifyObjectiveProgress(_combinedState.Progress, this);
      }
   }
}
