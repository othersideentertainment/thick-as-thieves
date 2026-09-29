// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATClueObjective.h"

// tat
#include "Variation/Clues/TATKnownCluesComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueObjective)

struct FTATClueObjectiveTrackerParams : FTATObjectiveTrackerParams
{
   TAT_DEFINE_TRACKER_PARAMS(FTATClueObjectiveTrackerParams)
   
   FGameplayTagContainer RequiredClueFacts;
   bool FailIfCaught = false;

   FTATClueObjectiveTrackerParams(const FGameplayTagContainer& requiredClueFacts, bool failIfCaught)
      : RequiredClueFacts(requiredClueFacts)
      , FailIfCaught(failIfCaught)
   {}
};

FText FTATClueObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   return ObjectiveText;
}

FTATObjectiveTrackerPayload FTATClueObjectiveInfo::CreateTracker() const
{
   return FTATObjectiveTrackerPayload {
      .TrackerClass = UTATClueObjectiveTracker::StaticClass(),
      .Params = MakeShared<FTATClueObjectiveTrackerParams>(RequiredClueFacts, FailIfCaught)
   };
}

#if WITH_EDITOR
void FTATClueObjectiveInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ObjectiveText.IsEmpty())
   {
      reportError(INVTEXT("Objective text is empty"));
   }

   if(RequiredClueFacts.IsEmpty())
   {
      reportError(INVTEXT("There are no required clue facts"));
   }
}
#endif

void UTATClueObjectiveTracker::Initialize(const FTATObjectiveTrackerContext& context)
{
   const FTATClueObjectiveTrackerParams* clueParams = context.GetParams<FTATClueObjectiveTrackerParams>();
   _requiredClueFacts = clueParams->RequiredClueFacts;
   _questTag = context.QuestTag;
   _failIfCaught = clueParams->FailIfCaught;
   
   _knownClues = UTATKnownCluesComponent::Get(context.PlayerState);
   if(ensure(_knownClues))
   {
      _knownClues->OnCluesUpdated.AddUniqueDynamic(this, &ThisClass::_RefreshObjective);
      _RefreshObjective(_knownClues);
   }
}

bool UTATClueObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   return IsComplete() && (escaped || !_failIfCaught);
}

void UTATClueObjectiveTracker::CheatComplete()
{
   check(_knownClues);
   for(const FGameplayTag& requiredFact : _requiredClueFacts)
   {
      if(!_knownClues->HasFactWithTag(_questTag, requiredFact))
      {
         _knownClues->AddSyntheticFact(_questTag, requiredFact);
      }
   }
}

void UTATClueObjectiveTracker::_RefreshObjective(UTATKnownCluesComponent* knownClues)
{
   _SetIsComplete(_CalculateObjectiveComplete());
}

bool UTATClueObjectiveTracker::_CalculateObjectiveComplete() const
{
   check(_knownClues);
   for(const FGameplayTag& requiredFact : _requiredClueFacts)
   {
      if(!_knownClues->HasFactWithTag(_questTag, requiredFact))
      {
         return false;
      }
   }

   return true;
}
