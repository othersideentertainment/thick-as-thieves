// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_ReceiveHoldAndDoublePress.generated.h"

class UAbilityTask_WaitInputWithTimeout;

// -------------------------------------------------------------------------------------------------------
/// Ability base class exposing hooks for single-press, double-press, and press-and-hold events
// -------------------------------------------------------------------------------------------------------
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_ReceiveHoldAndDoublePress : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:

   UTATGameplayAbility_ReceiveHoldAndDoublePress();

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

   UFUNCTION(BlueprintImplementableEvent, Category="Input")
   void HandleSinglePress();

   UFUNCTION(BlueprintImplementableEvent, Category = "Input")
   void HandleDoublePress();

   UFUNCTION(BlueprintImplementableEvent, Category = "Input")
   void HandlePressAndHold();

protected:
   /// Amount of time between 2 presses to be considered a double-press input, rather than 2 separate inputs
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float DoublePressThresholdSeconds = 0.35f;

   /// Amount of time for initial press to be considered a hold
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float PressHoldThresholdSeconds = 0.65f;

   /// If false, HandleDoublePress() will not be called
   UPROPERTY(EditDefaultsOnly)
   bool ReceiveDoublePress = true;

   /// If false, HandlePressAndHold() will not be called
   UPROPERTY(EditDefaultsOnly)
   bool ReceivePressAndHold = true;

private:
   UFUNCTION()
   void _OnInitialPressReleased(float timeHeld, bool bTimedOut);

   UFUNCTION()
   void _OnInitialPressTimeOut(float timeHeld, bool bTimedOut);

   UFUNCTION()
   void _OnDoublePress(float timeHeld, bool bTimedOut);

   UFUNCTION()
   void _OnDoublePressTimeOut(float timeHeld, bool bTimedOut);

private:
   /// Task for initial press release
   UPROPERTY(Transient)
   UAbilityTask_WaitInputWithTimeout* _waitInputReleaseTask = nullptr;

   /// Task for second press begin
   UPROPERTY(Transient)
   UAbilityTask_WaitInputWithTimeout* _waitInputPressTask = nullptr;
};
