// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilityTypes.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTypes)

FOSEGameplayEffectContext* FOSEGameplayEffectContext::GetFromHandle(FGameplayEffectContextHandle& handle)
{
   FGameplayEffectContext* baseEffectContext = handle.Get();
   if (baseEffectContext && ensure(baseEffectContext->GetScriptStruct()->IsChildOf(FOSEGameplayEffectContext::StaticStruct())))
   {
      return (FOSEGameplayEffectContext*)baseEffectContext;
   }
   return nullptr;
}

const FOSEGameplayEffectContext* FOSEGameplayEffectContext::GetFromHandle(const FGameplayEffectContextHandle& handle)
{
   const FGameplayEffectContext* baseEffectContext = handle.Get();
   if (baseEffectContext && ensure(baseEffectContext->GetScriptStruct()->IsChildOf(FOSEGameplayEffectContext::StaticStruct())))
   {
      return (const FOSEGameplayEffectContext*)baseEffectContext;
   }
   return nullptr;
}

AOSECharacterBase* FOSEGameplayEffectContext::GetInstigatorCharacter() const
{
   AActor* originalInstigator = GetOriginalInstigator();
   AOSECharacterBase* instigatorPawn = Cast<AOSECharacterBase>(originalInstigator);
   APlayerState* instigatorPlayerState = Cast<APlayerState>(originalInstigator);

   if (!instigatorPawn && instigatorPlayerState)
   {
      instigatorPawn = instigatorPlayerState->GetPawn<AOSECharacterBase>();
   }

   return instigatorPawn;
}

bool FOSEGameplayEffectContext::NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess)
{
   if (Super::NetSerialize(ar, map, outSuccess))
   {
      SupplementalEventTags.NetSerialize(ar, map, outSuccess);

      return outSuccess;
   }
   return false;
}

UScriptStruct* FOSEGameplayEffectContext::GetScriptStruct() const
{
   return FOSEGameplayEffectContext::StaticStruct();
}

FGameplayEffectContext* FOSEGameplayEffectContext::Duplicate() const
{
   FOSEGameplayEffectContext* newContext = new FOSEGameplayEffectContext();
   *newContext = *this;
   newContext->AddActors(Actors);
   if (GetHitResult())
   {
      // Does a deep copy of the hit result
      newContext->AddHitResult(*GetHitResult(), true);
   }
   return newContext;
}

FString FOSEGameplayEffectContext::ToString() const
{
   return Super::ToString();
}

AActor* FOSEGameplayEffectContext::GetOriginalInstigator() const
{
   // Override this if we have to start tracking the original character that started it in addition to the projectile/intermediate actor
   return Super::GetOriginalInstigator();
}

