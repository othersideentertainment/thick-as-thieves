// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTargetActor.h"

#include "OSEAbilityTargetActor_AOE.generated.h"

class UGameplayAbility;

UCLASS(Abstract, Blueprintable, notplaceable)
class OSECORE_API AOSEAbilityTargetActor_AOE : public AGameplayAbilityTargetActor
{
   GENERATED_UCLASS_BODY()

public:

   virtual void StartTargeting(UGameplayAbility* Ability) override;
   virtual bool ShouldProduceTargetData() const override;
   
   virtual void ConfirmTargetingAndContinue() override;

   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
   virtual void Tick(float DeltaSeconds) override;
protected:
   virtual void PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview = false);
   void UpdateReticles(const TArray<TWeakObjectPtr<AActor>>& actors);
   FGameplayAbilityTargetDataHandle MakeTargetData(const TArray<TWeakObjectPtr<AActor>>& actors) const;

private:
   void _DestroyWorldReticles();

private:
   TArray<TWeakObjectPtr<AGameplayAbilityWorldReticle>> _reticleActors;

};
