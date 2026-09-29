// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"

#include "TATSmartObjectTagInterface.generated.h"

UINTERFACE()
class UTATSmartObjectTagInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATSmartObjectTagInterface
{
   GENERATED_BODY()

public:
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() = 0;
};
