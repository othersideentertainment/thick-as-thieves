// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "AsyncTaskDamageTaken.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageTaken, float, previousHealth, float, currentHealth, float, maxHealth, float, damageTaken);

UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskDamageTaken : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
   // Listens for an attribute changing.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnDamageTakenListener")
   static UAsyncTaskDamageTaken* ListenForDamageTaken(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent);

public:
   UPROPERTY(BlueprintAssignable)
   FOnDamageTaken OnDamageTaken;
   
   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

protected:
   UPROPERTY()
   UAbilitySystemComponent* _abilitySystemComponent;

   FGameplayAttribute _attributeToListenFor;
   TArray<FGameplayAttribute> _attributesToListenFor;

   void _OnHealthAttributeChanged(const FOnAttributeChangeData& data);
};

DECLARE_LOG_CATEGORY_EXTERN(LogAsyncTaskDamageTaken, Log, All);
