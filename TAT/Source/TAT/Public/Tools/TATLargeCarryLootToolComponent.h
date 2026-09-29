// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "Tools/TATToolComponent.h"

// ue5
#include "CoreMinimal.h"

#include "TATLargeCarryLootToolComponent.generated.h"


USTRUCT()
struct TAT_API FTATLargeCarryLootToolParams
{
   GENERATED_BODY()

public:
   // The type of loot the character is holding.
   UPROPERTY()
   FTATLootIdentifier LootId;

   UPROPERTY()
   TObjectPtr<USkeletalMesh> Mesh = nullptr;

   bool IsValid() const { return LootId.IsValid(); }
};

// A tool that represents large-carried loot in-hand
//
// The visuals are parameterized and replicated, so that it can be re-used
//
// Very similar to the original version of the major loot tool circa mid 2023
UCLASS()
class TAT_API UTATLargeCarryLootToolComponent : public UTATToolComponent
{
	GENERATED_BODY()

public:
   UTATLargeCarryLootToolComponent();

   void AuthoritySetLootParams(const FTATLargeCarryLootToolParams& params) { _lootParams = params; }

   UFUNCTION(BlueprintPure, Category = "TAT LargeCarry Loot Tool Component")
   FTATLootIdentifier GetLootIdentifier() const { return _lootParams.LootId; }

   UFUNCTION(BlueprintPure, Category = "TAT LargeCarry Loot Tool Component")
   UTATLootInventoryComponent* GetOwningInventory() const;

   // from ToolComponent
   virtual bool IsReady() const override;

protected:
   // from UActorComponent
   virtual void OnRegister() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// IToolInterface (protected)
   virtual bool OnAddToToolSet_Implementation() override;
   virtual bool OnUnequip_Implementation() override;


#if WITH_EDITOR
   virtual bool CanEditChange(const FProperty* inProperty) const override;
#endif

private:
   bool _HasAuthority() const;

   UFUNCTION()
   void _OnRep_LootParams();
   void _OnLootParamsAvailable();
   void _InjectIntoInputPrompts(const FTATLootInfo& lootInfo);

protected:
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_LootParams)
   FTATLargeCarryLootToolParams _lootParams;

   UPROPERTY(EditDefaultsOnly)
   bool _automaticallyEquipOnAddToToolset = true;
};
