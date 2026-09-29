// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"

class UAbilitySystemComponent;
class UTATBreakableComponent;

// This didn't end up working, as it broke UHT's rtti
//#define BREAKABLE_ACTOR_INHERITANCE IAbilitySystemInterface, public IGameplayTagAssetInterface

#define BREAKABLE_ACTOR_DECLARATIONS() \
   virtual UAbilitySystemComponent* GetAbilitySystemComponent() const final override;\
   virtual void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const final override; \
   virtual bool HasMatchingGameplayTag(FGameplayTag tagToCheck) const final override; \
   virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const final override; \
   virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const final override; \
   virtual bool IsBroken() const override;
