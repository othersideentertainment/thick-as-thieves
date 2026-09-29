// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayEffectCustomApplicationRequirement.h"

#include "OSECustomEffectRequirements.generated.h"


// Keeping the names short-ish, as the engine property does not use display names

UCLASS(Abstract)
class OSECORE_API UOSEEffectRequirement_AvatarClass : public UGameplayEffectCustomApplicationRequirement
{
   GENERATED_BODY()
public:
   virtual bool CanApplyGameplayEffect_Implementation(const UGameplayEffect* gameplayEffect, const FGameplayEffectSpec& spec, UAbilitySystemComponent* asc) const override;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<AActor> RequiredAvatarClass;
};

UCLASS(meta = (DisplayName = "HasCharacterAvatar"))
class OSECORE_API UOSEEffectRequirement_CharacterAvatar : public UOSEEffectRequirement_AvatarClass
{
   GENERATED_BODY()
public:
   UOSEEffectRequirement_CharacterAvatar();
};
