// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
//TAT
#include "Player/TATCharacter.h"

//UE
#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_WaitForIsNotSeen.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeoutDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIsNotSeenDelegate);

/**
 * 
 */
UCLASS()
class TAT_API UAbilityTask_WaitForIsNotSeen : public UAbilityTask
{
	GENERATED_BODY()
public:
   UAbilityTask_WaitForIsNotSeen(const FObjectInitializer& objectInitializer);

   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitForIsNotSeen* WaitForIsNotSeen(UGameplayAbility* owningAbility, ATATCharacter* inSeenCharacter, float inTimeoutTime);

   UPROPERTY(BlueprintAssignable)
   FOnTimeoutDelegate OnTimeout;

   UPROPERTY(BlueprintAssignable)
   FOnIsNotSeenDelegate OnNotSeen;

   virtual void TickTask(float deltaTime) override;

   UPROPERTY()
   ATATCharacter* seenCharacter;

   float timeoutTime = 0;
};
