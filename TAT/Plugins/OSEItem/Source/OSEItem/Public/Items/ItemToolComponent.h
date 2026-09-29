// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Items/ToolComponent.h"

// ue4

#include "ItemToolComponent.generated.h"

class UItemInfo;

//---------------------------------------------------------------------------------------------------------------------------------
///  When the inventory grants an ItemToolComponent to the tool set, it stashes a back ptr to the item info that granted it.
//---------------------------------------------------------------------------------------------------------------------------------
UCLASS(ClassGroup = (Tools), Abstract, Blueprintable, BlueprintType
   , meta = (BlueprintSpawnableComponent, IsBlueprintBase = "true")
   , hideCategories = (Activation, Collision, ComponentReplication, Components, "Components|Activation", ComponentTick, Cooking, LOD, Object, Physics, Rendering, Utilities))
class OSEITEM_API UItemToolComponent : public UToolComponent
{
   GENERATED_BODY()

public:
   UItemToolComponent();

   // the inventory should call this when this tool is granted by an item
   void AuthoritySetGrantedByItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass) { check(GetOwner()->HasAuthority()); _grantedByItemInfoClass = itemInfoClass; }
   
   UFUNCTION(BlueprintPure, Category = "Tools|Items")
   TSubclassOf<UItemInfo> GetGrantedByItemInfoClass() const { return _grantedByItemInfoClass; }

   // from ToolComponent
   virtual bool IsReady() const override;

   // from UActorComponent
   virtual void OnRegister() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

#if WITH_EDITOR
   virtual bool CanEditChange(const FProperty* inProperty) const override;
#endif
protected:
   virtual bool _ShouldModifyVisualsOnRegister() const override final { return false; }

private:
   UFUNCTION()
   void _OnRep_GrantedByItemInfoClass();
   void _OnItemInfoAvailable();

private:
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_GrantedByItemInfoClass)
   TSubclassOf<UItemInfo> _grantedByItemInfoClass;

   /// copies Name, Description and Icon from the source item to the granting tool
   UPROPERTY(EditDefaultsOnly, Category="Item")
   bool _copyBasicItemInfoToToolInfo;

   // Whether to inject the item name into input prompts that have {ItemName}
   UPROPERTY(EditDefaultsOnly, Category = "Item")
   bool _injectItemNameInInputPrompts;
};
