// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "UI/OSEUserWidget.h"

// tat
#include "UI/TATNavigationConfig.h"

#include "TATUserWidget.generated.h"

class AGameStateBase;
class AOSECharacterBase;
class AOSEPlayerState;
class ATATCharacter;
class ATATGameState;
class ATATPlayerState;
class UTATScreenWidget;
class ULocalPlayer;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATUserWidget : public UOSEUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void NativePreConstruct() override;
   virtual FReply NativeOnKeyDown(const FGeometry& inGeometry, const FKeyEvent& inKeyEvent) override;
   virtual FReply NativeOnKeyUp(const FGeometry& inGeometry, const FKeyEvent& inKeyEvent) override;

   UFUNCTION(BlueprintImplementableEvent, Category="TAT UserWidget")
   FEventReply OnNavigationAction(const EUINavigationAction navigationAction);
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   FEventReply OnNavigationActionReleased(const EUINavigationAction navigationAction);
   
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   FEventReply OnNavigationDirection(const EUINavigation navigationDirection);
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   FEventReply OnNavigationDirectionReleased(const EUINavigation navigationDirection);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   FEventReply OnExtendedNavigationAction(const EUIExtendedNavigationAction navigationAction);
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   FEventReply OnExtendedNavigationActionReleased(const EUIExtendedNavigationAction navigationAction);

   // Use this to update any visuals that should only appear in the widget editor preview. Called on widget recompile / property edit.
   // For example: populating a list with example entries to preview their appearance in the UI.
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT UserWidget")
   void RefreshWidgetEditorPreview();

   UFUNCTION(BlueprintCallable, Category = "TAT UserWidget")
   UTATScreenWidget* TryFindParentTATScreenWidget() const;

   // helpers for forcing a widget to be hidden
   void ForceHidden(bool bHide);
   bool IsHidden() const
   {
      return _bHidden;
   }
   
   void SetVisibility(ESlateVisibility InVisibility) override;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnGameStateFound = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnPlayerStateAdded = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnPlayerStateRemoved = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnLocalPlayerStateAdded = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnLocalCharacterIsReady = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnLocalCharacterSaveIdChanged = false;
   UPROPERTY(EditDefaultsOnly, Category = "User Interface|TAT")
   bool ReceiveOnAddedToViewport = false;

   // Determines whether this UI element is forced to remain hidden when demo UI mode is toggled on.
   // Children are also hidden if this is set to true.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "User Interface|TAT|Demo")
   bool _HideInDemoMode = false;

protected:
   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnGameStateFound(ATATGameState* gameState);
   virtual void _OnGameStateFound_Implementation(ATATGameState* gameState);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnPlayerStateAdded(ATATPlayerState* ps);
   virtual void _OnPlayerStateAdded_Implementation(ATATPlayerState* ps);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnPlayerStateRemoved(ATATPlayerState* ps);
   virtual void _OnPlayerStateRemoved_Implementation(ATATPlayerState* ps);
   
   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnLocalPlayerStateAdded(ATATPlayerState* ps);
   virtual void _OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnLocalCharacterIsReady(AOSECharacterBase* character);
   virtual void _OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnLocalCharacterSaveIdChanged(FTATCharacterSaveId saveId);
   virtual void _OnLocalCharacterSaveIdChanged_Implementation(FTATCharacterSaveId saveId);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT UserWidget")
   void _OnAddedToViewport();
   virtual void _OnAddedToViewport_Implementation() {}


private:
   UFUNCTION()
   void _OnGameStateSetEvent(AGameStateBase* gameState);
   UFUNCTION()
   void _OnPlayerStateIsAdded(AOSEPlayerState* osePS);
   UFUNCTION()
   void _OnPlayerStateIsRemoved(AOSEPlayerState* osePS);
   UFUNCTION()
   void _OnLocalPlayerStateChanged(APlayerState* ps);
   UFUNCTION()
   void _HandleCharacterSaveIdChanged(ATATPlayerState* ps, FTATCharacterSaveId saveId);
   UFUNCTION()
   void _OnLocalPawnChanged(APawn* newPawn);
   UFUNCTION()
   void _OnLocalCharacterIsReadyChanged(AOSECharacterBase* character);
   UFUNCTION()
   void _OnWidgetAddedToViewport(UWidget* addedWidget, ULocalPlayer* localPlayer);

   void _HandleLocalPlayerState(ATATPlayerState* ps);
   void _InitGameStateBindings(AGameStateBase* gameState);
   void _ClearBindings(AGameStateBase* gameState);
   bool _ReceivesAnyGameStateEvent() const;

   // helper members for toggling UI visibility
   bool _bHidden = false;
   ESlateVisibility _hiddenVisibility = ESlateVisibility::Hidden;

   FDelegateHandle _onWidgetAddedHandle;
};
