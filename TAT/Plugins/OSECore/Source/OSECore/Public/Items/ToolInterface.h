// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityInfo.h"
#include "Items/ToolInput.h"

// ue4
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "ToolInterface.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTools, Log, All);

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Tools")
class UToolInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------
/// Tool interface
//---------------------------------------------------------------------------------------------------

class OSECORE_API IToolInterface
{
   GENERATED_BODY()

public:

   /// Returns true if the tool is ready
   virtual bool IsReady() const = 0;

   // Returns true if the tool is playing the equip animation
   virtual bool IsPlayingEquipAnimation() const = 0;

   /// Returns true if the tool is equipped
   virtual bool IsEquipped() const = 0;

   /// Returns true if the tool is stowed
   virtual bool IsStowed() const = 0;

public:
   /// Returns the user-facing input info for the tool
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tools")
   FOSEToolInput GetToolInput() const;
   virtual FOSEToolInput GetToolInput_Implementation() const { return FOSEToolInput(); }

   /// Quickly stow the current tool so it's not in the avatar hands while performing things like mantling, sliding, etc
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tools")
   void SetStowed(bool stowed);
   virtual void SetStowed_Implementation(bool stowed) {  }

   /// Called by AnimNotifyState_OSEToolVisualState to indicate that an animation driven visual effect has begun.
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   void OnVisualStateBegin(FGameplayTag visualStateTag, float totalDuration);
   virtual void OnVisualStateBegin_Implementation(FGameplayTag visualStateTag, float totalDuration) { }

   /// Called by AnimNotifyState_OSEToolVisualState to indicate that an animation driven visual effect has ended.
   /// Possibly not called if the current tool changes mid animation. Be sure to clean up effects if tool component is unequipped.
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   void OnVisualStateEnd(FGameplayTag visualStateTag);
   virtual void OnVisualStateEnd_Implementation(FGameplayTag visualStateTag) { }

protected:

   /// Called when this tool is added to tool set
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   bool OnAddToToolSet();
   virtual bool OnAddToToolSet_Implementation() { return false; }

   /// Called when this tool is removed from tool set
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   bool OnRemoveFromToolSet();
   virtual bool OnRemoveFromToolSet_Implementation() { return false; }

   /// Called when this tool is equipped
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   bool OnEquip(const TScriptInterface<IToolInterface>& prevTool);
   virtual bool OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool) { return false; }

   /// Called when this tool is unequipped
   UFUNCTION(BlueprintNativeEvent, Category = "Tools")
   bool OnUnequip();
   virtual bool OnUnequip_Implementation() { return false; }
};
