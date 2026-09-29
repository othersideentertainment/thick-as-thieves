// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Abilities/Attributes/TATPropHealthAttributeSet.h"
#include "Breakables/TATBreakableActorFwd.h"
#include "Breakables/TATBreakableComponent.h"
#include "Breakables/TATPropAbilitySystemComponent.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

#define BREAKABLE_ACTOR_IMPLS(ClassName, AscName, BreakableName) \
   UAbilitySystemComponent* ClassName::GetAbilitySystemComponent() const { return AscName; }\
   void ClassName::GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const \
   {\
      if(AscName)\
      {\
         AscName->GetOwnedGameplayTags(tagContainer);\
      }\
   } \
   bool ClassName::HasMatchingGameplayTag(FGameplayTag tagToCheck) const \
   { return AscName ? AscName->HasMatchingGameplayTag(tagToCheck) : false; } \
   bool ClassName::HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const \
   { return AscName ? AscName->HasAllMatchingGameplayTags(tagContainer) : false; } \
   bool ClassName::HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const \
   { return AscName ? AscName->HasAnyMatchingGameplayTags(tagContainer) : false; } \
   bool ClassName::IsBroken() const \
   {   return BreakableName && BreakableName->IsBroken(); }


static inline UAbilitySystemComponent* CreateAbilitySystemForBreakables(AActor* owner)
{
   UAbilitySystemComponent* asc = owner->CreateDefaultSubobject<UTATPropAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
   if (asc)
   {
      asc->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

      // This will clobber the attribute sets, so doing this allows the breakable component to do this instead
      // TODO: This also mean that ::UninitializeComponent is also not called, which does some mostly-irrelevant cleanup
      //       Possibly use ASC subclass instead that removes that logic from InitializeComponent (or fall back on the actor just explicitly calling a method)
      asc->bWantsInitializeComponent = false;
   }
   return asc;
}
