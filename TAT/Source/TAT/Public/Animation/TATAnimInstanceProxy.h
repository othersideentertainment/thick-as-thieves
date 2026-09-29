// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"

// OSE
#include "Animation/Graph/OSEAnimInstanceProxy.h"

// TAT
#include "TATAnimInstanceProxy.generated.h"


//--------------------------------------------------------------------------------------------------
/// TAT-specific animation instance proxy
//--------------------------------------------------------------------------------------------------

USTRUCT()
struct TAT_API FTATAnimInstanceProxy : public FOSEAnimInstanceProxy
{
   GENERATED_BODY()

public:

   FTATAnimInstanceProxy();
   FTATAnimInstanceProxy(UAnimInstance* inAnimInstance);
};
