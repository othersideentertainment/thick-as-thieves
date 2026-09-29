// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Camera/CameraTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_CameraShake.generated.h"

class UCameraShakeBase;

//---------------------------------------------------------------------------------------
// UAnimNotify_CameraShake
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "CameraShake"))
class OSECORE_API UAnimNotify_CameraShake : public UAnimNotify
{
   GENERATED_BODY()

public:

   UAnimNotify_CameraShake(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   virtual FString GetNotifyName_Implementation() const override;
   virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

   UPROPERTY(EditAnywhere, Category = "Camera Shake")
   TSubclassOf<UCameraShakeBase> CameraShakeClass;
   UPROPERTY(EditAnywhere, Category = "Camera Shake")
   float Scale = 1.0f;
   UPROPERTY(EditAnywhere, Category = "Camera Shake")
   ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal;
   UPROPERTY(EditAnywhere, Category = "Camera Shake")
   FRotator UserPlaySpaceRot = FRotator::ZeroRotator;
};
