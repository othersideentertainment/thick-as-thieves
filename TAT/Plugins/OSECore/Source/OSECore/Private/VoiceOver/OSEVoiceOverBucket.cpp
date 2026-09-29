// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverBucket.h"

//ue4
#include "AkAudioEvent.h"
#include "AkComponent.h"

// ose
#include "VoiceOver/OSEVoiceOverLine.h"
#include "VoiceOver/OSEVoiceOverConversation.h"
#include "Audio/OSEAkAudioComponentSystemInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverBucket)


UAkAudioEvent* UOSEVoiceOverBucket::GetMostAudibleEvent(const AActor* speaker) const
{
   if (!IsValid(speaker))
   {
      return nullptr;
   }

   if (!speaker->Implements<UOSEAkAudioComponentSystemInterface>())
   {
      return nullptr;
   }

   UAkComponent* voComponent = IOSEAkAudioComponentSystemInterface::Execute_GetAkComponent(speaker, EAkComponentType::Voice);
   if (!IsValid(voComponent))
   {
      return nullptr;
   }

   float maxAudibility = 0;
   UAkAudioEvent* maxEvent = nullptr;

   for (const FOSEVoiceOverBucketEntry& entry : Entries)
   {
      FOSEVoiceOverLineIdentityData* identityData = nullptr;

      if (UOSEVoiceOverLine* line = Cast<UOSEVoiceOverLine>(entry.VoiceItem))
      {
         identityData = line->GetIdentityData(voComponent);
      }
      else if (UOSEVoiceOverConversation* conversation = Cast<UOSEVoiceOverConversation>(entry.VoiceItem))
      {
         // This has a subtle requirement that the root conversation node must always be usable on the requesting VO actor.
         // This ultimately needs a better solutioon but that's for a later change.
         UOSEVoiceOverConversationNode* rootNode = conversation->ConversationRoots[0];
         if (IsValid(rootNode->Line))
         {
            identityData = rootNode->Line->GetIdentityData(voComponent);
         }
      }

      if (identityData != nullptr)
      {
         float radius = identityData->AudioEvent->MaxAttenuationRadius;
         if (radius > maxAudibility)
         {
            maxAudibility = radius;
            maxEvent = identityData->AudioEvent;
         }
      }
   }

   return maxEvent;
}

