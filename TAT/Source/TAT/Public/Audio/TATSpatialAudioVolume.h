// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"

// wwise
#include "AkSpatialAudioVolume.h"

#include "TATSpatialAudioVolume.generated.h"

class UTATSceneRequirementVisComponent;

/// A convenience child class of AkSpatialAudioVolume
/// Spatial Audio Volume actors are the most common way of placing a Room Component, so this class just overrides
/// the default room component with the TAT-level override
UCLASS(ClassGroup = TAT, Blueprintable, hidecategories = (Advanced, Attachment, Volume))
class TAT_API ATATSpatialAudioVolume : public AAkSpatialAudioVolume
{
   GENERATED_BODY()
public:
   ATATSpatialAudioVolume(const FObjectInitializer& objectInitializer);

   virtual void BeginPlay() override;

private:
   void _ResolveRoomSound();

   // Audio event played when scene requirement not met
   UPROPERTY(EditAnywhere, Category="PlayRoomSoundRequirement", meta=(DisplayName = "Alternate Audio Event",  DisplayAfter="_playIfNoRequirement"))
	TObjectPtr<UAkAudioEvent> _alternateAudioEvent = nullptr;
   
   UPROPERTY(EditAnywhere, Category=PlayRoomSoundRequirement, meta = (ShowOnlyInnerProperties))
   FTATSceneRequirement _playRoomSoundRequirement;

   // whether to auto-play if no requirement set
   UPROPERTY(EditAnywhere, Category=PlayRoomSoundRequirement, meta = (DisplayName="Play if No Requirement"))
   bool _playIfNoRequirement = false;

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   UTATSceneRequirementVisComponent* _sceneRequirementVisComponent = nullptr;
#endif
};
