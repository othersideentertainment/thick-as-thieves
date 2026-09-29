// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATGameCredits.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameCredits)

// static
void UTATGameCreditHelpers::GetAllGameCreditRowNames(UDataTable* creditsDataTable, TArray<FName>& rowNames)
{
   rowNames.Empty();
   if (!IsValid(creditsDataTable))
   {
      return;
   }

   // this would be an excellent place to do any sorting if required

   creditsDataTable->ForeachRow<FTATGameCredit>(TEXT("UTATGameCreditHelpers::GetAllGameCreditRowNames"),
      [&](const FName& rowName, const FTATGameCredit& row)
      {
         if (row.Enabled)
         {
            rowNames.Add(rowName);
         }
      });
}
