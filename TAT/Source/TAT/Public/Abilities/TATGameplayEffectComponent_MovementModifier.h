// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat

// ue
#include "GameplayEffectComponent.h"
#include "GameplayEffectTypes.h"

#include "TATGameplayEffectComponent_MovementModifier.generated.h"

class UPaperSprite;

UENUM(BlueprintType)
enum class ETATGameplayEffectComponent_MovementModifierOp : uint8
{
   ClampMax,
};

UENUM(BlueprintType)
enum class ETATGameplayEffectComponent_MovementModifierStat : uint8
{
   None = 0,
   Friction,
   BrakingDeceleration,

   // WalkSpeed,
   // SprintSpeed,
   // CrouchSpeed,
   // Acceleration,
};

USTRUCT(BlueprintType)
struct TAT_API FTATGameplayEffectComponent_MovementModifierItem
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   ETATGameplayEffectComponent_MovementModifierStat Stat = ETATGameplayEffectComponent_MovementModifierStat::None;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   ETATGameplayEffectComponent_MovementModifierOp Operator = ETATGameplayEffectComponent_MovementModifierOp::ClampMax;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float Value = 0.0f;
};

UCLASS(DisplayName="[TAT] Movement Modifier (Experimental)")
class TAT_API UTATGameplayEffectComponent_MovementModifier : public UGameplayEffectComponent
{
   GENERATED_BODY()

public:
   UTATGameplayEffectComponent_MovementModifier();

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Modifier")
   TArray<FTATGameplayEffectComponent_MovementModifierItem> Modifiers;

   // From UGameplayEffectComponent
   virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& activeGEContainer, FActiveGameplayEffect& activeGE) const override;

private:
   void _OnActiveGameplayEffectRemoved(const FGameplayEffectRemovalInfo& removalInfo, FActiveGameplayEffectsContainer* activeGEContainer) const;

   static ACharacter* _GetOwnerCharacter(const UAbilitySystemComponent* asc);
};
