// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"
#include "Utl/OSEShapeCollisionTrackerComponent.h"

#include "OSEOverlapEffectGrantComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayEffect;

/// Grants a collection of effects to overlapping actors
UCLASS(meta=(BlueprintSpawnableComponent))
class OSECORE_API UOSEOverlapEffectGrantComponent : public UOSEShapeCollisionTrackerComponent
{
   GENERATED_BODY()
   
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

protected:
   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);

   UFUNCTION()
   void _AuthorityOnActorExitShape(AActor* actor);

   void _AuthorityTryGrantEffects(AActor* actor, UAbilitySystemComponent* asc);
   void _AuthorityTryRemoveEffects(AActor* actor);

private:
   /// Effects to grant to overlapping character on authority
   UPROPERTY(EditDefaultsOnly, Category = "Overlap")
   TArray<TSubclassOf<UGameplayEffect>> _effectsToGrantOnOverlap;

   /// Granted-effects bookkeeping for removal when overlap ends
   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _authorityOverlapGrantedEffectEntries;
};
