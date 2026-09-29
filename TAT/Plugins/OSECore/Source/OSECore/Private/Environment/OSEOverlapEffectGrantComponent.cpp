// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/OSEOverlapEffectGrantComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEOverlapEffectGrantComponent)
DEFINE_LOG_CATEGORY_STATIC(LogOSEOverlapEffectGrantComponent, Log, All);

#if WITH_EDITOR
EDataValidationResult UOSEOverlapEffectGrantComponent::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult result = Super::IsDataValid(context);

   // Ensure effects are populated
   if (_effectsToGrantOnOverlap.IsEmpty())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("_effectsToGrantOnOverlap is empty!"))));
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

void UOSEOverlapEffectGrantComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      OnActorEnteredShape.AddDynamic(this, &UOSEOverlapEffectGrantComponent::_AuthorityOnActorEnterShape);
      OnActorExitedShape.AddDynamic(this, &UOSEOverlapEffectGrantComponent::_AuthorityOnActorExitShape);
   }
}

void UOSEOverlapEffectGrantComponent::_AuthorityOnActorEnterShape(AActor* actor)
{
   check(actor);
   check(GetOwner()->HasAuthority());
   
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      _AuthorityTryGrantEffects(actor, asc);
   }
}

void UOSEOverlapEffectGrantComponent::_AuthorityOnActorExitShape(AActor* actor)
{
   check(actor);
   check(GetOwner()->HasAuthority());

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      _AuthorityTryRemoveEffects(actor);
   }
}

void UOSEOverlapEffectGrantComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Cancel any active effects, in case owner is destroyed
   if (GetOwner()->HasAuthority())
   {
      _authorityOverlapGrantedEffectEntries.CancelAll();
   }

   Super::EndPlay(endPlayReason);
}

void UOSEOverlapEffectGrantComponent::_AuthorityTryGrantEffects(AActor* actor, UAbilitySystemComponent* asc)
{
   check(asc);
   check(GetOwner()->HasAuthority());
   if (_effectsToGrantOnOverlap.Num() == 0)
   {
      UE_LOG(LogOSEOverlapEffectGrantComponent, Error, TEXT("[%s] | _AuthorityTryGrantEffects() failed due to empty _effectsToGrantOnOverlap!"), *GetName());
      return;
   }


   FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
   effectContext.AddInstigator(GetOwner(), GetOwner());

   // Grant effects to overlapping actor's ASC
   TArray<FActiveGameplayEffectHandle> activeEffectHandles;
   for (const TSubclassOf<UGameplayEffect>& gameplayEffect : _effectsToGrantOnOverlap)
   {
      FGameplayEffectSpec effectSpec(gameplayEffect.GetDefaultObject(), effectContext);
      FActiveGameplayEffectHandle activeEffectHandle = asc->ApplyGameplayEffectSpecToSelf(effectSpec);
      if (activeEffectHandle.IsValid())
      {
         activeEffectHandles.Add(activeEffectHandle);
      }
   }

   // Store entry for removal upon overlap end
   _authorityOverlapGrantedEffectEntries.AddMultiple(actor, activeEffectHandles);

   UE_LOG(LogOSEOverlapEffectGrantComponent, Verbose, TEXT("[%s] | _AuthorityTryGrantEffects() granted effects to %s")
      , *GetName()
      , *actor->GetName());
}

void UOSEOverlapEffectGrantComponent::_AuthorityTryRemoveEffects(AActor* actor)
{
   check(actor);
   check(GetOwner()->HasAuthority());

   if (_authorityOverlapGrantedEffectEntries.CancelByActor(actor))
   {
      UE_LOG(LogOSEOverlapEffectGrantComponent, Verbose, TEXT("[%s] | _AuthorityTryRemoveEffects() removed effects from %s")
         , *GetName()
         , *actor->GetName());
   }
}
