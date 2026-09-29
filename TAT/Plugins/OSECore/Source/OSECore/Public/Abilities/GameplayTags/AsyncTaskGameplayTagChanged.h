// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "AsyncTaskGameplayTagChanged.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameplayTagChanged, FGameplayTag, tag, bool, added, int32, stackCount);

/**
 * Blueprint node to automatically register a listener for gameplay tag changes in an AbilitySystemComponent.
 * NOTE: You must call EndTask() when the owner of this async task is going away (Destruct in UI, EndPlay in actors)
 */
UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskGameplayTagChanged : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   UPROPERTY(BlueprintAssignable)
   FOnGameplayTagChanged OnGameplayTagChanged;
   
   // Listens for an attribute changing.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnGameplayTagChangedListener")
   static UAsyncTaskGameplayTagChanged* ListenForGameplayTagChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTag tag);

   // Listens for an attribute changing.
   // Version that takes in an array of Attributes. Check the Attribute output for which Attribute changed.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnGameplayTagsChangedListener")
   static UAsyncTaskGameplayTagChanged* ListenForGameplayTagsChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, const FGameplayTagContainer& tags);

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

protected:
   UPROPERTY()
   UAbilitySystemComponent* _abilitySystemComponent;

   struct FTagCallbackBindings
   {
      FGameplayTag Tag;
      FDelegateHandle Delegate;
   };
   TArray<FTagCallbackBindings> _tagsToListenFor;

   void _OnGameplayTagChanged(const FGameplayTag tag, int32 newCount);
};
