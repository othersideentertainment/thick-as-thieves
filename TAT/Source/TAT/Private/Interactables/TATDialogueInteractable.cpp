// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATDialogueInteractable.h"

// ue4
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDialogueInteractable)

void ATATDialogueInteractable::ValidateText(FName logCategory) const
{
   FMessageLog msgLog(logCategory);

   if (!DefaultDialogueText.IsFromStringTable())
   {
      msgLog.Error()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Dialogue text \"%s\" must live in a string table."), *DefaultDialogueText.ToString()))));
   }
}

#if WITH_EDITOR
void ATATDialogueInteractable::CheckForErrors()
{
   Super::CheckForErrors();
   ValidateText("MapCheck");
}
#endif // WITH_EDITOR

