// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/UpgradeQueryInterface.h"
#include "Items/ToolInput.h"
#include "Items/ToolInterface.h"
#include "Items/ToolVisuals.h"

// ue4
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"

#include "ToolComponent.generated.h"

class UInputMappingContext;
class UOSEGameplayAbilitySet;
class UToolSetComponent;
class UAnimInstance;

//--------------------------------------------------------------------------------------------------
/// Component that represents a dynamically created tool
//--------------------------------------------------------------------------------------------------
UCLASS(ClassGroup = (Tools), Abstract, Blueprintable, BlueprintType
   , meta = (BlueprintSpawnableComponent, IsBlueprintBase = "true")
   , hideCategories = (Activation, Collision, ComponentReplication, Components, "Components|Activation", ComponentTick, Cooking, LOD, Object, Physics, Rendering, Utilities))
class OSECORE_API UToolComponent
   : public UActorComponent
   , public IToolInterface
   , public IUpgradeQueryInterface
   , public IAbilitySystemInterface
{
   GENERATED_BODY()

public:

   // Sets default values for this component's properties
   UToolComponent();

   // UActorComponent interface
   virtual void OnComponentCreated() override;
   virtual void OnRegister() override;
   virtual void OnUnregister() override;
   virtual void OnComponentDestroyed(bool destroyingHierarchy) override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   // Returns true if the tool is ready
   virtual bool IsReady() const override;
   virtual bool IsPlayingEquipAnimation() const override { return _isPlayingEquipAnim; }
   // Returns true if the tool is equipped
   UFUNCTION(BlueprintPure)
   virtual bool IsEquipped() const override final;

   /// Returns true if the tool is stowed
   UFUNCTION(BlueprintPure)
   virtual bool IsStowed() const override final { return _counterStowed > 0; }

   // Whether the tool is equipped but not stowed
   UFUNCTION(BlueprintPure)
   bool IsShown() const { return IsEquipped() && !IsStowed(); }

   /// Returns true if the tool is usable (i.e. it has ammo)
   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
   virtual bool CanCurrentlyBeUsed() const { return true; }

   /// Upon equipping this tool, returns true if the tool should be stowed (prevents mesh visibility and anim layer linking)
   virtual bool IsToolStowedOnEquip() const { return StowToolIfUnusable && !CanCurrentlyBeUsed(); }


   const FOSEAbilityInfo& GetToolInfo() const { return ToolInfo; }
   const FToolAnimations& GetToolAnimations() const { return ToolAnimations; }

   UFUNCTION(BlueprintPure, Category = "Tools")
   ACharacter* GetOwnerCharacter() const;

   /// Get the 1p skeletal mesh component for this tool
   UFUNCTION(BlueprintPure, Category = "Tools|Mesh")
   USkeletalMeshComponent* GetToolVisSkeletalMeshComp1p() const { return _toolVisSkeletalMeshComp1p; }
   
   /// Get the 3p skeletal mesh component for this tool
   UFUNCTION(BlueprintPure, Category = "Tools|Mesh")
   USkeletalMeshComponent* GetToolVisSkeletalMeshComp3p() const { return _toolVisSkeletalMeshComp3p; }
   
   /// Get the 3p unequipped skeletal mesh component for this tool
   UFUNCTION(BlueprintPure, Category = "Tools|Mesh")
   USkeletalMeshComponent* GetToolVisSkeletalMeshComp3pUnequipped() const { return _toolVisSkeletalMeshComp3pUnequipped; }

   USkeletalMesh* GetToolSkeletalMeshAssetFirstPerson() const;
   USkeletalMesh* GetToolSkeletalMeshAssetThirdPerson() const;

   /// IAbilitySystemInterface
   virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipToolFinished, const UToolComponent*, tool);

   // Called when the tool is fully equipped (i.e. input mapping context applied, tool ability activated)
   UPROPERTY(BlueprintAssignable, Category = "Tools")
   FOnEquipToolFinished OnEquipToolFinished;

protected:

   /// Shows the tool meshes
   virtual void ShowMesh();

   /// Hides the tool meshes
   virtual void HideMesh();

public:

   /// IToolInterface (public)
   virtual FOSEToolInput GetToolInput_Implementation() const override;


   // UpgradeQueryInterface
   virtual int32 GetUpgradeValue(FGameplayTag tag, int32 fallback = 0) const final;

protected:

   /// IToolInterface (protected)
   virtual bool OnAddToToolSet_Implementation() override;
   virtual bool OnRemoveFromToolSet_Implementation() override;
   virtual bool OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool) override;
   virtual bool OnUnequip_Implementation() override;
   virtual void SetStowed_Implementation(bool stowed) override;

   void _OnDestroyedWhileEquipped();

   UToolSetComponent* _GetOwningToolSet() const;

   // A hook to modify the visuals for a given mesh component
   UFUNCTION(BlueprintNativeEvent, Category=Tools)
   void ModifyVisuals(EMeshPerspective perspective, USkeletalMeshComponent* mesh);

   void _ModifyAllVisuals();
   virtual bool _ShouldModifyVisualsOnRegister() const { return true; }

   // Event fired when the tool shows its mesh (eg. after being equipped)
   UFUNCTION(BlueprintImplementableEvent, Category = Tools, meta = (DisplayName = "On Show Mesh"))
   void BP_OnShowMesh();

   // Event fired when the tool hides its mesh (eg. after being unequipped)
   UFUNCTION(BlueprintImplementableEvent, Category = Tools, meta = (DisplayName = "On Hide Mesh"))
   void BP_OnHideMesh();

protected:
   
   // Whether we want to stow the tool if it isn't usable
   // A Crossbow with no ammo should set this to false as we still want to see the crossbow if we're out of bolts
   // Gadgets like throwables and potions should set this to true as we should not be holding any of those in hand if we're out
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   bool StowToolIfUnusable = false;

   /// User facing tool data
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FOSEAbilityInfo ToolInfo;

   /// User facing custom input + input info for this tool
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FOSEToolInput ToolInput;

   /// Third person tool visual representation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FToolVisuals ToolVisuals3P;

   /// Third person tool visual representation for unequipped tools (ie sword on back, dagger on belt, etc)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FToolVisuals ToolVisuals3P_Unequipped;

   /// First person tool visual representation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FToolVisuals ToolVisuals1P;

   /// Tool animation data
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   FToolAnimations ToolAnimations;

   /// Ability type to grant on add, activate on equip
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TSubclassOf<class UToolAbility> ToolAbilityClass;

   /// Ability set(s) to grant when this tool is added to the tool set
   /// NOT automatically activated when equipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TArray<UOSEGameplayAbilitySet*> InitialAbilitySets;

   /// Ability set(s) to grant when this tool is equipped
   /// They are removed when the tool is unequipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TArray<UOSEGameplayAbilitySet*> EquippedAbilitySets;

   /// Valid on server only. Handle to granted ability
   UPROPERTY()
   FGameplayAbilitySpecHandle GrantedAbilityHandle;
   UPROPERTY()
   TArray<FGameplayAbilitySpecHandle> GrantedInitialAbilities;
   TArray<FGameplayAbilitySpecHandle> GrantedEquippedAbilities;

   /// Valid local and server. Handle to activated ability
   UPROPERTY()
   FGameplayAbilitySpecHandle ActivatedAbilityHandle;

   /// Counter for local tool equipped checks
   UPROPERTY()
   int32 CounterEquipped;

   /// Links an animation layer class to the owner's animation instance
   virtual void _LinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer);

   /// Unlinks an animation layer class from the owner's animation instance
   virtual void _UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer);

   void _UpdateStowedStateBasedOnUsability();

protected:

   /// Executes when equip animation is completed
   void _OnEquipFinished();

   /// Plays an animation montage on the owner. Returns the length of the animation montage in seconds. Returns 0 if failed to play.
   float _PlayToolAnimation(EToolAnimation toolAnimation, float playbackRate);
   float _PlayToolAnimation(class UAnimMontage* animMontage, float playbackRate);

   /// Stops a playing tool animation
   void _StopToolAnimation(EToolAnimation toolAnimation);
   void _StopToolAnimation(class UAnimMontage* animMontage);

   /// Enters an input context - returns true if successful, false if it could not be entered
   bool _EnterInputContext(UInputMappingContext* context);

   /// Exits an input context
   void _ExitInputContext(UInputMappingContext* context);

   void _LinkAnimClassLayers();
   void _UnlinkAnimClassLayers();

   /// Returns the OSE ability system component
   class UOSEAbilitySystemComponent* _GetOSEAbilitySystemComponent() const;

   /// Returns the local enhanced input subsystem, if we're local
   class UEnhancedInputLocalPlayerSubsystem* _GetEnhancedInputLocalPlayerSubsystem() const;

   /// Searches for our ability in the ability system component. Can be called from the server or locally to get the
   /// ability spec handle that matches our source object. This allows different tools to use the same ability class.
   FGameplayAbilitySpecHandle _FindAbility() const;

   /// Ability wrapper methods
   bool _GiveToolAbility();
   bool _ClearToolAbility();
   bool _ActivateToolAbility();
   bool _CancelToolAbility();

   // Helper functions to keep track of when we are (un)stowing for usability reasons
   void _StowBecauseUnusable();
   void _UnstowBecauseUsable();

   /// Equip animation timers
   FTimerHandle _timerHandle_OnEquipFinished;
   bool _isPlayingEquipAnim = false;

   // temporarily stowed counter
   int32 _counterStowed = 0;

   // keep track of whether our tool is stowed due to being unusable or for other reasons
   bool _isStowedBecauseUnusable = false;

   // Handling if we've entered the input context already, or need to defer it to after equipping is finished
   bool _shouldEnterInputContextAfterFinishEquip = false;

   UPROPERTY(Transient)
   TWeakObjectPtr<UToolSetComponent> _cachedOwningToolset;

   // cached spawned skeletal mesh components
   UPROPERTY(Transient) USkeletalMeshComponent* _toolVisSkeletalMeshComp1p = nullptr;
   UPROPERTY(Transient) USkeletalMeshComponent* _toolVisSkeletalMeshComp3p = nullptr;
   UPROPERTY(Transient) USkeletalMeshComponent* _toolVisSkeletalMeshComp3pUnequipped = nullptr;
};
