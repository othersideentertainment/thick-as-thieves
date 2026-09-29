// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "InteractHoldAbilityInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UInteractHoldAbilityInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface to be implemented by abilities that are used for character-to-character interactions for hold time
class OSEINTERACTION_API IInteractHoldAbilityInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability")
   float GetInteractHoldTime(const ACharacter* interactingCharacter, const ACharacter* targetCharacter) const;

   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability")
   TSubclassOf<UGameplayEffect> GetInteractHoldTargetEffect(const ACharacter* targetCharacter) const;

   UFUNCTION(BlueprintNativeEvent, Category = "Interactable|Ability")
   FGameplayTag GetInteractHoldAnimation(const ACharacter* targetCharacter) const;

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable|Ability")
   void OnInteractionStart(ACharacter* interactingCharacter, ACharacter* targetCharacter) const;

   
   /**
    * @param interactingCharacter The character interacting with the ability
    * @param targetCharacter The target character of the interaction
    * @param endContext The context of the interaction provided from the target character
    * @return If we return true, we've handled the interaction, else we need to continue execution
    */
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable|Ability")
   bool OnInteractionCompleted(ACharacter* interactingCharacter, ACharacter* targetCharacter, const FInteractEndContext& endContext) const;
   virtual bool OnInteractionCompleted_Implementation(ACharacter* interactingCharacter, ACharacter* targetCharacter, const FInteractEndContext& endContext) const { return false; }

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable|Ability")
   bool IsCharacterAllowedToMoveDuringHoldInteraction(ACharacter* interactingCharacter, ACharacter* targetCharacter) const;
   virtual bool IsCharacterAllowedToMoveDuringHoldInteraction_Implementation(ACharacter* interactingCharacter, ACharacter* targetCharacter) const { return false; }

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable|Ability")
   bool ShouldAbilityBlockPlayerPromptsWhilstHeld() const;
   virtual bool ShouldAbilityBlockPlayerPromptsWhilstHeld_Implementation() const { return false; }
};
