// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/TATPlayerPostProc.h"

// unreal
#include "Components/PostProcessComponent.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerPostProc)

ATATPlayerPostProc::ATATPlayerPostProc()
{
   PostProcComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
   PostProcComponent->bUnbound = true;

   RootComponent = PostProcComponent;
}

#if WITH_EDITOR
EDataValidationResult ATATPlayerPostProc::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if (!PostProcComponent->bUnbound)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Postproc component for %s must be unbound"), *GetName())));
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void ATATPlayerPostProc::SetVisibility(bool shouldBeVisible)
{
   if (_isVisible != shouldBeVisible)
   {
      _isVisible = shouldBeVisible;

      OnVisibilityChange(_isVisible);
   }
}
