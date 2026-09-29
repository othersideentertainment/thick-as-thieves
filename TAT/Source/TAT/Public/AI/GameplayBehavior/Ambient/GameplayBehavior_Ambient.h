// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayBehavior.h"
#include "GameplayBehaviorConfig_Ambient.h"
#include "Abilities/GameplayAbility.h"

#include "GameplayBehavior_Ambient.generated.h"

/**
 * 
 */
UCLASS()
class TAT_API UGameplayBehavior_Ambient : public UGameplayBehavior
{
   GENERATED_BODY()

public:
   virtual bool Trigger(AActor& Avatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner) override;
   virtual void EndBehavior(AActor& Avatar, const bool bInterrupted) override;

protected:
   static TSubclassOf<UGameplayAbility> GetValidAbilityFromConfig(const UGameplayBehaviorConfig_Ambient* configAmbient, const UAbilitySystemComponent& abilitySystemComponent);

   TSubclassOf<UGameplayAbility> _abilityToTrack;
   FGameplayAbilitySpecHandle _trackedAbilitySpecHandle;
   void _OnAbilityCompleted(UGameplayAbility* gameplayAbility, TWeakObjectPtr<AActor> avatar);

};
