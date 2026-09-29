// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// wwise
#include "AkAcousticPortal.h"

#include "TATAudioPortalComponent.generated.h"

/// A TAT-level override of AkPortalComponent
/// This is meant to be a thin child class, focused on allowing us to track the connections between portals and rooms
/// for noise propagation
UCLASS(ClassGroup = TAT, hidecategories = (Advanced, Attachment, Volume), BlueprintType, meta = (BlueprintSpawnableComponent))
class TAT_API UTATAudioPortalComponent : public UAkPortalComponent
{
   GENERATED_BODY()
public:

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;

protected:
   virtual void _OnUpdateConnectedRooms() override;

private:
   bool _hasRegisteredWithSubsystem = false;
};
