// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemActorRuntimeStateAbilities.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Abilities/Effects/OSEGameplayEffectSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemActorRuntimeStateAbilities)

// ue4

AItemActorRuntimeStateAbilities::AItemActorRuntimeStateAbilities()
   : Super()
{
   AbilitySystemComponent = CreateDefaultSubobject<UOSEAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
   BaseAttributeSet = CreateDefaultSubobject<UAttributeBaseSet>(TEXT("BaseAttributeSet"));
}

void AItemActorRuntimeStateAbilities::BeginPlay()
{
   Super::BeginPlay();
   _InitAbilities();
}

UAbilitySystemComponent* AItemActorRuntimeStateAbilities::GetAbilitySystemComponent() const
{
   return AbilitySystemComponent;
}

TScriptInterface<IAttributeBaseInterface> AItemActorRuntimeStateAbilities::GetBaseAttributeInterface() const
{
   return BaseAttributeSet;
}

void AItemActorRuntimeStateAbilities::_InitAbilities()
{
   // Apply initial effects
   for (const UOSEGameplayEffectSet* effectSet : InitialEffectSets)
   {
      if (effectSet)
      {
         effectSet->ApplyEffects(this);
      }
   }
}

