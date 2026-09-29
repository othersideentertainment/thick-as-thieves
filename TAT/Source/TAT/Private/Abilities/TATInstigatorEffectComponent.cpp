// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TATInstigatorEffectComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInstigatorEffectComponent)

// Sets default values for this component's properties
UTATInstigatorEffectComponent::UTATInstigatorEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UTATInstigatorEffectComponent::BeginPlay()
{
	Super::BeginPlay();

   AActor* owner = GetOwner();
   if (owner->HasAuthority())
   {
      UAbilitySystemComponent* instigatorAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(owner->GetInstigator());
      if (instigatorAsc && _instigatorEffect.Get())
      {
         FGameplayEffectContextHandle context = instigatorAsc->MakeEffectContext();
         context.AddInstigator(owner, owner);

         FGameplayEffectSpec spec(_instigatorEffect.GetDefaultObject(), context, 0);

         // copy actor lifespan to effect if applicable, mostly for display purposes
         if (spec.Def->DurationPolicy == EGameplayEffectDurationType::HasDuration && owner->GetLifeSpan() > 0)
         {
            spec.SetDuration(owner->GetLifeSpan(), true);
         }

         _authorityInstigatorEffect = instigatorAsc->ApplyGameplayEffectSpecToSelf(spec);
      }

   }
}

void UTATInstigatorEffectComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (endPlayReason == EEndPlayReason::Destroyed)
   {
      if (UAbilitySystemComponent* asc = _authorityInstigatorEffect.GetOwningAbilitySystemComponent())
      {
         asc->RemoveActiveGameplayEffect(_authorityInstigatorEffect);
      }
   }

   Super::EndPlay(endPlayReason);
}


