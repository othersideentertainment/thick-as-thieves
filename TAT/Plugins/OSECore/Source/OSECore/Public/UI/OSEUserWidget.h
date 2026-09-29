// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "Blueprint/UserWidget.h"

#include "OSEUserWidget.generated.h"

class UAsyncTaskAnimateWidget;
class UWidgetAnimation;

enum class EOSEInputHardwareType : uint8;

UCLASS(meta = (DisableNativeTick))
class OSECORE_API UOSEUserWidget : public UUserWidget
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetRemovedFromParent);
   UPROPERTY(BlueprintAssignable, Category = "User Interface|OSE")
   FOnWidgetRemovedFromParent OnWidgetRemovedFromParent;

   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void RemoveFromParent() override;

   UAsyncTaskAnimateWidget* GetAsyncAnimateWidgetTask(const UWidgetAnimation* animation) const;

   // Stores async animation task state information. Returns false if a task with matching animation already exists.
   bool AddAsyncAnimateWidgetTask(UAsyncTaskAnimateWidget* task, const UWidgetAnimation* animation);

   // Removes async animate widget tasks associated with the given animation, returning false if not found.
   bool RemoveAsyncAnimateWidgetTask(const UWidgetAnimation* animation) { return AsyncAnimateWidgetTasks.Remove(animation) > 0; }

protected:
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|OSE")
   bool ReceiveOnInputHardwareTypeChanged = false;

   // Used to track running async animation tasks associated with this widget
   UPROPERTY(Transient)
   TMap<const UWidgetAnimation*, UAsyncTaskAnimateWidget*> AsyncAnimateWidgetTasks;

   UFUNCTION(BlueprintImplementableEvent, Category = "User Interface|OSE")
   void _OnInputHardwareTypeChanged(EOSEInputHardwareType hardwareType);
   void _OnInputHardwareTypeChanged_Implementation(EOSEInputHardwareType hardwareType) { }
};
