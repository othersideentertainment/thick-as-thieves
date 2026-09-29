// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "GameFramework/Actor.h"

#include "TATUIInteractable.generated.h"

class ATATPlayerController;
class UTATScreenWidget;
class UTATActivatableWidget;

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATUIInteractable : public AActor, public IInteractableInterface
{
   GENERATED_BODY()
   
public:
   ATATUIInteractable();

   /// CommonUIWidgetClass overrides WidgetClass if both are defined
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="UI Interactable")
   TSubclassOf<UTATActivatableWidget> CommonUIWidgetClass;

   /// The layer to add the CommonUIWidget
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="UI Interactable", Meta = (Categories = "UI.Layer"))
   FGameplayTag CommonUILayerTag;

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Interactable")
   TSubclassOf<UTATScreenWidget> WidgetClass;

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Interactable")
   FText InteractPrompt;

   /// Only allow the mission owner to interact with this.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI Interactable")
   bool OnlyInteractableByMissionOwner = false;

   /// If enabled, HighlightInteractMeshes will automatically be called in the ShowHighlight event.
   /// Disable this if you need manual control over mesh highlighting.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI Interactable")
   bool AutoHighlightInteractMeshes = true;

   /// If enabled, the tutorial screen is added to the viewport first before the main widget.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI Interactable")
   bool ShowTutorialWidget = false;

   /// Tutorial screen to show before the actual widget is added to the viewport
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI Interactable", meta = (EditCondition = "ShowTutorialWidget"))
   TSubclassOf<UTATScreenWidget> TutorialWidgetClass;

protected:
   // from AActor
   virtual void BeginPlay() override;

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

   UFUNCTION(BlueprintPure, Category = "UI Interactable")
   ATATPlayerController* GetInteractingController() const { return _interactingPC; }

   UFUNCTION(BlueprintPure, Category = "UI Interactable")
   bool IsShowingScreen() const { return GetInteractingController() != nullptr; }

   UFUNCTION(BlueprintPure, Category = "UI Interactable")
   UTATScreenWidget* GetScreenWidget() const { return _widget; }

protected:
   /// If this returns true, a tutorial widget will be shown first before the main widget
   virtual bool _ShouldShowTutorials() const;

   /// Show the main widget. Can be overridden by subclasses to do something other than spawn a widget (eg. request that the server spawn a widget for all players)
   virtual void _OnShowUI(ATATPlayerController* localController);

   /// Called after the main widget is hidden. If you override _OnShowUI, you are responsible for calling this after your widget is no longer on the screen.
   virtual void _OnHideUI();

   bool _AddWidgetToViewport(ATATPlayerController* localController);
   bool _AddTutorialWidgetToViewport(ATATPlayerController* localController);

   bool _AddCommonUIWidgetToViewport(ATATPlayerController* localController);

   UFUNCTION(BlueprintNativeEvent, Category = "UI Interactable")
   void _OnTutorialWidgetCreated(UTATScreenWidget* widget);
   virtual void _OnTutorialWidgetCreated_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent, Category = "UI Interactable")
   void _OnTutorialWidgetAboutToShow(UTATScreenWidget* widget);
   virtual void _OnTutorialWidgetAboutToShow_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent, Category = "UI Interactable")
   void _OnTutorialWidgetClosed();
   virtual void _OnTutorialWidgetClosed_Implementation() {}

   UFUNCTION(BlueprintNativeEvent)
   void _OnWidgetCreated(UTATScreenWidget* widget);
   virtual void _OnWidgetCreated_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent)
   void _OnWidgetAboutToShow(UTATScreenWidget* widget);
   virtual void _OnWidgetAboutToShow_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent)
   void _OnWidgetClosed(UTATScreenWidget* widget);
   virtual void _OnWidgetClosed_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent)
   void _OnCommonUIWidgetCreated(UTATActivatableWidget* widget);
   virtual void _OnCommonUIWidgetCreated_Implementation(UTATActivatableWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent)
   void _OnCommonUIWidgetClosed(UTATActivatableWidget* widget);
   virtual void _OnCommonUIWidgetClosed_Implementation(UTATActivatableWidget* widget) {}

private:
   UPROPERTY(Transient)
   ATATPlayerController* _interactingPC = nullptr;

   UPROPERTY(Transient)
   UTATScreenWidget* _tutorialWidget = nullptr;

   UPROPERTY(Transient)
   UTATScreenWidget* _widget = nullptr;

   UPROPERTY(Transient)
   UTATActivatableWidget* _commonUIWidget = nullptr;

   FDelegateHandle _onCommonUIDeactivatedDelegate;
};
