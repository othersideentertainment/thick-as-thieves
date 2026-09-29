// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Attributes/AttributeBaseSystemInterface.h"
#include "ItemActorRuntimeState.h"

// ue4
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"

#include "ItemActorRuntimeStateAbilities.generated.h"

class UAttributeBaseSet;
class UOSEAbilitySystemComponent;
class UOSEGameplayEffectSet;

UCLASS(Abstract)
class OSEITEM_API AItemActorRuntimeStateAbilities
   : public AItemActorRuntimeState
   , public IAbilitySystemInterface
   , public IAttributeBaseSystemInterface
{
   GENERATED_BODY()

public:
   AItemActorRuntimeStateAbilities();

   // from AActor
   virtual void BeginPlay();

   // from IAbilitySystemInterface
   virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

   // from IAttributeBaseSystemInterface
   virtual TScriptInterface<IAttributeBaseInterface> GetBaseAttributeInterface() const override;

protected:
   UPROPERTY(Transient)
   UOSEAbilitySystemComponent* AbilitySystemComponent = nullptr;
   
   UPROPERTY(Transient)
   UAttributeBaseSet* BaseAttributeSet = nullptr;

   /// Initial gameplay effects
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TArray<UOSEGameplayEffectSet*> InitialEffectSets;

private:
   void _InitAbilities();
};
