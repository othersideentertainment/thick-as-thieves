// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue4
#include "Components/SceneComponent.h"
#include "GameplayAbilitySpec.h"

#include "ToolSetComponent.generated.h"

class UToolComponent;
class UOSEAbilitySystemComponent;


/// Stores the current tool, and any related information that needs to atomically change,
/// even via replication
USTRUCT()
struct OSECORE_API FOSECurrentToolData
{
   GENERATED_BODY()

   /// The tool that we have currently equipped
   UPROPERTY(Transient)
   UToolComponent* ToolObject = nullptr;

   /// Whether we want to play animations for equipping/unequipping/etc.
   UPROPERTY(Transient)
   bool SuppressAnimations = false;
};

// substantially similar to FOSEAbilityBindRequirements, but kept separate in case they deviate
USTRUCT()
struct OSECORE_API FOSEDefaultToolRequirements
{
   GENERATED_BODY()

   /// The upgrade tag required to grant the tool (or none)
   UPROPERTY(EditDefaultsOnly, Category = "Requirements", meta=(Categories="Upgrades"))
   FGameplayTag RequiredUpgradeTag;

   /// The level of the upgrade required to grant the tool
   UPROPERTY(EditDefaultsOnly, Category = "Requirements")
   int32 RequiredUpgradeLevel = 1;

   bool IsMet(const UOSEAbilitySystemComponent* asc) const;
};

USTRUCT()
struct OSECORE_API FOSEDefaultToolEntry
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, meta = (MustImplement = "/Script/OSECore.ToolInterface"))
   TSubclassOf< UToolComponent > ToolClass;

   UPROPERTY(EditDefaultsOnly)
   FOSEDefaultToolRequirements Requirements;
};

//---------------------------------------------------------------------------------------------------
/// Tool set component class
///
/// This is a concrete implementation of the tool set interface. As this class is the actual concrete
/// implementation of a tool set, querying the tool set _system_ interface returns this object.
///
/// It also implements the tool set interface; see notes below as to why this was done.
///
/// \see IToolSetInterface, IToolSetSystemInterface, IToolInterface
//---------------------------------------------------------------------------------------------------

UCLASS(ClassGroup = (Tools)
   , meta = (BlueprintSpawnableComponent)
   , hideCategories = (Activation, Collision, ComponentReplication, Components, "Components|Activation", ComponentTick, Cooking, LOD, Object, Physics, Rendering, Utilities))
   class OSECORE_API UToolSetComponent
   : public UActorComponent
   , public IToolSetInterface
   , public IToolSetSystemInterface
{
   GENERATED_BODY()

public:

   /// Sets default values for this component's properties
   UToolSetComponent();

   // UActorComponent interface
   virtual FString GetReadableName() const override;

   /// Begins play for this component. Occurs at level startup or actor spawn
   /// This is before BeginPlay (Actor or Component).
   /// All Components(that want initialization) in the level will be Initialized on load before any Actor / Component gets BeginPlay.
   virtual void BeginPlay() override;

   /// Ends play for this component. Called from AActor::EndPlay only if bHasBegunPlay is true
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   // Called every frame
   virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

   // Our equipped tool has changed
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedItemChanged);
   UPROPERTY(BlueprintAssignable, Category = "Tools")
   FOnEquippedItemChanged OnEquippedToolChanged;

   /// A tool has been added to the tool set
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolAdded, UToolComponent*, tool);
   UPROPERTY(BlueprintAssignable, Category = "Tools")
   FOnToolAdded OnToolAdded;

   /// A tool has been removed from the tool set
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolRemoved, UToolComponent*, tool);
   UPROPERTY(BlueprintAssignable, Category = "Tools")
   FOnToolRemoved OnToolRemoved;

   /// A tool has finished its equip routine with its input mapping context applied + tool ability triggered
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipToolFinished, const UToolComponent*, tool);
   UPROPERTY(BlueprintAssignable, Category = "Tools")
   FOnEquipToolFinished OnEquipToolFinished;

public:

   virtual bool IsSuppressingAnimations() const { return CurrentToolData.SuppressAnimations; }

   const TArray<FOSEDefaultToolEntry>& GetDefaultTools() const { return DefaultTools; }

   // IToolSetInterface
   virtual bool IsReady() const override;
   virtual int32 GetNumTools() const override;
   virtual bool IsEquipmentLocked() const override;
   virtual void LockEquipmentByClass(TSubclassOf<UToolComponent> toolClass) override;
   virtual void LockEquipment(bool bUnequipCurrentTool = false) override;
   virtual void UnlockEquipment() override;
   virtual UToolComponent* GetCurrentTool() const override;
   virtual int32 GetIndexOfCurrentTool() const override;
   virtual UToolComponent* GetToolAtIndex(int32 toolIndex) const override;
   virtual UToolComponent* GetToolByCategory(const FGameplayTag& toolCategory, bool matchesExact) const override;
   virtual UToolComponent* GetToolByClass(TSubclassOf<UToolComponent> toolClass, bool matchesExact) const override;
   virtual FGameplayTag GetCurrentToolCategory() const override;
   virtual TArray<UToolComponent*> FindToolsInCategories(const FGameplayTagContainer& toolCategories) const override;
   virtual bool HasToolClass(TSubclassOf<UToolComponent> toolClass) const override;
   virtual bool HasToolCategory(const FGameplayTag& toolCategory) const override;
   virtual bool HasToolEquipped(const FGameplayTag& toolCategory) const override;
   virtual bool EquipToolByClass(TSubclassOf<UToolComponent> toolClass) override;
   virtual bool EquipToolByCategory(const FGameplayTag& toolCategory, bool matchesExact) override;
   virtual bool UnequipCurrentTool() override;
   virtual bool UnequipCurrentToolByCategory(const FGameplayTag& toolCategory, bool matchesExact) override;
   virtual UToolComponent* AuthorityCreateToolClass(TSubclassOf<UToolComponent> toolClass) override;
   virtual bool AuthorityAddToolClass(TSubclassOf<UToolComponent> toolClass) override;
   virtual bool AuthorityAddTool(UToolComponent* toolComponent) override;
   virtual int AuthorityRemoveToolsOfClass(TSubclassOf<UToolComponent> toolClass) override;
   virtual int AuthorityRemoveToolsOfCatagory(const FGameplayTag& toolCategory, bool matchesExact) override;
   virtual void StowCurrentTool(bool stowed) override;

   // IToolSetSystemInterface
   virtual TScriptInterface<IToolSetInterface> GetToolSetInterface() const override;

#if WITH_EDITOR
   // UObject
   virtual void PostLoad() override;
#endif

protected:
   /// [Server] Determine if we should create the default tools
   virtual bool ShouldCreateDefaultTools() const { return true; }

   /// [Server] Creates default tools
   void CreateDefaultTools();

   /// [Server] Removes all tools and destroys them
   void DestroyAllTools();

   /// [Server] Add tool to set
   virtual bool AddTool(UToolComponent* tool);

   /// [Server] Remove tool from set
   virtual void RemoveTool(UToolComponent* tool);

   /// [Server + Local] Equips an existing tool from the set
   virtual void EquipTool(UToolComponent* tool);

   /// [Server] Equip tool
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerEquipTool(UToolComponent* tool);

   /// [Replication] Tools array has changed
   UFUNCTION()
   void OnRep_Tools(const TArray<UToolComponent*>& oldTools);

   /// [Replication] Current tool replication handler
   UFUNCTION()
   void OnRep_CurrentToolInfo(const FOSECurrentToolData& prevToolInfo);

   /// Updates current tool
   void SetCurrentTool(UToolComponent* newTool, UToolComponent* prevTool = nullptr);

   /// [Server + Local] Locks or unlocks equipment
   void SetLockEquipment(bool lockEquipment);

   /// [Server] Set equipment lock
   UFUNCTION(Reliable, Server)
   void ServerSetLockEquipment(bool lockEquipment);

   /// Updates equipment lock
   void InternalSetLockEquipment(bool lockEquipment);

protected:

   /// Whether or not to enable local prediction when equipping / unequipping tools.
   /// Local prediction essentially allows the equip logic to run immediately without
   /// waiting for the server to replicate the equipped tool.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   bool EnableLocalPrediction;

   /// Whether or not to auto-equip the default tool when default tools are created
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tools")
   bool AutoEquipDefaultTool;

   UPROPERTY(Transient)
   bool CheckForAutoEquip;

   /// Whether equipment is locked. If true, tools can't be equipped
   UPROPERTY(Transient, Replicated)
   bool bIsEquipmentLocked = false;

#if WITH_EDITORONLY_DATA
   /// Default tool list
   UPROPERTY()
   TArray< TSubclassOf< UToolComponent > > DefaultToolClasses_DEPRECATED;
#endif

   UPROPERTY(EditDefaultsOnly, Category = "Tools", meta = (ShowOnlyInnerProperties))
   TArray<FOSEDefaultToolEntry> DefaultTools;

   UPROPERTY(Transient, ReplicatedUsing = OnRep_Tools)
   TArray<UToolComponent*> Tools;

   /// Currently equipped tool
   UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentToolInfo)
   FOSECurrentToolData CurrentToolData;

   /// The number of starting tools actually initialized + 1. 0 if not yet known or replicated
   UPROPERTY(Transient, Replicated)
   int32 _startingToolCountPlusOne;

protected:

   /// Returns ability system component
   class UOSEAbilitySystemComponent* GetAbilitySystemComponent() const;

   // Check to see if this tool is in a state to be equipped or re-equipped.
   virtual bool _CanToolBeEquipped(const UToolComponent* tool) const;

   // Will immediately run the delegate if default tools have already been created. Otherwise, will register the delegate to be called
   // after the default tools have been created
   void _CallOrRegisterAuthorityDefaultToolsCreatedDelegate(const FSimpleMulticastDelegate::FDelegate& toolsCreatedDelegate);
   UToolComponent* _FindToolOfExactClass(TSubclassOf<UToolComponent> toolClass) const;

   /// Subclasses may want to gate code-driven equips on this, which lets us know if an equip is already in-progress,
   /// since SetCurrentTool is not re-entrant safe
   bool _IsCurrentlyChangingEquippedTool() const { return _isChangingEquippedTool; }

   virtual void _OnToolAdded(UToolComponent* newToolComp) {}
   virtual void _OnToolRemoved(UToolComponent* oldToolComp) {}

private:
   // utility functions
   bool _TryEquipTool(UToolComponent* tool);
   bool _ValidateSpawnedTool(UToolComponent* spawnedTool) const;

   UFUNCTION()
   void _OnEquipToolFinished(const UToolComponent* tool);

   bool _authorityHasCreatedDefaultTools = false;

   FSimpleMulticastDelegate _onAuthorityDefaultToolsCreatedDelegate;

   bool _isChangingEquippedTool = false;
};
