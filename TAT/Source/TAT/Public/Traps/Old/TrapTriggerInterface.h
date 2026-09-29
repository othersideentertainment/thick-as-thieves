// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UObject/Interface.h"
#include "TrapTriggerInterface.generated.h"

UENUM(BlueprintType)
enum class ETrapTriggerType : uint8
{
   Mechanical,
   Magical
};

UINTERFACE()
class UTrapTriggerInterface_Old : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITrapTriggerInterface_Old
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
   ETrapTriggerType GetTriggerType() const;

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
   bool IsArmed() const;
};
