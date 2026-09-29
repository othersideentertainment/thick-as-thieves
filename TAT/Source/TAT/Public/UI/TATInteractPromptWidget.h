// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"
#include "Interactables/InteractableInterface.h"

// tat
#include "TATUserWidget.h"

#include "TATInteractPromptWidget.generated.h"

class ATATCharacter;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct TAT_API FAbilityExtraContextualInputRequest
{
GENERATED_BODY()
public:
   TWeakObjectPtr<UGameplayAbility> Ability;

   UPROPERTY(Transient, BlueprintReadOnly)
   FText InputText;

   UPROPERTY(Transient, BlueprintReadOnly)
   EAbilityInputType InputType = EAbilityInputType::None;

   UPROPERTY(Transient, BlueprintReadOnly)
   bool IsPress = true;

   UPROPERTY(Transient, BlueprintReadOnly)
   FGameplayTag ActionTag;
};

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATInteractPromptWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   UFUNCTION(BlueprintCallable, Category = "TAT Interact Prompt Widget")
   void RequestAbilityInteractPrompt(UGameplayAbility* ability, bool isPress);
   UFUNCTION(BlueprintCallable, Category = "TAT Interact Prompt Widget")
   void ClearRequestedInteractPrompt(UGameplayAbility* ability, bool isPress);

   UFUNCTION(BlueprintCallable, Category = "TAT Interact Prompt Widget")
   void RequestAbilityExtraContextualInput(UGameplayAbility* ability, EAbilityInputType inputType, bool isPress);
   UFUNCTION(BlueprintCallable, Category = "TAT Interact Prompt Widget")
   void ClearAbilityExtraContextualInput(UGameplayAbility* ability, EAbilityInputType inputType, bool isPress);

protected:
   // from UTATUserWidget
   virtual void _OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character) override;
   
   UFUNCTION(BlueprintNativeEvent, Category = "TAT Interact Prompt Widget")
   void _OnPromptChanged(const FInteractPrompt& prompt);
   void _OnPromptChanged_Implementation(const FInteractPrompt& prompt) { }

   UFUNCTION(BlueprintNativeEvent, Category = "TAT Interact Prompt Widget")
   void _OnExtraContextualInputPromptsChanged(const TArray<FAbilityExtraContextualInputRequest>& extraContextualInputs);
   void _OnExtraContextualInputPromptsChanged_Implementation(const TArray<FAbilityExtraContextualInputRequest>& extraContextualInputs) { }

private:
   UFUNCTION()
   void _OnInteractTargeterPromptChanged(const FInteractPrompt& prompt);
   void _CleanupRequestedPrompts();
   void _OnPromptChanged();
   void _OnExtraContextualInputPromptsChanged();

private:
   struct FAbilityPromptRequest
   {
      TWeakObjectPtr<UGameplayAbility> Ability;
      bool IsPress = false;
      
      FORCEINLINE bool operator==(const FAbilityPromptRequest& other) const
      {
         return Ability == other.Ability && IsPress == other.IsPress;
      }
   };
   FInteractPrompt _targeterPrompt;
   TArray<FAbilityPromptRequest> _requestedAbilityPrompts;

   TArray<FAbilityExtraContextualInputRequest> _requestedAbilityExtraContextualInputs;
};
