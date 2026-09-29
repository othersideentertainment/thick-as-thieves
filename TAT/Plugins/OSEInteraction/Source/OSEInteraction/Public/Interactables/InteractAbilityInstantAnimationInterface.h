// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "InteractAbilityInstantAnimationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UInteractAbilityInstantAnimationInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface to be implemented by abilities that are used for character-to-character interactions with custom animation
class OSEINTERACTION_API IInteractAbilityInstantAnimationInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability")
   FGameplayTag GetInteractInstantAnimation(const ACharacter* targetCharacter) const;
};
