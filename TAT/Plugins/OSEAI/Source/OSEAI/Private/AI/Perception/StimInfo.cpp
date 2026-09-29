// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Perception/StimInfo.h"

// ue
#include "Algo/Rotate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StimInfo)

FStimInfo FStimInfo::Invalid = FStimInfo();

const AActor* FStimInfo::GetNextResponsiveActor() const
{
   return ResponsiveActors.Num() > 0 ? ResponsiveActors[0].Get() : nullptr;
}

void FStimInfo::OnResponsiveActorCheckedForGoals(AActor* actor)
{
   const AActor* nextResponsiveActor = GetNextResponsiveActor();
   if (nextResponsiveActor != actor)
      return;

   // if this stim is not yet investigated...
   if (InvestigationState == EStimInvestigationState::NotInvestigated)
   {
      // if it's still not under investigation by this AI, we pop off the front
      ResponsiveActors.RemoveAt(0);

      // if we have no one else to respond to this, just consider it investigated, no one handled it
      if (ResponsiveActors.Num() == 0)
      {
         InvestigationState = EStimInvestigationState::Investigated;
      }
   }
}

FString FStimInfo::ToString() const
{
   return FString::Printf(TEXT("[%d] %s \"%s\" (%s) [%s] [Instigator: %s]"), Id, *UEnum::GetValueAsString(Type), *Tag.ToString(), *Location.ToString(), *UEnum::GetValueAsString(InvestigationState), Instigator.IsValid() ? *Instigator->GetName() : TEXT("NULL"));
}

