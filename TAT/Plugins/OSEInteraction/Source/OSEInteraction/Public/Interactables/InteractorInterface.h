// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "InteractableInterface.h"

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/Character.h"

#include "InteractorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UInteractorInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 * 
 */
class OSEINTERACTION_API IInteractorInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   ACharacter* GetCharacter() const;

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   TScriptInterface<IInteractableInterface> GetCurrentInteractable() const;
};
