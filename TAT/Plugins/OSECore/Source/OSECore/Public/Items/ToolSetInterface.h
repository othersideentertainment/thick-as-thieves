// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Items/ToolInterface.h"

#include "ToolSetInterface.generated.h"

struct FGameplayTag;

class UToolComponent;

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools", meta = (CannotImplementInterfaceInBlueprint))
class UToolSetInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------
/// Tool Set interface
/// 
/// This interface defines what it means to be a tool set. Tool Sets contain tools that can be
/// selected, equipped, unequipped, used, and queried.
/// 
/// \note Currently, this interface has a dependency on the concrete implementation of a
///      tool (UToolComponent). This should be changed to support TScriptInterface<IToolInterface>
///      where possible (the tool set spawns tools and compares classes, so this may be challenging)
//---------------------------------------------------------------------------------------------------
class OSECORE_API IToolSetInterface
{
   GENERATED_BODY()

public:

   static TScriptInterface<IToolSetInterface> GetToolSetFromActor(AActor* actor);

   /// Returns true if the tool set is ready
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool IsReady() const = 0;

   /// Returns the number of tools in the tool set
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual int32 GetNumTools() const = 0;

   /// Returns whether equipment is locked. If true, tool can't be equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool IsEquipmentLocked() const = 0;

   /// Equip the given tool and disable equipping tools. If ItemClass is null, unequip the current tool.
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual void LockEquipmentByClass(TSubclassOf<UToolComponent> itemClass) = 0;

   /// Lock equipment with the option to unequip the current tool
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual void LockEquipment(bool bUnequipCurrentTool = false) = 0;

   /// Enable equipping tool, keeping the current tool equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual void UnlockEquipment() = 0;

   /// Returns the tool component reference for the current tool
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual UToolComponent* GetCurrentTool() const = 0;
   
   /// Returns the index of the current tool, or -1 if none is equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual int32 GetIndexOfCurrentTool() const = 0;

   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual FGameplayTag GetCurrentToolCategory() const = 0;

   // Returns the tool component reference for the specified tool index
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual UToolComponent* GetToolAtIndex(int32 toolIndex) const = 0;

   /// Returns a tool given a tool category
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual UToolComponent* GetToolByCategory(const FGameplayTag& itemCategory, bool matchesExact) const = 0;

   /// Returns a tool matching the specified class or a subclass of it
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual UToolComponent* GetToolByClass(TSubclassOf<UToolComponent> itemClass, bool matchesExact) const = 0;

   /// Returns an array of tools with matching tags
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual TArray<UToolComponent*> FindToolsInCategories(const FGameplayTagContainer& itemCategories) const = 0;

   /// Returns true if there exists a tool matching the specified class or a subclass of it
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool HasToolClass(TSubclassOf<UToolComponent> itemClass) const = 0;

   /// Returns true if there exists a tool of the specified category
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool HasToolCategory(const FGameplayTag& itemCategory) const = 0;

   /// Returns true if a tool of the given category is currently equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool HasToolEquipped(const FGameplayTag& itemCategory) const = 0;

   /// Equips a tool by class. Returns true if a tool matching the class was found and is not the current tool
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool EquipToolByClass(TSubclassOf<UToolComponent> itemClass) = 0;

   /// Equips a tool by category. Returns true if a tool matching the category was found and is not the current tool
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool EquipToolByCategory(const FGameplayTag& itemCategory, bool matchesExact) = 0;

   /// Unequips the current tool. Returns true if a tool was equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool UnequipCurrentTool() = 0;

   /// Unequips the current tool if it matches the category tag. Returns true if a tool with this tag was equipped
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool UnequipCurrentToolByCategory(const FGameplayTag& toolCategory, bool matchesExact) = 0;

   /// Creates a tool with the correct owner, assumes that the caller will later add it via AuthorityAddTool
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual UToolComponent* AuthorityCreateToolClass(TSubclassOf<UToolComponent> toolClass) = 0;

   template<typename T>
   T* AuthorityCreateToolClass(TSubclassOf<T> toolClass)
   {
      return CastChecked<T>(AuthorityCreateToolClass(TSubclassOf<UToolComponent>(toolClass)));
   }

   /// Adds this tool class to the tool set
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool AuthorityAddToolClass(TSubclassOf<UToolComponent> toolClass) = 0;

   /// Adds this specific tool to the tool set.  This version allows for setting up state on the tool component before adding it to the set, if needed.
   /// NOTE: This destroys the component if it fails to add, or registers the component if it successfully adds, so either way calling this function hands
   ///       the lifetime of the component over to the tool set
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual bool AuthorityAddTool(UToolComponent* toolComponent) = 0;

   /// Remove any tools from the toolset that match this class.  Returns the number of tools removed
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual int AuthorityRemoveToolsOfClass(TSubclassOf<UToolComponent> toolClass) = 0;


   /// Remove any tools from the toolset that match this class.  Returns the number of tools removed
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual int AuthorityRemoveToolsOfCatagory(const FGameplayTag& toolCategory, bool matchesExact) = 0;

   /// Quickly stow the current tool so it's not in the avatar hands while performing things like mantling, sliding, etc
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual void StowCurrentTool(bool stowed) = 0;
 

};
