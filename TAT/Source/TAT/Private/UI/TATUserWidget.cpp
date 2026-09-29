// (c) 2020-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATUserWidget.h"

// tat
#include "Player/TATCharacter.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "UI/TATScreenWidget.h"

// ue5
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATUserWidget, Log, All);

static void OnToggleDemoUIVisibility()
{
   TArray<UUserWidget*> tatWidgets;
   UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
      GEngine->GameViewport->GetWorld(), tatWidgets, UTATUserWidget::StaticClass(), false);
   for (auto* widget : tatWidgets)
   {
      auto* const tatWidget = Cast<UTATUserWidget>(widget);
      tatWidget->ForceHidden(!tatWidget->IsHidden());
   }
}

static FAutoConsoleCommand CVarToggleDemoHUD(
   TEXT("tat.ToggleDemoHUD"),
   TEXT("Toggles a set of HUD elements on or off.")
   TEXT("TATUserWidgets can be configured to be hidden or visible in this mode by toggling Hide in Demo Mode."),
   FConsoleCommandDelegate::CreateStatic(OnToggleDemoUIVisibility),
   ECVF_Default
);

void UTATUserWidget::NativeConstruct()
{
   Super::NativeConstruct();

   if (UWorld* world = GetWorld())
   {
      if (ReceiveOnAddedToViewport)
      {
         if (UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(world))
         {
            if (IsInViewport())
            {
               _OnAddedToViewport();
            }
            else
            {
               _onWidgetAddedHandle = viewportSubsystem->OnWidgetAdded.AddUObject(this, &UTATUserWidget::_OnWidgetAddedToViewport);
            }
         }
      }
      if (_ReceivesAnyGameStateEvent())
      {
         if (AGameStateBase* gameState = world->GetGameState())
         {
            _InitGameStateBindings(gameState);
         }
         else
         {
            world->GameStateSetEvent.AddUObject(this, &UTATUserWidget::_OnGameStateSetEvent);
         }
      }

      if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(world))
      {
         if (ReceiveOnLocalPlayerStateAdded || ReceiveOnLocalCharacterSaveIdChanged)
         {
            if (ATATPlayerState* ps = pc->GetTATPlayerState())
            {
               _HandleLocalPlayerState(ps);
            }
            else
            {
               pc->OnPlayerStateChanged.AddUniqueDynamic(this, &UTATUserWidget::_OnLocalPlayerStateChanged);
            }
         }

         if (ReceiveOnLocalCharacterIsReady)
         {
            if (AOSECharacterBase* character = pc->GetPawn<AOSECharacterBase>())
            {
               _OnLocalPawnChanged(character);
            }

            // always bind to pawn changed, even when we already have a character because
            // we want to re-send the _OnLocalCharacterIsReady() whenever we change characters
            pc->OnPawnChanged.AddUniqueDynamic(this, &UTATUserWidget::_OnLocalPawnChanged);
         }
      }
      else
      {
         UE_LOG(LogTATUserWidget, Error, TEXT("Unable to find the local playercontroller, maybe %s is being used outside of the hud?"), *GetName());
      }
   }
}

void UTATUserWidget::NativeDestruct()
{
   Super::NativeDestruct();

   if (GetWorld() && GetWorld()->GetGameState())
   {
      _ClearBindings(GetWorld()->GetGameState());
   }

   if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(GetWorld()))
   {
      pc->OnPlayerStateChanged.RemoveAll(this);
      pc->OnPawnChanged.RemoveAll(this);

      if (ATATPlayerState* ps = pc->GetTATPlayerState())
      {
         ps->OnCharacterSaveIdChanged.RemoveAll(this);
      }

      if (AOSECharacterBase* character = pc->GetPawn<AOSECharacterBase>())
      {
         character->OnCharacterReady.RemoveAll(this);
      }
   }
}

void UTATUserWidget::NativePreConstruct()
{
   Super::NativePreConstruct();

   const bool isDesignTime = IsDesignTime();
   if (isDesignTime)
   {
      RefreshWidgetEditorPreview();
   }
}

FReply UTATUserWidget::NativeOnKeyDown(const FGeometry& inGeometry, const FKeyEvent& inKeyEvent)
{
   // Don't trigger navigation hooks for auto-repeat input event
   if (LIKELY(FSlateApplication::IsInitialized()) && !inKeyEvent.IsRepeat())
   {
      const FTATNavigationConfig navConfig = FTATNavigationConfig::GetInstance().Get();

      const EUINavigation navigationDirectionFromKey = FSlateApplication::Get().GetNavigationDirectionFromKey(inKeyEvent);
      const EUINavigationAction navigationAction = FSlateApplication::Get().GetNavigationActionFromKey(inKeyEvent);
      const EUIExtendedNavigationAction extendedNavigationAction = navConfig.GetExtendedNavigationActionFromKey(inKeyEvent, this);

      // Allow our navigation hooks to handle valid navigation input first
      if (navigationDirectionFromKey != EUINavigation::Invalid)
      {
         return OnNavigationDirection(navigationDirectionFromKey).NativeReply;
      }
      if (navigationAction != EUINavigationAction::Invalid)
      {
         return OnNavigationAction(navigationAction).NativeReply;
      }
      if (extendedNavigationAction != EUIExtendedNavigationAction::Invalid)
      {
         return OnExtendedNavigationAction(extendedNavigationAction).NativeReply;
      }
   }

   return Super::NativeOnKeyDown(inGeometry, inKeyEvent);
}

FReply UTATUserWidget::NativeOnKeyUp(const FGeometry& inGeometry, const FKeyEvent& inKeyEvent)
{
   if (LIKELY(FSlateApplication::IsInitialized()))
   {
      const FTATNavigationConfig navConfig = FTATNavigationConfig::GetInstance().Get();

      const EUINavigation navigationDirectionFromKey = FSlateApplication::Get().GetNavigationDirectionFromKey(inKeyEvent);
      const EUINavigationAction navigationAction = FSlateApplication::Get().GetNavigationActionFromKey(inKeyEvent);
      const EUIExtendedNavigationAction extendedNavigationAction = navConfig.GetExtendedNavigationActionFromKey(inKeyEvent, this);

      // Allow our navigation hooks to handle valid navigation input first
      if (navigationDirectionFromKey != EUINavigation::Invalid)
      {
         return OnNavigationDirectionReleased(navigationDirectionFromKey).NativeReply;
      }
      if (navigationAction != EUINavigationAction::Invalid)
      {
         return OnNavigationActionReleased(navigationAction).NativeReply;
      }
      if (extendedNavigationAction != EUIExtendedNavigationAction::Invalid)
      {
         return OnExtendedNavigationActionReleased(extendedNavigationAction).NativeReply;
      }
   }

   return Super::NativeOnKeyUp(inGeometry, inKeyEvent);
}

UTATScreenWidget* UTATUserWidget::TryFindParentTATScreenWidget() const
{
   UWidget* parent = GetParent();
   if (UWidgetTree* tree = Cast<UWidgetTree>(GetOuter()))
   {
      parent = Cast<UWidget>(tree->GetOuter());
   }

   while (parent)
   {
      if (UTATScreenWidget* tatScreenParent = Cast<UTATScreenWidget>(parent))
      {
         return tatScreenParent;
      }

      if (UWidget* newParent = parent->GetParent())
      {
         parent = newParent;
      }
      else
      {
         if (UWidgetTree* tree = Cast<UWidgetTree>(parent->GetOuter()))
            parent = Cast<UWidget>(tree->GetOuter());
         else
            parent = nullptr;
      }
   }
   return nullptr;
}

void UTATUserWidget::_OnGameStateFound_Implementation(ATATGameState* gameState)
{
   // bp stub
}

void UTATUserWidget::_OnPlayerStateAdded_Implementation(ATATPlayerState* ps)
{
   // bp stub
}

void UTATUserWidget::_OnPlayerStateRemoved_Implementation(ATATPlayerState* ps)
{
   // bp stub
}

void UTATUserWidget::_OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps)
{
   // bp stub
}

void UTATUserWidget::_OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character)
{
   // bp stub
}

void UTATUserWidget::_OnLocalCharacterSaveIdChanged_Implementation(FTATCharacterSaveId saveId)
{
   // bp stub
}

void UTATUserWidget::_OnGameStateSetEvent(AGameStateBase* gameState)
{
   _InitGameStateBindings(gameState);
}

void UTATUserWidget::_OnPlayerStateIsAdded(AOSEPlayerState* osePS)
{
   _OnPlayerStateAdded(CastChecked<ATATPlayerState>(osePS));
}

void UTATUserWidget::_OnPlayerStateIsRemoved(AOSEPlayerState* osePS)
{
   _OnPlayerStateRemoved(CastChecked<ATATPlayerState>(osePS));
}

void UTATUserWidget::_OnLocalPlayerStateChanged(APlayerState* ps)
{
   _HandleLocalPlayerState(CastChecked<ATATPlayerState>(ps));
}

void UTATUserWidget::_HandleCharacterSaveIdChanged(ATATPlayerState* ps, FTATCharacterSaveId saveId)
{
   _OnLocalCharacterSaveIdChanged(saveId);
}

void UTATUserWidget::_OnLocalPawnChanged(APawn* newPawn)
{
   if (AOSECharacterBase* character = Cast<AOSECharacterBase>(newPawn))
   {
      if (character->IsCharacterReady())
      {
         _OnLocalCharacterIsReady(character);
      }
      else
      {
         character->OnCharacterReady.AddUniqueDynamic(this, &UTATUserWidget::_OnLocalCharacterIsReadyChanged);
      }
   }
}

void UTATUserWidget::_OnLocalCharacterIsReadyChanged(AOSECharacterBase* character)
{
   _OnLocalCharacterIsReady(character);
}

void UTATUserWidget::_OnWidgetAddedToViewport(UWidget* addedWidget, ULocalPlayer* localPlayer)
{
   // Annoyingly, when a widget is added to the viewport, its child widgets are not passed to this callback, 
   // nor does IsInViewport() return true for them (due to management of bIsManagedByGameViewportSubsystem).
   //
   // So we have to check whether we're a child of the added widget. However, IsChildOf() doesn't go all the 
   // way up to the screen level (stops at highest parent, rather than climbing further up the widget tree). 
   // 
   // So TryFindParentTATScreenWidget() is necessary too, since screens are generally what we add to the viewport.
   if (addedWidget == this || IsChildOf(addedWidget) || addedWidget == TryFindParentTATScreenWidget())
   {
      _OnAddedToViewport();

      UWorld* world = GetWorld();
      check(world);
      UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(world);
      check(viewportSubsystem);
      viewportSubsystem->OnWidgetAdded.Remove(_onWidgetAddedHandle);
   }
}

void UTATUserWidget::_HandleLocalPlayerState(ATATPlayerState* ps)
{
   if (ReceiveOnLocalPlayerStateAdded)
   {
      _OnLocalPlayerStateAdded(ps);
   }

   if (ReceiveOnLocalCharacterSaveIdChanged)
   {
      if (ps->GetCharacterSaveId().IsValid())
      {
         _OnLocalCharacterSaveIdChanged(ps->GetCharacterSaveId());
      }

      ps->OnCharacterSaveIdChanged.AddUniqueDynamic(this, &UTATUserWidget::_HandleCharacterSaveIdChanged);
   }
}

void UTATUserWidget::_InitGameStateBindings(AGameStateBase* gameState)
{
   ATATGameState* tatGS = CastChecked<ATATGameState>(gameState);

   if (ReceiveOnPlayerStateAdded)
   {
      tatGS->OnPlayerStateAdded.AddUniqueDynamic(this, &UTATUserWidget::_OnPlayerStateIsAdded);

      // send an initial add for everyone already in the player array
      for (APlayerState* ps : tatGS->PlayerArray)
      {
         _OnPlayerStateIsAdded(CastChecked<AOSEPlayerState>(ps));
      }
   }

   if (ReceiveOnPlayerStateRemoved)
   {
      tatGS->OnPlayerStateRemoved.AddUniqueDynamic(this, &UTATUserWidget::_OnPlayerStateIsRemoved);
   }

   if (ReceiveOnGameStateFound)
   {
      _OnGameStateFound(tatGS);
   }
}

void UTATUserWidget::_ClearBindings(AGameStateBase* gameState)
{
   ATATGameState* tatGS = CastChecked<ATATGameState>(gameState);
   tatGS->OnPlayerStateAdded.RemoveAll(this);
   tatGS->OnPlayerStateRemoved.RemoveAll(this);
}

bool UTATUserWidget::_ReceivesAnyGameStateEvent() const
{
   return (ReceiveOnGameStateFound || ReceiveOnPlayerStateAdded || ReceiveOnPlayerStateRemoved);
}

void UTATUserWidget::SetVisibility(ESlateVisibility InVisibility)
{
   if (_bHidden)
   {
      _hiddenVisibility = InVisibility;
      Super::SetVisibility(ESlateVisibility::Hidden);
   }
   else
   {
      Super::SetVisibility(InVisibility);
   }
}

void UTATUserWidget::ForceHidden(const bool bHide)
{
   if (_HideInDemoMode)
   {
      const auto newVis = _bHidden ? _hiddenVisibility : GetVisibility();
      _bHidden = bHide;
      SetVisibility(newVis);
   }
}
