// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "AsyncTaskAttributeChanged.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnAttributeChanged, FGameplayAttribute, attribute, float, newValue, float, oldValue, float, delta, const FGameplayTagContainer&, effectAssetTags);

/**
 * Blueprint node to automatically register a listener for all attribute changes in an AbilitySystemComponent.
 * NOTE: You must call EndTask() when the owner of this async task is going away (Destruct in UI, EndPlay in actors)
 */
UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskAttributeChanged : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   UPROPERTY(BlueprintAssignable)
   FOnAttributeChanged OnAttributeChanged;
   
   // Listens for an attribute changing.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnAttributeChangeListener")
   static UAsyncTaskAttributeChanged* ListenForAttributeChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayAttribute attribute);

   // Listens for an attribute changing.
   // Version that takes in an array of Attributes. Check the Attribute output for which Attribute changed.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnAttributesChangeListener")
   static UAsyncTaskAttributeChanged* ListenForAttributesChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, TArray<FGameplayAttribute> attributes);

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

protected:
   UPROPERTY()
   UAbilitySystemComponent* _abilitySystemComponent;

   FGameplayAttribute _attributeToListenFor;
   TArray<FGameplayAttribute> _attributesToListenFor;

   void _OnAttributeChanged(const FOnAttributeChangeData& data);
};

DECLARE_LOG_CATEGORY_EXTERN(LogAsyncTaskAttributeChanged, Log, All);
