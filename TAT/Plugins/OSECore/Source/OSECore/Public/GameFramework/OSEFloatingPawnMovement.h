// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "OSEFloatingPawnMovement.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class OSECORE_API UOSEFloatingPawnMovement : public UFloatingPawnMovement
{
   GENERATED_BODY()
   
   //Begin UActorComponent Interface
   virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
   //End UActorComponent Interface
};
