// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_Dodge.generated.h"

class UAnimMontage;

// Associates a dodge montage with a direction (relative to character-facing), to be selected when most closely matches the direction of a dodge
USTRUCT()
struct TAT_API FTATDodgeDirectionalAnimEntry
{
   GENERATED_BODY()

public:
   // Direction relative to player's facing direction
   UPROPERTY(EditDefaultsOnly)
   FVector DodgeDirection = FVector::ZeroVector;

   // Animation to play when dodging in this direction (or more so than the direction of other entries)
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAnimMontage> DodgeMontage;
};

UCLASS()
class TAT_API UTATGameplayAbility_Dodge : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UTATGameplayAbility_Dodge();
      
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:
   // Takes a dodge direction (relative to the player's facing-direction) and returns the anim montage of a dodge anim entry with most closely-aligned direction
   UFUNCTION(BlueprintPure)
   UAnimMontage* GetDodgeAnimMontage(const FVector& relativeDodgeDirection) const;

private:
   // Collection of direction -> animation entries which we query to find the best-fitting animation for the direction the player dodges
   UPROPERTY(EditDefaultsOnly, Category = "Dodge Settings")
   TArray<FTATDodgeDirectionalAnimEntry> _dodgeDirectionalAnimEntries;
};
