// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotify_ForceControllerRotation.generated.h"

UCLASS()
class OSEAI_API UAnimNotify_ForceControllerRotation : public UAnimNotifyState
{
   GENERATED_BODY()
public:
   virtual void NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

protected:
   UPROPERTY(EditDefaultsOnly)
   FName _lookAroundYawCurveName { TEXT("LookaroundYaw") };
   UPROPERTY(EditDefaultsOnly)
   FName _lookAroundPitchCurveName { TEXT("LookaroundPitch") };
};
