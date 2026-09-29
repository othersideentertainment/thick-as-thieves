// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "AsyncTaskCooldownChanged.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCooldownChanged, FGameplayTag, cooldownTag, float, endTime, float, duration, bool, isNewlyAddedEffect);

/**
 * Blueprint node to automatically register a listener for changes (Begin and End) to an array of Cooldown tags.
 * Useful to use in UI.
 * NOTE: You must call EndTask() when the owner of this async task is going away (Destruct in UI, EndPlay in actors)
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskCooldownChanged : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   UPROPERTY(BlueprintAssignable)
   FOnCooldownChanged OnCooldownBegin;

   UPROPERTY(BlueprintAssignable)
   FOnCooldownChanged OnCooldownEnd;

   // Listens for changes (Begin and End) to cooldown GameplayEffects based on the cooldown tag.
   // If using ServerCooldown, TimeRemaining and Duration will return -1 to signal local predicted cooldown has begun.
   // If checkInitialCooldown is true, then OnCooldownBegin will fire if there is an existing cooldown when we start listening
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnCooldownChangedListener")
   static UAsyncTaskCooldownChanged* ListenForCooldownChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTagContainer cooldownTags, bool checkInitialCooldown);

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

   virtual void Activate() override;

protected:
   UPROPERTY()
   UAbilitySystemComponent* _abilitySystemComponent;

   bool _checkInitialCooldown = false;

   FGameplayTagContainer _cooldownTags;

   // Subset of _cooldownTags associated with a currently-active cooldown effect
   TArray<FGameplayTag> _activeCooldownEffectTags;

   virtual void _OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* target, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle);
   virtual void _OnCooldownTagChanged(const FGameplayTag cooldownTag, int32 newCount);

   bool _GetCooldownRemainingForTag(const FGameplayTagContainer& cooldownTags, float& endTime, float& duration);

   void _BroadcastCooldownBeginForTag(FGameplayTag cooldownTag, float endTime, float duration);
};
