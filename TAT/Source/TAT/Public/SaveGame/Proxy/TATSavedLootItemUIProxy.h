// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "SaveGame/TATSavedLoot.h"

// ue
#include "GameplayTagContainer.h"
#include "Misc/DateTime.h"
#include "UObject/SoftObjectPtr.h"

#include "TATSavedLootItemUIProxy.generated.h"

class UTATThievesDenQuestSubsystem;

USTRUCT(BlueprintType)
struct TAT_API FTATSavedLootItemUIProxyState
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   int32 Quantity = 0;

   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   bool HasDateAcquired = false;

   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   FDateTime DateAcquired;
};

UCLASS(Blueprintable)
class TAT_API UTATSavedLootItemUIProxy : public UObject
{
   GENERATED_BODY()

   DECLARE_MULTICAST_DELEGATE(FOnDataChangedEvent);

public:
   FORCEINLINE const FTATLootIdentifier& GetID() const { return _id; }

   UFUNCTION(BlueprintCallable)
   const FTATLootMetadataBP& GetMetadata() const { return _metadata; }

   UFUNCTION(BlueprintCallable)
   const FTATSavedLootItemUIProxyState& GetState() const { return _state; }

   // Specifies whether the loot item this proxy is linked with can be modified (i.e. sold)
   UFUNCTION(BlueprintCallable)
   bool IsModifiable() const { return _isModifiable; }

   UFUNCTION(BlueprintCallable)
   void SetSellQuantity(int32 quantity);

   UFUNCTION(BlueprintCallable)
   int32 GetSellQuantity() const { return _sellQuantity; }

   UFUNCTION(BlueprintCallable)
   void ClearSellQuantity() { SetSellQuantity(0); }

   void BuildFromLootStack(const FTATSavedLootStack& stack, const FTATLootInfo& info);
   void Reset();

   void UpdateLootStackState(const FTATSavedLootStack& stack);

   void RefreshIsModifiable(const UTATThievesDenQuestSubsystem* questSubsystem);

   FOnDataChangedEvent OnDataChanged;

private:
   UPROPERTY()
   FTATLootIdentifier _id;

   UPROPERTY()
   FTATLootMetadataBP _metadata;

   UPROPERTY()
   FTATSavedLootItemUIProxyState _state;

   // Specifies whether the loot item this proxy is linked with can be modified (i.e. sold)
   bool _isModifiable = true;

   int32 _sellQuantity = 0;
		
};
