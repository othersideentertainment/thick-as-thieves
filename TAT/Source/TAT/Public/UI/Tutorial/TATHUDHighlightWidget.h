// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATUserWidget.h"

#include "TATHUDHighlightWidget.generated.h"

// A simple widget that positions itself over the bounds of a specified widget in the HUD
// 
// This is not the most performant way to do this, but it is sufficient when used for
// realtively fixed elements in the tutorial.
// 
// NOTE: Explicitly not including `DisableNativeTick`, since it is ticking
UCLASS(Blueprintable)
class TAT_API UTATHUDHighlightWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   virtual void NativeConstruct() override;
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;

   UFUNCTION(BlueprintCallable)
   void SetTarget(UWidget* target);

private:
   void _PositionToTarget();
   
   UPROPERTY(Transient)
   TObjectPtr<UWidget> _targetWidget;
};
