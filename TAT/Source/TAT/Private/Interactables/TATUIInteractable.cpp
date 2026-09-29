// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATUIInteractable.h"

// tat
#include "Player/TATPlayerController.h"
#include "Online/TATGameState.h"
#include "UI/TATActivatableWidget.h"
#include "UI/TATLayoutSubsystem.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "Interactables/TATInteractHighlightUtils.h"

// ue4
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUIInteractable)

DEFINE_LOG_CATEGORY_STATIC(LogUIInteractable, Log, All);

ATATUIInteractable::ATATUIInteractable()
{
   PrimaryActorTick.bCanEverTick = false;
}

void ATATUIInteractable::BeginPlay()
{
   Super::BeginPlay();
}

bool ATATUIInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (OnlyInteractableByMissionOwner && interactingCharacter != nullptr)
   {
      UWorld* world = GetWorld();
      check(world != nullptr);
      if (world->GetNetMode() == NM_Client)
      {
         APlayerState* interactingPlayerState = interactingCharacter->GetPlayerState();
         ATATGameState* gameState = world->GetGameState<ATATGameState>();
         if (interactingPlayerState != nullptr && gameState != nullptr && !gameState->IsMissionOwner(interactingPlayerState))
         {
            return false;
         }
      }
   }

   return !IsShowingScreen();
}

void ATATUIInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.PressAction = InteractPrompt;
}

FInteractStartResult ATATUIInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;
   ATATPlayerController* pc = Cast<ATATPlayerController>(interactingCharacter->GetController());
   if (pc != nullptr && pc->IsLocalPlayerController())
   {
      // make sure this is set before calling _OpenWidget so that the blueprint callbacks can access it
      _interactingPC = pc;

      // If we can show the tutorial widget first, do that - it will call _OnShowUI when closed.
      const bool didShowTutorialWidget = _ShouldShowTutorials() && _AddTutorialWidgetToViewport(pc);

      // We didn't show the tutorial, so show the UI immediately.
      if (!didShowTutorialWidget)
      {
         _OnShowUI(pc);
      }
   }
   return result;
}

void ATATUIInteractable::ShowHighlight_Implementation(bool showHighlight)
{
   if (AutoHighlightInteractMeshes)
   {
      UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
   }
}

bool ATATUIInteractable::_ShouldShowTutorials() const
{
   if (!ShowTutorialWidget || !IsValid(TutorialWidgetClass))
   {
      return false;
   }

   return true;
}

void ATATUIInteractable::_OnShowUI(ATATPlayerController* localController)
{
   check(localController != nullptr);
   check(localController->IsLocalPlayerController());

   if (localController != nullptr && localController->IsLocalPlayerController())
   {
      if (IsValid(CommonUIWidgetClass))
      {
         _AddCommonUIWidgetToViewport(localController);
      }
      else if (IsValid(WidgetClass))
      {
         _AddWidgetToViewport(localController);
      }
   }
}

void ATATUIInteractable::_OnHideUI()
{
   // Remove the OnDeactivated delegate
   if (IsValid(_commonUIWidget) && _onCommonUIDeactivatedDelegate.IsValid())
   {
      _commonUIWidget->OnDeactivated().Remove(_onCommonUIDeactivatedDelegate);
      _onCommonUIDeactivatedDelegate.Reset();
   }

   // no longer interacting
   _interactingPC = nullptr;
}

bool ATATUIInteractable::_AddWidgetToViewport(ATATPlayerController* localController)
{
   UTATScreenMgr* screenMgr = localController->GetTATScreenMgr();
   if (screenMgr == nullptr)
   {
      return false;
   }

   // create if not created
   if (_widget == nullptr)
   {
      const FString widgetName = FString::Printf(TEXT("TATUIInteractableWidget_%s"), *WidgetClass->GetName());
      _widget = CreateWidget<UTATScreenWidget>(localController, WidgetClass, MakeUniqueObjectName(localController, WidgetClass, *widgetName));
      if (!ensure(_widget != nullptr))
      {
         return false;
      }

      // tell our bp to set it up as needed
      _OnWidgetCreated(_widget);
   }

   // show it
   _OnWidgetAboutToShow(_widget);

   screenMgr->AddScreen(_widget, [weakThis = MakeWeakObjectPtr(this)](UTATScreenWidget* widget)
   {
      if (ATATUIInteractable* self = weakThis.Get())
      {
         // Blueprint event when the widget is closed
         self->_OnWidgetClosed(widget);

         // Post-interact cleanup
         self->_OnHideUI();
      }
   });

   return true;
}

bool ATATUIInteractable::_AddTutorialWidgetToViewport(ATATPlayerController* localController)
{
   UTATScreenMgr* screenMgr = localController->GetTATScreenMgr();
   if (screenMgr == nullptr)
   {
      return false;
   }

   // create if not created
   if (_tutorialWidget == nullptr)
   {
      const FString widgetName = FString::Printf(TEXT("TATUIInteractableWidget_Tutorial_%s"), *WidgetClass->GetName());
      _tutorialWidget = CreateWidget<UTATScreenWidget>(localController, WidgetClass, MakeUniqueObjectName(localController, WidgetClass, *widgetName));
      if (!ensure(_tutorialWidget != nullptr))
      {
         return false;
      }

      // tell our bp to set it up as needed
      _OnTutorialWidgetCreated(_tutorialWidget);
   }

   // show it
   _OnTutorialWidgetAboutToShow(_tutorialWidget);

   screenMgr->AddScreen(_tutorialWidget, [weakThis = MakeWeakObjectPtr(this), weakController = MakeWeakObjectPtr(localController)](UTATScreenWidget* widget)
   {
      if (ATATUIInteractable* self = weakThis.Get())
      {
         // Blueprint event when the tutorial widget is closed
         self->_OnTutorialWidgetClosed();

         // Automatically open the regular widget now that the tutorial is closed
         if (ATATPlayerController* pc = weakController.Get())
         {
            self->_OnShowUI(pc);
         }
      }
   });

   return true;
}

bool ATATUIInteractable::_AddCommonUIWidgetToViewport(ATATPlayerController* localController)
{
   UTATLayoutSubsystem* layoutSubsystem = GetGameInstance()->GetSubsystem<UTATLayoutSubsystem>();
   if (layoutSubsystem == nullptr)
   {
      return false;
   }

   FTATLayoutInitFuncDelegate initDelegate;
   _commonUIWidget = Cast<UTATActivatableWidget>(layoutSubsystem->PushWidget(localController->GetLocalPlayer(), CommonUILayerTag, CommonUIWidgetClass, initDelegate));

   if (!ensure(_commonUIWidget != nullptr))
   {
      return false;
   }

   // tell our bp to set it up as needed
   _OnCommonUIWidgetCreated(_commonUIWidget);

   ensure(!_onCommonUIDeactivatedDelegate.IsValid());

   _onCommonUIDeactivatedDelegate = _commonUIWidget->OnDeactivated().AddLambda(
      [weakThis = MakeWeakObjectPtr(this), widget = _commonUIWidget, weakController = MakeWeakObjectPtr(localController)]()
      {
         if (ATATUIInteractable* self = weakThis.Get())
         {
            // CommonUI does not reset the input correctly, so doing it here to make sure the player can move/look as expected.
            // TODO: Remove this and fix the input correctly, the issue is mainly caused due to the transition to CommonUI 
            // while the game still mainly uses the older UI.
            if (ATATPlayerController* localPC = weakController.Get())
            {
               localPC->bShowMouseCursor = false;

               // Reset the cursor to the center of the screen to prevent camera sweeps based on where the mouse was last on UI
               int32 sizeX, sizeY;
               localPC->GetViewportSize(sizeX, sizeY);
               const FVector2D viewportSize = FVector2D(sizeX, sizeY);
               const FVector2D viewportCenter = viewportSize / 2.0f;
               localPC->SetMouseLocation(viewportCenter.X, viewportCenter.Y);

               UWidgetBlueprintLibrary::SetInputMode_GameOnly(localPC);
            }

            // Blueprint event when the widget is closed
            self->_OnCommonUIWidgetClosed(widget);

            // Post-interact cleanup
            self->_OnHideUI();
         }
      });

   return true;
}
