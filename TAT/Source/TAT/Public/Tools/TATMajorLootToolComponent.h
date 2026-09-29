// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "Tools/TATToolComponent.h"

// ue5
#include "CoreMinimal.h"

#include "TATMajorLootToolComponent.generated.h"


USTRUCT()
struct TAT_API FTATMajorLootToolParams
{
   GENERATED_BODY()

public:
   FTATMajorLootToolParams() = default;
   FTATMajorLootToolParams(const FTATLootInstance& lootInstance, TObjectPtr<USkeletalMesh> mesh)
      : LootId(lootInstance.Identifier)
      , LootInstanceID(lootInstance.Id)
      , Mesh(mesh)
   {}

   // The type of loot the character is holding. Note that the inventory component owns the loot, not the tool,
   // which is why this is just an identifier and not an instance.
   UPROPERTY()
   FTATLootIdentifier LootId;

   UPROPERTY()
   FTATLootInstanceId LootInstanceID;

   UPROPERTY()
   TObjectPtr<USkeletalMesh> Mesh = nullptr;

   bool IsValid() const { return LootId.IsValid(); }
};

/**
 * 
 */
UCLASS()
class TAT_API UTATMajorLootToolComponent : public UTATToolComponent
{
	GENERATED_BODY()

public:
   UTATMajorLootToolComponent();

   void AuthoritySetLootParams(FTATMajorLootToolParams params) { _lootParams = params; }

   UFUNCTION(BlueprintPure, Category = "TAT Major Loot Tool Component")
   inline FTATLootIdentifier GetLootIdentifier() const { return _lootParams.LootId; }

   UFUNCTION(BlueprintPure, Category = "TAT Major Loot Tool Component")
   inline FTATLootInstanceId GetLootInstanceID() const { return _lootParams.LootInstanceID; }

   UFUNCTION(BlueprintPure, Category = "TAT Major Loot Tool Component")
   UTATLootInventoryComponent* GetOwningInventory() const;

   UFUNCTION(BlueprintPure)
   FTATLootInstance GetLootInstanceData() const;

   // from ToolComponent
   virtual bool IsReady() const override;

protected:
   // from UActorComponent
   virtual void OnRegister() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// IToolInterface (protected)
   virtual bool OnAddToToolSet_Implementation() override;
   virtual bool OnRemoveFromToolSet_Implementation() override;


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
   FTATMajorLootToolParams _lootParams;

   UPROPERTY(EditDefaultsOnly)
   bool _automaticallyEquipOnAddToToolset = true;
};
