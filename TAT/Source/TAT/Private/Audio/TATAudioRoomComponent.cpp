// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Audio/TATAudioRoomComponent.h"

// tat
#include "AI/Perception/TATNoisePropagationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAudioRoomComponent)


void UTATAudioRoomComponent::BeginPlay()
{
   Super::BeginPlay();

   if (UseForNoiseStimPropagation && !_hasRegisteredWithNoiseSubsystem)
   {
      if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
      {
         propagationSubsystem->RegisterRoomComponent(this);
      }

      _hasRegisteredWithNoiseSubsystem = true;
   }
}

void UTATAudioRoomComponent::OnRegister()
{
   Super::OnRegister();

   // Only do this for game worlds, since OnRegister can also be called in the Editor
   if (GetWorld()->IsGameWorld())
   {
      if (UseForNoiseStimPropagation && !_hasRegisteredWithNoiseSubsystem)
      {
         if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
         {
            propagationSubsystem->RegisterRoomComponent(this);
         }

         _hasRegisteredWithNoiseSubsystem = true;
      }
   }
}

void UTATAudioRoomComponent::EndPlay(EEndPlayReason::Type reason)
{
   if (UseForNoiseStimPropagation && ensure(_hasRegisteredWithNoiseSubsystem))
   {
      if (UTATNoisePropagationSubsystem* propagationSubsystem = GetWorld()->GetSubsystem<UTATNoisePropagationSubsystem>())
      {
         propagationSubsystem->UnregisterRoomComponent(this);
      }
   }

   Super::EndPlay(reason);
}
