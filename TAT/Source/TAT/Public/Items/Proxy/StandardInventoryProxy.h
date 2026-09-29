// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "Items/Proxy/InventoryProxyBase.h"

#include "StandardInventoryProxy.generated.h"

class UTATItemInventoryComponent;
struct FTATInventorySlot;

USTRUCT()
struct FStandardInventoryProxyBucket
{
   GENERATED_BODY()

public:
   UPROPERTY()
   TMap<FInventoryStackId, UTATItemUIProxy*> Cache;

   UPROPERTY()
   TArray<UTATItemUIProxy*> Items;
};

// An inventory proxy what wraps a standard inventory component
UCLASS()
class TAT_API UStandardInventoryProxy : public UInventoryProxyBase
{
   GENERATED_BODY()

   UStandardInventoryProxy();

   UFUNCTION(BlueprintCallable)
   void Init(UTATItemInventoryComponent* inventory, TSubclassOf<UTATItemUIProxy> proxyClass);

   virtual bool CanMoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket) const override;
   virtual void MoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket) override;

   virtual const TArray< UTATItemUIProxy*>& GetBucket(EInventoryType bucket) const override;

   // ITATUpgradeCurrencySource
   virtual int32 GetUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag) const override;

private:
   void _UpdateBucket(EInventoryType bucketType, const TArray<FTATInventorySlot>& slot);
   UFUNCTION()
   void _OnBackpackChanged();
   UFUNCTION()
   void _OnQuestItemsChanged();
   UFUNCTION()
   void _OnToolbeltChanged();
   UFUNCTION()
   void _OnUpgradeCurrencyChanged(FGameplayTag currencyTag, int32 amount);

private:
   UPROPERTY()
   UTATItemInventoryComponent* _inventory;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UTATItemUIProxy> _proxyClass;

   UPROPERTY()
   TMap<EInventoryType, FStandardInventoryProxyBucket> _standardBuckets;
};
