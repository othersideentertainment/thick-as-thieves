// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Upgrades/TATUpgradeType.h"

// ue
#include "UObject/ConstructorHelpers.h"
#include "PaperSprite.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeType)

UTATUpgradeType::UTATUpgradeType()
{
   struct FConstructorStatics
   {
      ConstructorHelpers::FObjectFinderOptional<UPaperSprite> DefaultIcon{ TEXT("/Paper2D/DummySprite.DummySprite") };
   };
   static FConstructorStatics sConstructorStatics;

   Icon = sConstructorStatics.DefaultIcon.Get();
}

#if WITH_EDITOR
EDataValidationResult UTATUpgradeType::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);
   if (Name.IsEmptyOrWhitespace())
   {
      context.AddWarning(INVTEXT("Upgrade type has an empty (or all-whitespace) name property"));
   }
   if (!UpgradeTag.IsValid())
   {
      context.AddError(INVTEXT("Upgrade type does not have a valid upgrade tag"));
   }
   return CombineDataValidationResults(baseResult,
      (context.GetNumWarnings() + context.GetNumErrors() == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}
#endif // WITH_EDITOR
