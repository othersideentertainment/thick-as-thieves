// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEHeldActionCues.h"

// ue
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "InteractableInterface.generated.h"

class UGameplayAbility;
class ACharacter;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FInteractStartResult
{
   GENERATED_BODY()

   FInteractStartResult() {}

   FInteractStartResult(bool waitForDelay, float delay)
      : bWaitForDelay(waitForDelay), Delay(delay)
   {}

   static FInteractStartResult Wait(float delay)
   {
      return FInteractStartResult(true, delay);
   }

public:
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   bool bWaitForDelay = false;

   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   float Delay = 0;

   /// Tag representing the animation to be played on instant interaction
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   FGameplayTag InstantAnimationTag;

   /// Tag representing the animation to be played on hold interaction
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   FGameplayTag HoldAnimationTag;

   // Struct of cues to be played on held action start / complete / interrupt.
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   FOSEHeldActionCues HoldActionCues;

   // Effect to be applied to target on hold interaction (re-evaluate this approach later, may not be useful/generalizable enough to justify)
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   TSubclassOf<UGameplayEffect> HoldTargetEffect;

   // Effect to be applied to the interacting character on hold interaction
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   TSubclassOf<UGameplayEffect> HoldSourceEffect;

   // Message shown after interact
   // If not a timed interaction, it is shown transiently
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   FText Message;
   
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   bool IsCharacterAllowedToMoveDuringInteraction { false };

   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   const UGameplayAbility* HoldGameplayAbility { nullptr };
};

// Data for constructing the prompt message
// Should be able to be populated cheaply, as it may be polled frequently to detect changes
USTRUCT(BlueprintType)
struct FInteractPrompt
{
   GENERATED_BODY()

   // Message shown for the press
   UPROPERTY(BlueprintReadWrite)
   FText PressAction;

   // Message shown for the hold
   UPROPERTY(BlueprintReadWrite)
   FText HoldAction;

   // Message shown when there is an error
   UPROPERTY(BlueprintReadWrite)
   FText ErrorMessage;

   // Tag that identifies what Interact Prompt Data this prompt should use for Press Actions
   UPROPERTY(BlueprintReadWrite)
   FGameplayTag PressActionTag;

   // Tag that identifies what Interact Prompt Data this prompt should use for Hold Actions
   UPROPERTY(BlueprintReadWrite)
   FGameplayTag HoldActionTag;

   // Tag associating a status with the interaction for displaying a custom visualization (e.g. icon)
   UPROPERTY(BlueprintReadWrite, AdvancedDisplay)
   FGameplayTag InteractStatusTag;

   bool IsValid() const
   {
      return !PressAction.IsEmpty() || !HoldAction.IsEmpty() || !ErrorMessage.IsEmpty();
   }

   bool IdenticalTo(const FInteractPrompt& other) const
   {
      return other.PressAction.IdenticalTo(PressAction) && other.HoldAction.IdenticalTo(HoldAction) && other.ErrorMessage.IdenticalTo(ErrorMessage) && InteractStatusTag == other.InteractStatusTag;
   }
};

USTRUCT(BlueprintType)
struct OSEINTERACTION_API FInteractEndContext
{
   GENERATED_BODY()

   FInteractEndContext() {}

public:
   UPROPERTY(BlueprintReadWrite)
   float PercentComplete = 0;

   UPROPERTY(BlueprintReadWrite)
   float Duration = 0;

   UPROPERTY(BlueprintReadWrite)
   bool bWasCanceled = false;

   bool IsComplete() const { return PercentComplete >= 1; }
   bool IsProbablyInstant() const;
};

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class OSEINTERACTION_API UInteractableInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 *
 */
class OSEINTERACTION_API IInteractableInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   bool IsInteractable(ACharacter* InteractingCharacter) const;

   // Get the prompt for the current interaction
   // This will be polled so should be cheap to compute, without non-amortized memory allocations
   // If that is not possible, changes to the struct to support that may be advisable
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   void GetInteractPrompt(ACharacter* InteractingCharacter, FInteractPrompt& outPrompt);

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   FInteractStartResult StartInteract(ACharacter* InteractingCharacter);

   // NOTE: Still prototype-ish may change
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   bool EndInteract(ACharacter* InteractingCharacter, const FInteractEndContext& context);

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
   void ShowHighlight(bool bShowHighlight);
};
