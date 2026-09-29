// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATCharacterAnimationInterface.generated.h"

class UAnimMontage;

/// Allows for querying characters for which specific montage to play based on a semantic tag
/// See also UTATCharacterAnimationMappingAsset
UINTERFACE(BlueprintType, MinimalAPI, Category = "Animation", meta = (CannotImplementInterfaceInBlueprint))
class UTATCharacterAnimationInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATCharacterAnimationInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Animation")
   virtual UAnimMontage* GetCharacterMontage(UPARAM(Meta = (Categories = "Animation.Character")) const FGameplayTag& animationTag) const = 0;
};
