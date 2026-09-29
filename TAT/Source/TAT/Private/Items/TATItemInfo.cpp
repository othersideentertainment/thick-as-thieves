// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/TATItemInfo.h"

// tat
#include "Items/TATItemActor.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemInfo)

DEFINE_LOG_CATEGORY(LogTATItems);


bool UTATItemInfo::ConvertsToGoldOnMissionComplete() const
{
   // TODO: use loot category when it exists
   return !KeepOnMissionSuccess && GoldValue > 0;
}

#if WITH_EDITOR
EDataValidationResult UTATItemInfo::IsDataValid(FDataValidationContext& context) const
{
   // only check if present to tolerate abstract classes, etc
   if (!ItemActor.IsNull())
   {
      const ATATItemActor* itemActor = Cast<ATATItemActor>(ItemActor.LoadSynchronous()->GetDefaultObject());
      // if there are situations where we want this to vary, can add an opt out later
      if (itemActor != nullptr && itemActor->GetItemInfo() != GetClass())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] Item actor %s does not have self as ItemInfo"), *GetName(), *itemActor->GetName())));
      }
   }

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract))
   {
      if (InventoryDestination == ETATInventoryDestination::ToolbeltIfPossible && ToolToGrant.IsNull())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] non-abstract toolbelt item has no tool specified"), *GetName())));
      }
   }

   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

