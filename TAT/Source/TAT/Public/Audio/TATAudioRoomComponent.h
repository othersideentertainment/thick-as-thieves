// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// wwise
#include "AkRoomComponent.h"

#include "TATAudioRoomComponent.generated.h"

/// A TAT-level override of AkRoomComponent
/// This is meant to be a thin child class, focused on allowing us to track the connections between portals and rooms
/// for noise propagation
UCLASS(ClassGroup = TAT, BlueprintType, hidecategories = (Transform, Rendering, Mobility, LOD, Component, Activation, Tags), meta = (BlueprintSpawnableComponent))
class TAT_API UTATAudioRoomComponent : public UAkRoomComponent
{
   GENERATED_BODY()
public:
   virtual void BeginPlay() override;
   virtual void OnRegister() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;

   /// If true, then we will use this room and its portal connection to propagate noise stims
   /// Defaults to true
   UPROPERTY(EditAnywhere, Category = "Noise Propagation")
   bool UseForNoiseStimPropagation = true;

private:
   bool _hasRegisteredWithNoiseSubsystem = false;
};
