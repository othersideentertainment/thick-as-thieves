// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/ResponseCurve.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ResponseCurve)

#if WITH_EDITOR
EDataValidationResult UResponseCurve::IsDataValid(FDataValidationContext& context)
{
   // Verify that all keys are in the range [0, 1] on both axes.
   float min, max;

   FloatCurve.GetTimeRange(min, max);
   if (min < -KINDA_SMALL_NUMBER || max > (1.0f + KINDA_SMALL_NUMBER))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Curve time range must be between [0, 1] inclusive. It is currently: [%0.2f, %0.2f]"), min, max)));
   }

   FloatCurve.GetValueRange(min, max);
   if (min < -KINDA_SMALL_NUMBER || max > (1.0f + KINDA_SMALL_NUMBER))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Curve value range must be between [0, 1] inclusive. It is currently: [%0.2f, %0.2f]"), min, max)));
   }

   return context.GetNumErrors() + context.GetNumWarnings() > 0 ?
      EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

