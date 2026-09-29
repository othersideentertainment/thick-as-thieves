// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Audio/TATSpatialAudioVolume.h"

// tat
#include "Audio/TATAudioRoomComponent.h"
#include "Variation/SceneVariants/TATSceneRequirementVisComponent.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"

// wwise
#include "AkAudioEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpatialAudioVolume)

ATATSpatialAudioVolume::ATATSpatialAudioVolume(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
      .SetDefaultSubobjectClass<UTATAudioRoomComponent>(TEXT("Room")))
{
#if WITH_EDITORONLY_DATA
   _sceneRequirementVisComponent = CreateEditorOnlyDefaultSubobject<UTATSceneRequirementVisComponent>(TEXT("SceneRequirementVisualizer"));
#endif

   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;

   // Don't need to update this every frame
   // CONSIDER: don't tick on dedicated server?
   if (LateReverb)
   {
      // This matches the recalculate interval (could also do a fraction of this)
      // NOTE: Just setting this does make them clump together on a single frame, but the spike is still low
      //       enough that it is probably fine
      // TODO: Spread it out
      LateReverb->PrimaryComponentTick.TickInterval = 0.1f;
   }
}

void ATATSpatialAudioVolume::BeginPlay()
{
   Super::BeginPlay();

   _ResolveRoomSound();

}

void ATATSpatialAudioVolume::_ResolveRoomSound()
{
   if (Room->AutoPost)
   {
      return;
   }

   if (_playRoomSoundRequirement.IsNone())
   {
      if (_playIfNoRequirement)
      {
         Room->PostAssociatedAkEvent(0, FOnAkPostEventCallback());
      }
      return;
   }

   if (UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _playRoomSoundRequirement))
   {
      Room->PostAssociatedAkEvent(0, FOnAkPostEventCallback());
   }
   else if (_alternateAudioEvent)
   {
      Room->PostAkEvent(_alternateAudioEvent);
   }
}
