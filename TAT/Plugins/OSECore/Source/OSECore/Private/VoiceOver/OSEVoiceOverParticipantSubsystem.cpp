// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverParticipantSubsystem.h"

// ue5
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverParticipantSubsystem)

bool UOSEVoiceOverParticipantSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   return !CastChecked<UWorld>(outer)->IsNetMode(NM_Client);
}

void UOSEVoiceOverParticipantSubsystem::RegisterParticipant(AActor* actor)
{
   check(!_participants.Contains(actor));
   _participants.Emplace(actor);
}

void UOSEVoiceOverParticipantSubsystem::UnregisterParticipant(AActor* actorToRemove)
{
   _participants.RemoveSingleSwap(actorToRemove);
}

void UOSEVoiceOverParticipantSubsystem::FindPossibleParticipantsInRange(const FVector& origin, float radius, const AActor* exclude, const TFunctionRef<void(AActor*)>& handler) const
{
   // Probably not a win to have to keep a spacial data structure up to date
   const float radiusSqr = FMath::Square(radius);

   for (TWeakObjectPtr<AActor> weakCandidate : _participants)
   {
      AActor* candidate = weakCandidate.Get();
      if (candidate == nullptr || candidate == exclude)
      {
         continue;
      }

      if (FVector::DistSquared(candidate->GetActorLocation(), origin) > radiusSqr)
      {
         continue;
      }

      handler(candidate);
   }
}

bool UOSEVoiceOverParticipantSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}
