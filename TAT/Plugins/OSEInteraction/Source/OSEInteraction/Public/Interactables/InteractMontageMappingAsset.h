// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "InteractMontageMappingAsset.generated.h"

class UAnimMontage;

// Mapping of gameplay tag to montage for interaction animations
// TODO: This is using TSoftObjectPtr, but it is unclear whether it is worth the
//       latency of async loading these animations on the fly. If the latency does
//       turn out to be noticeable, convert back to hard UAnimMontage pointers
UCLASS(BlueprintType)
class OSEINTERACTION_API UInteractMontageMappingAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   // Get the montage for this animation type, searches in parent tags if not found
   UFUNCTION(BlueprintCallable)
   UAnimMontage* FindInstantInteractMontage(FGameplayTag animationTypeTag) const;

   // Get the montage for this animation type, searches in parent tags if not found
   UFUNCTION(BlueprintCallable)
   TSoftObjectPtr<UAnimMontage> FindHoldInteractMontage(FGameplayTag animationTypeTag) const;
private:
   UPROPERTY(EditDefaultsOnly, meta = (Categories="InteractAnimation"))
   TMap<FGameplayTag, TObjectPtr<UAnimMontage>> _instantInteractMontages;

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAnimMontage> _fallbackInstantInteractMontage;

   UPROPERTY(EditDefaultsOnly)
   TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> _holdInteractMontages;
};
