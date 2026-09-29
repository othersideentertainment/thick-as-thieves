// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATInteractionFunctionLibrary.generated.h"


UCLASS()
class TAT_API UTATInteractionFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // Just a passthrough now, maybe inline and delete later
   UFUNCTION(BlueprintCallable, Category = "TAT|Interaction")
   static bool StartInteractionWithInteractableByCharacter(TScriptInterface<IInteractableInterface> interactable, ACharacter* interactor, FInteractStartResult& interactStartResult);
};
