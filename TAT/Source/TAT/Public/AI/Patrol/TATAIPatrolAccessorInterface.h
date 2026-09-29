// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATAIPatrolAccessorInterface.generated.h"

class APatrolPath;
// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTATAIPatrolAccessorInterface : public UInterface
{
   GENERATED_BODY()
};


class TAT_API ITATAIPatrolAccessorInterface
{
   GENERATED_BODY()
public:
   UFUNCTION(BlueprintNativeEvent)
   APatrolPath* GetPatrolPath() const;
   virtual APatrolPath* GetPatrolPath_Implementation() const = 0;
};
