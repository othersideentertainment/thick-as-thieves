// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATInteractionFunctionLibrary.h"

// ue5
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractionFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogTATInteractionFunctionLibrary, Log, All)

bool UTATInteractionFunctionLibrary::StartInteractionWithInteractableByCharacter(TScriptInterface<IInteractableInterface> interactable, ACharacter* interactor, FInteractStartResult& interactStartResult)
{
   if (!interactable.GetObject())
   {
      UE_LOG(LogTATInteractionFunctionLibrary, Error,
         TEXT("StartInteractionWithInteractableByCharacter called on non-interactable or null '%s'"), *GetNameSafe(interactable.GetObject()));
      return false;
   }

   interactStartResult = IInteractableInterface::Execute_StartInteract(interactable.GetObject(), interactor);
   return true;
}
