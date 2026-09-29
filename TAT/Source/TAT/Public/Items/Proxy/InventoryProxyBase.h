// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Items/TATInventoryTypes.h"
#include "Upgrades/UpgradeCurrencies/TATUpgradeCurrencySource.h"

// ue4
#include "UObject/NoExportTypes.h"

#include "InventoryProxyBase.generated.h"

class UTATItemUIProxy;

/// A base class for a proxy view of a (subset of) an inventory
/// This could either be an actual inventory, or the one in the hub
UCLASS(Abstract, BlueprintType)
class TAT_API UInventoryProxyBase : public UObject, public ITATUpgradeCurrencySource
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintCallable)
   virtual const TArray< UTATItemUIProxy*>& GetBucket(EInventoryType bucket) const;

   UFUNCTION(BlueprintCallable)
   virtual bool CanMoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket) const { return false; }
   UFUNCTION(BlueprintCallable)
   virtual void MoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket) { }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBucketChanged, EInventoryType, bucket);
   UPROPERTY(BlueprintAssignable, Category = "Items")
   FOnBucketChanged OnBucketChanged;

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FTATUpgradeCurrencyChanged UpgradeCurrencyChanged;

   // ITATUpgradeCurrencySource
   virtual FTATUpgradeCurrencyChanged& GetUpgradeCurrencyChanged(FTATCharacterSaveId character) final { return UpgradeCurrencyChanged; }
};
