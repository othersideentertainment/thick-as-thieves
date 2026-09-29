// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Perception/OSEAIPerceptionComponent.h"

// ose
#include "AI/Perception/AISense_VisualEvent.h"
#include "AI/Perception/OSEAISense_Team.h"
#include "AI/Perception/OSEAISenseConfig_Sight.h"

// ue4
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Touch.h"
#include "Perception/AISenseConfig_Sight.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAIPerceptionComponent)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

void UOSEAIPerceptionComponent::BeginPlay()
{
   Super::BeginPlay();

   OnTargetPerceptionUpdated.AddUniqueDynamic(this, &UOSEAIPerceptionComponent::_OnTargetPerceptionUpdated);
   OnTargetPerceptionInfoUpdated.AddUniqueDynamic(this, &UOSEAIPerceptionComponent::_OnTargetPerceptionInfoUpdated);
}

bool UOSEAIPerceptionComponent::GetActorsLastStimulus(AActor* actor, TSubclassOf<UAISense> senseToUse, FAIStimulus& stimulus) const
{
   if (!senseToUse || !IsValid(actor))
   {
      return false;
   }

   if (const FActorPerceptionInfo* perceivedInfo = GetActorInfo(*actor))
   {
      const FAISenseID senseID = UAISense::GetSenseID(senseToUse);
      if (perceivedInfo->HasKnownStimulusOfSense(senseID))
      {
         stimulus = perceivedInfo->LastSensedStimuli[senseID];
         return true;
      }
   }
   return false;
}

bool UOSEAIPerceptionComponent::GetLastSensedActorLocation(AActor* actor, TSubclassOf<UAISense> senseToUse, FVector& lastSensedLocation) const
{
   if (!IsValid(actor))
   {
      return false;
   }

   if (const FActorPerceptionInfo* perceivedInfo = GetActorInfo(*actor))
   {
      const FAISenseID senseID = UAISense::GetSenseID(senseToUse);
      lastSensedLocation = senseToUse ? perceivedInfo->GetStimulusLocation(senseID) : perceivedInfo->GetLastStimulusLocation();
      return true;
   }
   return false;
}

bool UOSEAIPerceptionComponent::IsActorCurrentlyPerceived(AActor* actor, TSubclassOf<UAISense> senseToUse) const
{
   if (!IsValid(actor))
   {
      return false;
   }

   if (const FActorPerceptionInfo* perceivedInfo = GetActorInfo(*actor))
   {
      const FAISenseID senseID = UAISense::GetSenseID(senseToUse);
      return senseToUse ? perceivedInfo->IsSenseActive(senseID) : perceivedInfo->HasAnyCurrentStimulus();
   }
   return false;
}

void UOSEAIPerceptionComponent::SetAllSensesEnabled(bool enabled)
{
   for(UAISenseConfig* senseConfig : SensesConfig)
   {
      if (!senseConfig)
         continue;
      
      UpdatePerceptionAllowList(senseConfig->GetSenseID(), enabled);
   }
}

void UOSEAIPerceptionComponent::_OnTargetPerceptionUpdated(AActor* actor, FAIStimulus stimulus)
{
   // Ignore stims from us or instigated by us.
   AActor* owningPawn = AIOwner ? AIOwner->GetPawn() : nullptr;
   if (owningPawn && actor && (actor == owningPawn || actor->GetInstigator() == owningPawn))
   {
      return;
   }

   if (stimulus.Type == UAISense::GetSenseID<UOSEAISense_Sight>())
   {
      OnTargetSightPerceptionUpdated.Broadcast(actor, stimulus);
   }
   else if (stimulus.Type == UAISense::GetSenseID<UAISense_VisualEvent>())
   {
      OnVisualStimEvent.Broadcast(actor, stimulus);
   }
   else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
   {
      OnHearingEvent.Broadcast(actor, stimulus);
   }
   else if (stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
   {
      OnDamageEvent.Broadcast(actor, stimulus);
   }
   else if (stimulus.Type == UAISense::GetSenseID<UOSEAISense_Team>())
   {
      OnTeamStimEvent.Broadcast(actor, stimulus);
   }
   else if (stimulus.Type == UAISense::GetSenseID<UAISense_Touch>())
   {
      OnTouchStimEvent.Broadcast(actor, stimulus);
   }
}

void UOSEAIPerceptionComponent::_OnTargetPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& updateInfo)
{
   // TODO: Should we be using this?
}
