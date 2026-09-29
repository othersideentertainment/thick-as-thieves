// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Tokens/TATInventoryToken.h"

// tat
#include "Items/Tokens/TATTokenEffect.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInventoryToken)

#if WITH_EDITOR
EDataValidationResult UTATInventoryToken::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if (Icon.IsNull())
   {
      context.AddWarning(INVTEXT("No Icon specified"));
   }

   if(const FTATTokenEffect* effect = Effect.GetPtr<FTATTokenEffect>())
   {
      effect->Validate([&context](const FText& message)
      {
         context.AddError(message);
      });
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
