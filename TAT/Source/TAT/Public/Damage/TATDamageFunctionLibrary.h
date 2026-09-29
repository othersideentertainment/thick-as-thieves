// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATDamageFunctionLibrary.generated.h"

struct FTATDamageWithType;

UCLASS()
class TAT_API UTATDamageFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

   static float CalculateOutgoingDamageForSource(const FTATDamageWithType& damage, const AActor* actor);

   // Tries to deal damage from the (optional) source actor to the target actor
   UFUNCTION(BlueprintCallable, Category=Damage, meta=(AutoCreateRefTerm="origin,hitResult"))
   static void DealDamage(
      AActor* sourceActor,
      AActor* targetActor, 
      FTATDamageWithType damage, 
      const FVector & origin,
      const FHitResult & hitResult,
      UPARAM(Meta = (Category = "DamageContext")) FGameplayTag damageContext = FGameplayTag());

   UFUNCTION(BlueprintPure, Category = "Damage|Conversions", meta = (DisplayName = "To DamageWithType", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast))
   static FTATDamageWithType Conv_TATScalableDamageWithTypeToTATDamageWithType(const FTATScalableDamageWithType& scalableDamage);

   UFUNCTION(BlueprintPure, Category = "Damage|Conversions")
   static FTATDamageWithType EvaluateDamageAtLevel(const FTATScalableDamageWithType& scalableDamage, float level);

   UFUNCTION(BlueprintPure, Category = "Damage")
   static FGameplayTag ExtractDamageTypeFromContainer(const FGameplayTagContainer& tagContainer);

   UFUNCTION(BlueprintPure, Category = "Damage")
   static FGameplayTag ExtractDamageContextFromContainer(const FGameplayTagContainer& tagContainer);

   UFUNCTION(BlueprintPure, Category = "Damage")
   static bool IsComponentInvalidDamageTarget(const UActorComponent* component);

   UFUNCTION(BlueprintPure, Category = "Damage")
   static bool IsActorPossiblyDamageable(const AActor* actor);

   /// A target-based cooldown to rate-limit damage from noisy collision/proximity with environmental damage sources
   ///
   /// Relatively contained surface area in case it needs to be replaced.
   ///
   /// Prediction:
   ///    The pseudo-prediction predictively the adds tag from the cooldown effect, and removes it after the duration.
   ///    It does not use prediction keys, or attempt to rollback. The use-case is to allow clients to predictively
   ///    play cosmetic impact FX that roughly align to when the server will produce damage, so we don't care that
   ///    much.
   /// 
   ///    The main edge case is that there is a window where the removal of the actual replicated cooldown (which also
   ///    still happens) hasn't reached the client yet, so the server allows it, but the client does not play
   ///    anticipatory FX. This is probably acceptable if rare, and certainly better than the reverse.
   /// 
   UFUNCTION(BlueprintCallable, Category = "Damage|Cooldown", meta = (ExpandBoolAsExecs="ReturnValue"))
   static bool TryApplyEnvironmentalDamageCooldown(AActor* target, TSubclassOf<UGameplayEffect> cooldownEffect, bool allowPseudoPrediction = true);
};
