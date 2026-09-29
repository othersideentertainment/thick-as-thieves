// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Audio/TATAudioPortalComponent.h"

// tat
#include "AI/Perception/TATNoisePropagationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAudioPortalComponent)


void UTATAudioPortalComponent::BeginPlay()
{
   Super::BeginPlay();

   if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
   {
      propagationSubsystem->RegisterPortalComponent(this);
      _hasRegisteredWithSubsystem = true;
   }

}

void UTATAudioPortalComponent::EndPlay(EEndPlayReason::Type reason)
{
   if (_hasRegisteredWithSubsystem)
   {
      if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
      {
         propagationSubsystem->UnregisterPortalComponent(this);
      }
   }

   Super::EndPlay(reason);
}

void UTATAudioPortalComponent::_OnUpdateConnectedRooms()
{
   if (_hasRegisteredWithSubsystem)
   {
      if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
      {
         propagationSubsystem->OnPortalComponentRoomConnectionsUpdated(this);
      }
   }
}
