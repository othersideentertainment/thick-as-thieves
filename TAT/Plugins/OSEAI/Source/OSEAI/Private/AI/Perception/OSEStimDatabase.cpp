// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/OSEStimDatabase.h"

// ose
#include "AI/OSEAISettings.h"
#include "AI/Perception/StimInfo.h"
#include "AI/Perception/OSEStimDatabaseInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEStimDatabase)

void UOSEStimDatabase::UpdateDatabase(float deltaTime, float now)
{
   const UOSEAISettings& settings = UOSEAISettings::Get();

   // Forget stims that are:
   // - old
   // - already investigated, we don't need them right?
   // TODO: We probably want to retain non-ephemeral stims. Like bloodstains on the floor, arrows in the wall, etc. And only forget events, such as sounds, visuals, damage.
   // TODO: This logic probably wants to live game-side instead?
   _stims.RemoveAll([&](FStimInfo& stim)
   {
      if (!stim.CanBeForgotten)
      {
         return false;
      }

      // do not remove stims that are under investigation, let the investigation complete
      if (stim.InvestigationState == EStimInvestigationState::UnderInvestigation)
         return false;

      // remove stims that are already investigated, or are old at this point
      const bool removeStim = (stim.InvestigationState == EStimInvestigationState::Investigated) ||
                              (now > stim.Timestamp + settings.MaxStimAge);

      if (removeStim)
      {
         if (IOSEStimDatabaseOwnerInterface* stimDataBaseOwner = Cast<IOSEStimDatabaseOwnerInterface>(GetOuter()))
         {
            stimDataBaseOwner->AuthorityOnStimAboutToBeRemovedFromDatabase(stim);
         }
      }

      return removeStim;
   });
}

FStimInfo& UOSEStimDatabase::AddStimInfo(AActor* perceivedBy, const FVector& location, AActor* instigator, EStimType stimType, FName tag, EStimSeverity stimSeverity, float strength, int32 globalId)
{
   const float now = GetWorld()->GetTimeSeconds(); // TODO: use stim age instead?
   const uint32 stimId = _nextStimId++;
   FStimInfo& newStim = _stims.Emplace_GetRef(stimId, location, instigator, stimType, stimSeverity, tag, strength, now, globalId);

   if (IOSEStimDatabaseOwnerInterface* stimDataBaseOwner = Cast<IOSEStimDatabaseOwnerInterface>(GetOuter()))
   {
      stimDataBaseOwner->AuthorityOnStimAddedToDatabase(newStim);
   }

   AddStimPerceivedActor(newStim, perceivedBy);

   return newStim;
}

void UOSEStimDatabase::AddStimPerceivedActor(FStimInfo& stim, AActor* perceivedBy)
{
   stim.PerceivedByActors.Add(perceivedBy);

   if (IOSEStimDatabaseOwnerInterface* stimDataBaseOwner = Cast<IOSEStimDatabaseOwnerInterface>(GetOuter()))
   {
      stimDataBaseOwner->AuthorityOnStimPerceivedByActorsChanged(stim, perceivedBy);
   }
}

const FStimInfo* UOSEStimDatabase::FindStimInfo(int stimId) const
{
   return _stims.FindByPredicate([stimId](const FStimInfo& stim)
   {
      return stim.Id == stimId;
   });
}

const FStimInfo* UOSEStimDatabase::FindStimInfo(const FStimDatabaseQuery& query) const
{
   return _stims.FindByPredicate([&query](const FStimInfo& stim)
   {
      return query.QueryMatches(stim);
   });
}

const FStimInfo* UOSEStimDatabase::FindNewestStim() const
{
   const FStimInfo* newestStim = nullptr;
   for (const FStimInfo& stim : _stims)
   {
      if (!newestStim || stim.Timestamp > newestStim->Timestamp)
      {
         newestStim = &stim;
      }
   }
   return newestStim;
}

