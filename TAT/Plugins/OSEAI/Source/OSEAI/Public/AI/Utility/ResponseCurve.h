// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"

#include "ResponseCurve.generated.h"

UCLASS()
class UResponseCurve : public UCurveFloat
{
   GENERATED_BODY()

public:

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
#endif

};

