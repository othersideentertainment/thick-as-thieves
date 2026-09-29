// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Input/OSEEnhancedInputPriority.h"
#include "UObject/Interface.h"
#include "OSEActivatableWidgetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UOSEActivatableWidgetInterface : public UInterface
{
   GENERATED_BODY()
};

///
/// Interface for a UMG widget that can be activated and deactivated.
/// Ex. When an OSEAnimatedSwitcher widget switches to a widget, it can automatically activate it if it has this interface.
///
/// You must use the following functions to activate and deactivate widgets:
///   UOSEUIFunctionLibrary::ActivatableWidgetInterface_Activate()
///   UOSEUIFunctionLibrary::ActivatableWidgetInterface_Deactivate()
///
class OSECORE_API IOSEActivatableWidgetInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activatable Widget Interface")
   bool IsActivated() const;

   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   void OnWidgetActivated();

   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   void OnWidgetDeactivated();

   /// If UseInputContextWhenActivated returns true and an input context is successfully enabled or disabled, this event is called
   /// to allow widgets to update input hints and glyphs
   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   void OnInputContextToggled(bool enabled);

   /// Should this widget automatically be focused when activated?
   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   bool ShouldFocusWidgetOnActivation() const;
   virtual bool ShouldFocusWidgetOnActivation_Implementation() const { return true; }

   /// Gets the widget to focus when this widget activates (if null, focuses the whole widget)
   /// Note that this will only be called if ShouldFocusWidgetOnActivation() returns true
   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   UWidget* GetDesiredFocusTarget() const;
   virtual UWidget* GetDesiredFocusTarget_Implementation() const { return nullptr; }

   /// Return true and specify an input context priority to set one automatically when this widget is activated, and remove it automatically on deactivation
   UFUNCTION(BlueprintNativeEvent, Category = "Activatable Widget Interface")
   bool UseInputContextWhenActivated(FOSEInputContextPriority& inputContextPriority) const;
   virtual bool UseInputContextWhenActivated_Implementation(FOSEInputContextPriority& inputContextPriority) const { return false; }

};
