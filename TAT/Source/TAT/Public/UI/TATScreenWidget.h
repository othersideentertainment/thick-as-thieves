// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATUserWidget.h"

// ue
#include "Templates/SharedPointer.h"

#include "TATScreenWidget.generated.h"

class ATATPlayerController;
class UAkStateValue;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATScreenWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeOnInitialized() override;
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   // From UWidget
   virtual TSharedRef<SWidget> RebuildWidget() override;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // handlers for screen mgr to drive our screen state
   virtual void HandleOnScreenAddedToStack(ATATPlayerController& ownerPC);
   virtual void HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC);
   virtual void HandleOnScreenOnTopOfStack(ATATPlayerController& ownerPC);
   virtual void HandleOnScreenRemovedFromTopOfStack(ATATPlayerController& ownerPC);
   virtual void HandleOnTopOfStackWhenFocusRefreshed();

   void OnScreenReceiveFocus(TSharedPtr<SWidget> focusWidget, uint32 focusUser);

   UFUNCTION(BlueprintCallable, Category = "TAT Screen")
   void AddScreen();
   UFUNCTION(BlueprintCallable, Category = "TAT Screen")
   void RemoveScreen();
   UFUNCTION(BlueprintPure, Category = "TAT Screen")
   bool IsInScreenStack() const;

   // properties:

   // while this screen is on top, do we ignore move input?
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen")
   bool IgnoreMoveInput = true;
   // while this screen is on top, do we ignore look input?
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen")
   bool IgnoreLookInput = true;
   // while this screen is on top, do we show the mouse cursor?
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen")
   bool ShowMouseCursor = true;
   // when this screen is brought to the top, do we automatically set the Game and UI input mode or rely on the widget to set it itself?
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen")
   bool AutoSetGameAndUIInputModeOnShow = true;
   // while this screen is on top, do we allow the game ability system to accept input?
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen")
   bool DisableAbilitySystem = true;
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen|Audio", meta = (InlineEditConditionToggle))
   bool OverrideAkState = false;
   // State to assign to the Modal_State group while this screen is on the stack
   UPROPERTY(EditDefaultsOnly, Category = "TAT Screen|Audio", meta = (EditCondition="OverrideAkState"))
   TObjectPtr<UAkStateValue> AkStateWhileOnTopOfStackOverride;

protected:
   // when this widget is first added to the stack
   UFUNCTION(BlueprintNativeEvent, Category = "TAT Screen")
   void _OnScreenAddedToViewport();
   // when this widget is finally removed from the stack
   UFUNCTION(BlueprintNativeEvent, Category = "TAT Screen")
   void _OnScreenRemovedFromViewport();
   // called whenever this widget is moved to the top of the stack
   UFUNCTION(BlueprintNativeEvent, Category = "TAT Screen")
   void _OnScreenOnTopOfStack();
   // called whenever this widget is replaced on the top of the stack by another widget
   UFUNCTION(BlueprintNativeEvent, Category = "TAT Screen")
   void _OnScreenRemovedFromTopOfStack();

   // called to assign user focus whenever this widget reaches the top of the stack, or as a fallback if _lastFocusedWidget is no longer valid.
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TAT Screen")
   void _InitializeFocus();

   // Added to allow control of the ignore ability input count on the player controller.
   // This is to fix issues where old TATScreenWidgets open new CommonUI widgets.
   // TODO: This should be removed when the BPUI_ManagementScreen and BPUI_StartMenuOptions are fully updated to CommonUI. Dan C
   // See https://otherside.atlassian.net/browse/TVT-8607
   //  & https://otherside.atlassian.net/browse/TVT-8677aa
   UFUNCTION(BlueprintCallable, Category = "TAT Screen")
   void SetIgnoreAbilityInput(ATATPlayerController* ownerPC, bool shouldIgnore);

private:
   void _TryRemoveScreenFromMgr();
   void _SetInputState(ATATPlayerController& ownerPC, bool isOnTop);
   void _ReassignFocus();

private:
   // Caches our visibility setting, so we can change visibility in HandleOnScreenRemovedFromTopOfStack() and restore the initial value in HandleOnScreenOnTopOfStack()
   ESlateVisibility _initialVisibility;

   // Caches the user-focused widget when this screen is displaced from top of the stack via HandleOnScreenRemovedFromStack.
   // If this screen instance reaches the top of the stack again, _ReassignFocus() will prefer this widget.
   TWeakPtr<SWidget> _lastFocusedWidget;

   // ID of Slate user associated with _lastFocusedWidget
   uint32 _lastFocusedUser;
};
