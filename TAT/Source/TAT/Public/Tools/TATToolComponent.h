// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolTypes.h"

// ose
#include "Items/ToolComponent.h"
#include "OSECoreCheats.h" // To determine if cheats are enabled or not

// ue
#include "ActiveGameplayEffectHandle.h"

#include "TATToolComponent.generated.h"

class UGameplayEffect;
class UOSEGameplayEffectSet;
class UTATHUDToolWidget;

UENUM(BlueprintType)
enum class EAmmoRefillType : uint8
{
   /// Ammo can be refilled with loot crates, in safe rooms, etc.
   CanBeRefilledExternally = 0,
   /// Ammo can only be refilled with internal tool actions (e.g. Cranking a Zipwire)
   CanOnlyBeRefilledInternally = 1
};

UCLASS(Abstract)
class TAT_API UTATToolComponent : public UToolComponent
{
   GENERATED_BODY()
public:

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void PostLoad() override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   virtual bool CanEditChange(const FProperty* inProperty) const override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif

   // From UActorComponent
   virtual void BeginPlay() override;

   /// Effect to apply while the tool is equipped
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Equipped Effects")
   TSubclassOf<UGameplayEffect> EquippedEffectClass;

   /// If specified, only apply the equipped effect if the character has this upgrade tag
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Equipped Effects")
   FGameplayTag EquippedEffectUpgradeTag;

   /// Minimum level for the equipped effect upgrade tag requirement
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Equipped Effects", Meta = (UIMin = 1, ClampMin = 1))
   int32 EquippedEffectUpgradeLevel = 1;

   // Tag for anim set type that will be requested when the tool is equipped
   UPROPERTY(EditDefaultsOnly, Category = "Tools", meta=(Categories="AnimSet"))
   FGameplayTag AnimSetTag;

   // Effect set applied while equpped
   // (has its own upgrade properties, so does not use EquippedEffectUpgradeTag)
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Equipped Effects")
   TObjectPtr<UOSEGameplayEffectSet> EquippedEffectSet;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Ammo")
   ETATToolAmmoType AmmoType = ETATToolAmmoType::Finite;

   // Whether the tool automatically removes itself when out of ammo (not unequip, fully remove)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "AmmoType == ETATToolAmmoType::Finite", EditConditionHides))
   bool RemoveWhenOutOfAmmo = false;

   // For use with `TATAbilityCost_DefinedByToolAmmo` class.  Predefines the amount of ammo used for tool usages: Primary, Secondary, BoobyTrap
   // Intended to be the single source of truth used for tools whose functionality changes depending on ammo cost
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Ammo", meta = (Categories = "Tool.Usage", ForceInlineRow))
   TMap<FGameplayTag, int32> AmmoCostsByUsageTag;

   /// If true, then ammo will be refilled in safe rooms, etc.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Ammo", Meta = (EditCondition = "AmmoType == ETATToolAmmoType::Finite", EditConditionHides))
   EAmmoRefillType AmmoRefillType = EAmmoRefillType::CanBeRefilledExternally;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Metadata", Meta = (RowType="/Script/TAT.TATGearMetadataTableRow"))
   FDataTableRowHandle MetadataTableRow;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|HUD")
   TSoftClassPtr<UTATHUDToolWidget> HUDWidgetWhenEquipped;

   UFUNCTION(BlueprintNativeEvent, Category = "TAT|HUD")
   void OnHUDToolWidgetCreated(UTATHUDToolWidget* hudToolWidget);
   void OnHUDToolWidgetCreated_Implementation(UTATHUDToolWidget* hudToolWidget) {}

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   bool AuthorityUseAmmo(int32 ammoToUse);

   void AuthorityRefillAmmo();

   /// Sets the current ammo to the requested amount if it is within range
   /// Returns true if the update was successful, false otherwise
   bool AuthoritySetAmmoCount(int32 count);

   /// Increments the current ammo by the requested amount if it is within range
   /// Returns true if the update was successful, false otherwise
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   bool AuthorityIncreaseAmmoCount(int32 count);
   
   int32 GetRoomToIncreaseAmmoCount() const;

   /// Used to override the max ammo for the tool, typically used for upgrades
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityOverrideMaxAmmo(int32 newMaxAmmo);

#if OSE_CHEATS_ENABLED
   /// Will refill ammo even if the tool would ordinarily disallow refills
   void AuthorityRefillAmmo_CHEAT();
#endif

   int GetCurrentAmmo() const { return _currentAmmo; }
   int32 GetMaxAmmo() const { return MaxAmmo; }

   UFUNCTION(BlueprintPure)
   bool HasEnoughAmmoToUse(int32 ammoToUse) const { return _currentAmmo >= ammoToUse; }

   // Some tools exist in states separate from ammo that need to be reset (on respawn for example)
   UFUNCTION(BlueprintCallable)
   void AuthorityResetTool();
   // This is a Blueprint hook-in for the reset function to allow individual tools to run their own reset logic
   UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
   void BP_AuthorityOnResetTool();

   /// Gets the cost of the defined usage type
   /// Returns 0 if tool does not define cost for usage type
   UFUNCTION(BlueprintCallable, meta = (Categories = "Tool.Usage"))
   int32 GetToolCostByUsageType(FGameplayTag usageTypeTag) const;

   virtual bool CanCurrentlyBeUsed() const override;
   virtual bool IsToolStowedOnEquip() const override;

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
   virtual bool AuthorityGetParametersForWorldActor(UPARAM(Meta = (Categories = "Tool.Usage")) FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const;

   /// Fires when our ammo changes, both on server and the local client
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAmmoChanged, UTATToolComponent*, tool, int32, oldAmmo, int32, newAmmo);
   UPROPERTY(BlueprintAssignable, Category = "TAT|Ammo")
   FOnAmmoChanged OnAmmoChanged;

   /// Returns the HUD tool widget, if one was created
   UFUNCTION(BlueprintPure)
   UTATHUDToolWidget* GetHUDToolWidget() const { return _hudToolWidget; }

protected:

   void _ApplyEquippedGameplayEffects();
   void _RemoveEquippedGameplayEffects();
   void _RequestAnimSet() const;
   void _RemoveAnimSet() const;
   void _AddToolHUDWidget();
   void _RemoveToolHUDWidget();
   

   virtual bool OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool) override;
   virtual bool OnUnequip_Implementation() override;
   void ModifyVisuals_Implementation(EMeshPerspective perspective, USkeletalMeshComponent* mesh) override;

   virtual void _LinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer) override;
   virtual void _UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer) override;

   void _StartRemoveOnOutOfAmmo();
   void _OnAbilityEndForRemove(class UGameplayAbility* endedAbility);
   void _TryRemoveOnOutOfAmmo(UAbilitySystemComponent* asc);

   void _AuthorityRefillAmmoInternal();

   bool _AuthoritySetAmmoInternal(int32 newAmmoCount);

   UFUNCTION()
   void _OnRep_CurrentAmmo(int32 oldAmmo);

   /// Handle to GameplayEffect added as specified by EquippedEffectClass, for later cleanup.
   TArray<FActiveGameplayEffectHandle> _equippedEffectHandles;

   FDelegateHandle _abilityEndDelegateHandle;

   UPROPERTY(Transient)
   TObjectPtr<UTATHUDToolWidget> _hudToolWidget;

private:
   void _ReloadMetadataFromTable();

#if WITH_EDITOR
   void _BindToMetadataTableChanged();

   FDelegateHandle _onMetadataTableChangedHandle;
#endif

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   FDataTableRowHandle _metadataTableRowForChangedHandle;
#endif

   UPROPERTY(Transient, BlueprintReadOnly, Category = "TAT|Ammo", ReplicatedUsing=_OnRep_CurrentAmmo, meta=(AllowPrivateAccess = "true"))
   int32 _currentAmmo = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Ammo", Replicated, meta=(AllowPrivateAccess = "true", EditCondition = "AmmoType == ETATToolAmmoType::Finite", EditConditionHides, ClampMin = 0))
   int32 MaxAmmo = 3;
};
