// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TATInteractHighlightUtils.generated.h"

/**
 * Some game-specific helper methods for interactables
 */
UCLASS()
class TAT_API UTATInteractHighlightUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static const FName InteractHighlightTag_NAME;

   // Set highlight for all primitive components in the actor with the "Interact" tag
   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Meta=(DefaultToSelf=Actor))
   static void HighlightInteractMeshes(AActor* actor, bool bIsHighlighted);
   
};
