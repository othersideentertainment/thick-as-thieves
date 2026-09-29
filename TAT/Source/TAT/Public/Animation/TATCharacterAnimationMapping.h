// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATCharacterAnimationMapping.generated.h"

class UAnimMontage;

/// Defines a set of animations for a particular character
/// This is intended for cases where we want to play a particular animation, e.g. a stagger, but may want variations
/// for different characters. In that case, we query the character for the given gameplay tag, and it looks it up in
/// its data asset
/// See also ITATCharacterAnimationInterface
UCLASS(BlueprintType)
class TAT_API UTATCharacterAnimationMappingAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Animation.Character", ForceInlineRow))
   TMap<FGameplayTag, UAnimMontage*> AnimationTagToMontage;

   UAnimMontage* LookupMontageByTag(const FGameplayTag& animationTag, bool warnOnNotFound = true) const;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   int32 ValidateForCharacter(const class ACharacter* character, FDataValidationContext& context) const;
#endif
};
