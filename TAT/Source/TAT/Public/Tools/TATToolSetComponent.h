// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Items/ToolSetComponent.h"
#include "TATToolSetComponent.generated.h"


class AOSECharacterBase;
class UTATToolComponent;

UENUM()
enum class ETATToolSetAutoUnequipBehavior
{
   /// Do not automatically unequip tools based on gameplay tags
   None,

   /// Automatically unequip tools when the specified gameplay tags are present
   AutoUnequipOnly,

   /// Automatically unequip tools when the specified gameplay tags are present,
   /// and then re-equip the previous tool when the tags are all removed
   AutoUnequipAndReEquip
};

//
// Created to have TAT specific implementations of the UToolSet
// Specifically helpful with _CanToolBeEquipped
//
UCLASS()
class TAT_API UTATToolSetComponent : public UToolSetComponent
{
   GENERATED_BODY()
public:
   UTATToolSetComponent();

   // From AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const;

   UFUNCTION(BlueprintCallable)
   void EquipDefaultTool();

   UFUNCTION(BlueprintCallable)
   void EquipLastUsedToolIfNoToolEquipped();

   // Gets the number of gear tools the player has in the toolset.
   // Gear is a player-selectable tool (visible in the radial menu and elsewhere).
   UFUNCTION(BlueprintCallable, Category = "Tools")
   int32 GetNumGearTools() const;

   // Gets the gear tool at the specified index.
   // Gear is a player-selectable tool (visible in the radial menu and elsewhere).
   UFUNCTION(BlueprintCallable, Category = "Tools")
   UToolComponent* GetGearToolAtIndex(int32 gearToolIndex) const;

   // If a tool isn't equipped, equip it. Otherwise unequip it and reequip the last equipped tool (if any).
   // Returns true if the specified tool was equipped, false on unequip or failure (eg. no such tool in the toolset).
   UFUNCTION(BlueprintCallable, Category = "Tools")
   bool ToggleEquippedTool(TSubclassOf<UToolComponent> toolClass);

   // Checks if any starting gear items are less than max ammo and are able to be refilled.
   // Used as a check for things like "will an ammo crate benefit this toolset at this time?"
   UFUNCTION(BlueprintCallable, Category = "Tools")
   bool CanAmmoBeRefilled() const;

   /// Refill ammo for all the gear we started with, e.g. at a refill station
   /// Does not affect ammo for tools we picked up later
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityRefillAmmoForStartingGear();

   /// Reset all the tools we started with
   /// Does not affect tools we picked up later
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityResetStartingGear();

   /// If the tool exists on our toolset, add it and set the ammo to the requested count
   /// If it is already on our toolset, increase the ammo by this amount if possible
   /// Returns true if we accepted the ammo increase: 
   UFUNCTION(BlueprintCallable, Category = "Tools")
   bool AuthorityTryAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass, int32 ammoCount, bool removeOnOutOfAmmo = true);

   /// Check if we can add the given tool, with the given ammo amount
   /// If it already exists on our toolset, check if we can increase the ammo count by that much
   /// If it doesn't already exist, check if the requested amount is <= the max ammo for the tool
   UFUNCTION(BlueprintPure, Category = "Tools")
   bool HasRoomToAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass, int32 ammoCount) const;

   /// Get the amount of ammo we could add to this tool
   /// If it already exists on our toolset, get the amount we could increase
   /// If it doesn't already exist, get the max ammo for the tool
   UFUNCTION(BlueprintPure, Category = "Tools")
   int32 GetRoomToAddToolWithAmmo(TSubclassOf<UTATToolComponent> toolClass) const;

   /// If the tool exists on our toolset, check the ammo cost for the defined tool usage
   /// Returns 0 if the tool doesn't exist or doesn't define the usage type
   UFUNCTION(BlueprintPure, Category = "Tools", meta = (Categories = "Tool.Usage"))
   int32 GetAmmoCostByUsageType(TSubclassOf<UTATToolComponent> toolClass, FGameplayTag usageType) const;

   /// Add to our toolset, and mark it down as a starting tool that's part of our loadout
   /// NOTE: This is BP-exposed so that the tutorial can grant tools as if they were
   ///       starting tools, but should not be used during gameplay for other reasons.
   UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Tools")
   void AuthorityAddStartingToolClass(TSubclassOf<UToolComponent> toolClass);

   virtual bool IsSuppressingAnimations() const override;
   bool IsAutoStowed() const { return _isAutoStowed; }

   /// Should we automatically equip the default tools on startup
   UPROPERTY(EditDefaultsOnly)
   bool EquipDefaultToolsAutomatically = false;

   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "EquipDefaultToolsAutomatically", EditConditionHides))
   TSubclassOf<UToolComponent> DefaultToolPhysical;

   /// Should we unequip and/or re-equip tools based on the presence of the tags in TagsToCauseUnequip
   UPROPERTY(EditDefaultsOnly)
   ETATToolSetAutoUnequipBehavior AutoUnequipBehavior = ETATToolSetAutoUnequipBehavior::None;

   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "AutoUnequipBehavior != ETATToolSetAutoUnequipBehavior::None", EditConditionHides))
   FGameplayTagContainer TagsToCauseUnequip;

   /// If true, then we will suppress animations when automatically unequipping our tool due to tag presence
   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "AutoUnequipBehavior != ETATToolSetAutoUnequipBehavior::None", EditConditionHides))
   bool SuppressAnimationsOnAutomaticUnequip = false;

protected:
   // From UToolSetComponent
   virtual bool ShouldCreateDefaultTools() const override;

private:

   UFUNCTION()
   void _OnEquippedToolChanged();

   UFUNCTION()
   void _OnToolRemoved(UToolComponent* tool);

   UFUNCTION()
   void _OnOwnerCharacterReady(AOSECharacterBase* ownerCharacter);

   void _OnUnequipTagsChanged(const FGameplayTag tag, int32 newTagCount);

   UFUNCTION()
   void _OnAuthorityDefaultToolsHaveBeenCreated();

   UFUNCTION()
   void _AuthorityOnAbilitySystemComponentReady();

   void _OnAbilitiesInitialized();
   void _OnAbilitiesReset();

   void _OnAutoStowTagsChanged(const FGameplayTag tag, int32 newTagCount);
   void _SetIsAutoStowed(bool isAutoStowed);

   void _DoInitialSetup(AOSECharacterBase* ownerCharacter);
   void _SetIsRequestingUnequip(bool isRequestingUnequip);

   UPROPERTY(Transient)
   TSubclassOf<UToolComponent> _lastEquippedTool;

   UPROPERTY(Transient)
   TSubclassOf<UToolComponent> _toolEquippedBeforeToggle;

   UPROPERTY(Transient, Replicated)
   TArray<UTATToolComponent*> _startingGear;

   bool _isRequestingUnequip = false;
   bool _toolEquippedBeforeRequestingUnequip = false;
   bool _isAutoStowed = false;

   TMap<FGameplayTag, FDelegateHandle> _tagChangedDelegateHandles;
};
