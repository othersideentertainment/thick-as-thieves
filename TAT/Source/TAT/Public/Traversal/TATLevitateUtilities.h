// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TATLevitateUtilities.generated.h"


class UMovementComponent;

UCLASS()
class TAT_API UTATLevitateUtilities : public UObject
{
   GENERATED_BODY()


public:
   UFUNCTION(BlueprintCallable)
   static void AttachToLevitatingBase(UMovementComponent* movement, const AActor* base);

   UFUNCTION(BlueprintCallable)
   static void DetachFromLevitatingBase(UMovementComponent* movement);
   
};
