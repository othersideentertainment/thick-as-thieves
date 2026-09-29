// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "OSEAbilityInputBinds.h"
#include "OSEGameplayAbilitySet.generated.h"


/// A set of abilities that can be granted at once
UCLASS(ClassGroup = (Ability))
class OSECORE_API UOSEGameplayAbilitySet : public UDataAsset
{
   GENERATED_BODY()

public:

   virtual TArray<FGameplayAbilitySpecHandle> GiveAbilities(IAbilitySystemInterface* abilitySystemInterface) const;
   virtual TArray<FGameplayAbilitySpecHandle> GiveAbilities(UAbilitySystemComponent* abilitySystemComponent, UObject* ownerObj) const;

   bool IsReady(const UAbilitySystemComponent* abilitySystemComponent, const UObject* ownerObj) const;

   // mostly for validation
   // preferring this over an accessor just in case we added nested sets later
   void ForEachAbility(TFunctionRef<void(const FOSEAbilityBindInfo&)> handler) const;

protected:

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", Meta = (TitleProperty = AbilityClass))
   TArray<FOSEAbilityBindInfo> Abilities;
};
