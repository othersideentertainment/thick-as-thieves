// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"

#include "OSEAbilityTargetActor_BaseTrace.generated.h"

// intermediate subclass mostly just to override ShouldProduceTargetData with fix
// only for convenience, as there can be non-trace-subclass target actors as well
UCLASS(Abstract)
class OSECORE_API AOSEAbilityTargetActor_BaseTrace : public AGameplayAbilityTargetActor_Trace
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_BaseTrace();

   virtual bool ShouldProduceTargetData() const override;

   UFUNCTION(BlueprintPure, Category = "Targeting")
   UGameplayAbility* GetOwningAbility() const { return OwningAbility; }
};
