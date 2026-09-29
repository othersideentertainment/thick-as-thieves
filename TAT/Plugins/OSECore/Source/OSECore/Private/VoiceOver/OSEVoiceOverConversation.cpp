// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverConversation.h"

// ose
#include "VoiceOver/OSEVoiceOverLine.h"

// ue4
#include "AkAudioEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverConversation)

UOSEVoiceOverConversationNode* UOSEVoiceOverConversation::ChooseStartingNode(const TArray<AActor*>& participants, FRandomStream& randomStream) const
{
   if (ConversationRoots.Num() == 0)
      return nullptr;

   return UOSEVoiceOverConversationNode::SelectRandomValidNode(ConversationRoots, participants, randomStream);
}

float UOSEVoiceOverConversation::GetParticipantSearchRadius() const
{
   if (ConversationRoots.Num() == 0)
   {
      return 0; // Use default
   }

   const UOSEVoiceOverConversationNode* node = ConversationRoots[0];

   if (!IsValid(node))
   {
      return 0; //Use default.
   }

   auto identityIt = node->Line->Identities.CreateConstIterator();

   if (!identityIt)
   {
      return 0; // Use Default
   }

   UAkAudioEvent* akEvent = identityIt->Value.AudioEvent;

   if (!IsValid(akEvent))
   {
      return 0; // Use Default
   }

   return akEvent->MaxAttenuationRadius;

}

