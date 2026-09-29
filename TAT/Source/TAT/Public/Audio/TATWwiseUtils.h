// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// wwise
#include "AK/SoundEngine/Common/AkTypes.h"

// ue
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATWwiseUtils.generated.h"

class UAkAudioEvent;
class UAkSwitchValue;
class IWwiseSoundEngineAPI;

// TAT-level utility functions for wwise
UCLASS()
class TAT_API UTATWwiseUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   // Posts the event at the specified location, also setting the specified switch value
   // If no switch is provided, it will play the sound without it
   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Audiokinetic|TAT", meta=(WorldContext="worldContextObject"))
   static int32 PostEventAtLocationWithSwitch(UAkAudioEvent* akEvent, FVector location, FRotator orientation, const UAkSwitchValue* switchValue, const UObject* worldContextObject);

   // Could add other variations as needed, or an Ext version that takes a struct

private:
   // version of UAkAudioEvent::PostAtLocation that allows extra custom setup
   static AkPlayingID _PostEventAtLocationCustom(UAkAudioEvent* event, const FVector& location, const FRotator& orientation, const UObject* worldContext,
      TFunctionRef<void(IWwiseSoundEngineAPI&, AkGameObjectID)> setup);
};
