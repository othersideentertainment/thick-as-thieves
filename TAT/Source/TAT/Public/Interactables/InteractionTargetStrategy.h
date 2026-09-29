// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionTargetStrategy.generated.h"


// This class does not need to be modified.
UINTERFACE(BlueprintType)
class TAT_API UInteractionTargetStrategy : public UInterface
{
   GENERATED_BODY()
};

/**
 *
 */
class TAT_API IInteractionTargetStrategy
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable|Targeting")
   TScriptInterface<IInteractableInterface> FindTargetInteractable(ACharacter* InteractingCharacter) const;
};
