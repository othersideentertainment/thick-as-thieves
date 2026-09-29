// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "InteractPromptAbilityInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UInteractPromptAbilityInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface to be implemented by abilities that are used for character-to-character interactions for their specify prompt
class OSEINTERACTION_API IInteractPromptAbilityInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability")
   FText GetInteractPromptVerb() const;

   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability", meta=(GameplayTagFilter="InteractAction"))
   FGameplayTag GetInteractPromptActionTag() const;
};
