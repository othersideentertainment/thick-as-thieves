// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"

#include "TATHUDToolWidget.generated.h"

class UTATToolComponent;

/// Widget that a tool can provide for display on the HUD while the tool is equipped
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATHUDToolWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   /// Called by the tool widget just after the widget has been created
   void PostConstructSetupToolWidget(UTATToolComponent* toolComponent);

   UFUNCTION(BlueprintPure)
   UTATToolComponent* GetOwningToolComponent() const { return _ownerTool.Get(); }

   /// Called after the widget has been constructed and a tool component has been assigned to this widget.
   /// Useful for setting up tool-related event handlers.
   UFUNCTION(BlueprintNativeEvent, Category = "HUD Tool Widget")
   void OnToolWidgetCreated(UTATToolComponent* toolComponent);
   void OnToolWidgetCreated_Implementation(UTATToolComponent* toolComponent) {}

   UFUNCTION(BlueprintNativeEvent, Category = "HUD Tool Widget")
   void OnToolEquipped(UTATToolComponent* toolComponent);
   void OnToolEquipped_Implementation(UTATToolComponent* toolComponent) {}
   
   UFUNCTION(BlueprintNativeEvent, Category = "HUD Tool Widget")
   void OnToolUnequipped(UTATToolComponent* toolComponent);
   void OnToolUnequipped_Implementation(UTATToolComponent* toolComponent) {}

private:
   TWeakObjectPtr<UTATToolComponent> _ownerTool;

};
