// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"

#include "AsyncTaskEffectStackChanged.generated.h"

struct FActiveGameplayEffect;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGameplayEffectStackChanged, FGameplayTag, effectGameplayTag, FActiveGameplayEffectHandle, handle, int32, newStackCount, int32, oldStackCount);

/**
 * Blueprint node to automatically register a listener for changes to gameplay effect stacks
 * Useful to use in UI.
 * NOTE: You must call EndTask() when the owner of this async task is going away (Destruct in UI, EndPlay in actors)
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskEffectStackChanged : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:

   UPROPERTY(BlueprintAssignable)
   FOnGameplayEffectStackChanged OnGameplayEffectStackChange;

   // listen for gameplay effect stack changes
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnGameplayEffectStackChangeListener")
   static UAsyncTaskEffectStackChanged* ListenForGameplayEffectStackChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTag effectGameplayTag);

   // from UBlueprintAsyncActionBase
   virtual void Activate() override;

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

private:
   void _OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* target, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle);
   void _OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& effectRemoved);
   void _OnGameplayEffectStackChanged(FActiveGameplayEffectHandle effectHandle, int32 newStackCount, int32 previousStackCount);

private:
   UPROPERTY()
   UAbilitySystemComponent* _abilitySystemComponent;
   FGameplayTag _effectGameplayTag;
};
