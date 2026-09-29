// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATHUDIndicatorTypes.h"

// ue
#include "UObject/Interface.h"

#include "TATHUDIndicatorWidgetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Category = "TAT")
class TAT_API UTATHUDIndicatorWidgetInterface : public UInterface
{
   GENERATED_BODY()
};

/// An interface that can be implemented by widgets responsible for HUD indicators if they want to receive direct update callbacks
class TAT_API ITATHUDIndicatorWidgetInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.

public:

   UFUNCTION(BlueprintNativeEvent, Category = "HUD Indicator Widget Interface")
   void OnHUDIndicatorUpdated(const FTATHUDIndicatorState& newState);
   virtual void OnHUDIndicatorUpdated_Implementation(const FTATHUDIndicatorState& newState) {}

   UFUNCTION(BlueprintNativeEvent, Category = "HUD Indicator Widget Interface")
   void OnHUDIndicatorRemoved();
   virtual void OnHUDIndicatorRemoved_Implementation() {}

};
