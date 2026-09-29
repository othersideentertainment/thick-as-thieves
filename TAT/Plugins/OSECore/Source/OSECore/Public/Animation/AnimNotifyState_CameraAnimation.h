// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Camera/PlayerCameraAnim.h"
#include "AnimNotifyState_CameraAnimation.generated.h"


/// Overrides player camera animation settings for the duration of the notify state
UCLASS(deprecated, Blueprintable, meta = (DisplayName = "Player Camera Animation"))
class OSECORE_API UDEPRECATED_AnimNotifyState_CameraAnimation : public UAnimNotifyState
{
   GENERATED_BODY()

public:

   UDEPRECATED_AnimNotifyState_CameraAnimation() : Super() { }

protected:

   /// Used to fine-tune the animation notify duration if the time is not accurately reflected
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
   float TimeScale = 1.0f;

   /// When blending in, alpha proceeds from 0 to 1 over this time
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
   float AlphaInTime = 0.1f;

   /// When blending out, alpha proceeds from 1 to 0 over this time
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
   float AlphaOutTime = 0.1f;

   /// New animation settings to set
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
   FPlayerCameraAnimation Settings = FPlayerCameraAnimation(1.0f, 1.0f, 0.4f, 0.0f);
};
