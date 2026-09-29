// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Thiefsign/TATThiefsignSettings.h"
#include "Thiefsign/TATThiefsignTypes.h"

// ue
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_Thiefsign.generated.h"

class UAnimMontage;

UCLASS(Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_Thiefsign : public UOSEGameplayAbility
{
	GENERATED_BODY()

public:
   UTATGameplayAbility_Thiefsign();

   // from UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

protected:
   // Retrieves the animation associated with this ability instance's type
   UFUNCTION(BlueprintCallable)
   UAnimMontage* GetAnimation(const FGameplayEventData& triggerEventData, ETATThiefsignType type) const;

   // Generates a FGameplayCueParameters struct based on this ability instance's type.
   // Explicitly disable BlueprintPure (which Unreal auto-activates due to const-ness) to prevent multiple calls when out params are referenced multiple times.
   UFUNCTION(BlueprintCallable, BlueprintPure = false)
   void DetermineCueParams(ETATThiefsignType type, const FGameplayTag& thiefsignIdentifier, FGameplayTag& cueTag, FGameplayCueParameters& cueParams, bool& success) const;

private:

   // Contains animation/skeletal data for the activating-character type
   UPROPERTY(Transient)
   FTATThiefsignCharacterConfig _characterConfig;
};
