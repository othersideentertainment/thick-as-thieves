// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Abilities/OSEGameplayAbility.h"
#include "OSEGameplayAbility_Character.generated.h"


/// Base character ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_Character : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_Character();

   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;
   using Super::CancelAbility; //< reimported so that the CancelAbility(ACharacter*) overload does not hide this method and cause warnings

   /// Event for when the ability activates, along with the character it's on
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Ability", DisplayName = "ActivateOnCharacter", meta = (ScriptName = "ActivateOnCharacter"))
   void BP_ActivateOnCharacter(ACharacter* character);

protected:


   ACharacter* GetCharacter(const FGameplayAbilityActorInfo* actorInfo) const;

   virtual bool CanActivateAbility(ACharacter* character) const { return false; };
   virtual void ActivateAbility(ACharacter* character) { };
   virtual void CancelAbility(ACharacter* character) { };
};
