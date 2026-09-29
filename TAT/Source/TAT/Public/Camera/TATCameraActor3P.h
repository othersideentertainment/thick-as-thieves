// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// OSE
#include "Camera/OSECameraActor3P.h"

// UE
#include "CoreMinimal.h"
#include "TATCameraActor3P.generated.h"

UCLASS()
class TAT_API ATATCameraActor3P : public AOSECameraActor3P
{
   GENERATED_BODY()
protected:
   virtual const AActor* GetTargetActorForCamera() const override;
};
