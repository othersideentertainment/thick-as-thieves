// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "GameplayCueManager.h"

#include "OSEGameplayCueManager.generated.h"


UCLASS()
class OSECORE_API UOSEGameplayCueManager : public UGameplayCueManager
{
   GENERATED_BODY()

protected:
   virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;
	
   virtual void InvokeGameplayCueExecuted_FromSpec(UAbilitySystemComponent* owningComponent, const FGameplayEffectSpec& spec, FPredictionKey predictionKey) override;
};
