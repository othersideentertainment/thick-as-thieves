// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATHUD.h"

// tat
#include "Player/TATPlayerController.h"
#include "SaveGame/TATSaveGame.h"
#include "UI/TATHUDIndicatorSubsystem.h"

// ose
#include "UI/OSERadialWidget.h"
#include "UI/OSEAnimatedSwitcher.h"
#include "UI/OSEUIFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue4
#include "Blueprint/UserWidget.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif // WITH_EDITOR
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHUD)

namespace HUDHelpers
{
   struct FDebugTextHelper
   {
      AHUD* Hud = nullptr;
      FVector2D Position = { 10.0f, 10.0f };
      FVector2D LineSize = { 768.0f, 20.0f };

      explicit FDebugTextHelper(AHUD* hud, const FVector2D& position = FVector2D::ZeroVector) : Hud(hud), Position(position) {}

      void AddSection(const FString& label, const FLinearColor& fgColor = FLinearColor::White, const FLinearColor& bgColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.25f))
      {
         check(Hud != nullptr);
         static constexpr float sectionPadding = 5.0f;
         Position.Y += sectionPadding;
         Hud->DrawRect(bgColor, Position.X, Position.Y, LineSize.X, LineSize.Y);
         Hud->DrawLine(Position.X, Position.Y + LineSize.Y, Position.X + LineSize.X, Position.Y + LineSize.Y, fgColor);
         Hud->DrawText(label, fgColor, Position.X + (sectionPadding * 2.0f), Position.Y);
         Position.Y += LineSize.Y + sectionPadding;
      }

      void PrintLine(const FString& text, const FLinearColor& color = FLinearColor::White)
      {
         check(Hud != nullptr);
         Hud->DrawText(text, color, Position.X, Position.Y);
         Position.Y += LineSize.Y;
      }
   };
}

ATATHUD::ATATHUD()
   : Super()
{
   // default to bShowHUD's val
   _prevShowHUD = bShowHUD;
}

/* static */
ATATHUD* ATATHUD::TryGetLocalTATHUD(const UObject* contextObj)
{
   if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(contextObj))
      return pc->GetTATHUD();
   return nullptr;
}

void ATATHUD::BeginPlay()
{
   Super::BeginPlay();

   if (UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this))
   {
      // TODO: add/remove save data load/unload event bindings when they exist so we can clean up this 
      // state when a player profile signs out
      _SetHUDIconsVisible(saveGame->AreHUDIconsVisible());
   }

   // hide the HUD when we're simulating in editor, it shows up in a not-so-useful default state anyway
#if WITH_EDITOR
   if (UEditorEngine* editor = Cast<UEditorEngine>(GEngine))
   {
      if (editor->IsSimulateInEditorInProgress())
      {
         bShowHUD = false;
         _CheckForShowHudChanged();
      }
   }
#endif // WITH_EDITOR
}

void ATATHUD::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   for (UUserWidget* hudWidget : _hudLayers)
   {
      if (hudWidget)
      {
         hudWidget->RemoveFromParent();
      }
   }
   _hudLayers.Reset();

   Super::EndPlay(endPlayReason);
}

void ATATHUD::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   for (int i = 0; i < (int)ETATHUDLayers::Num; i++)
   {
      _hudLayers.Add(nullptr);
   }
}

void ATATHUD::PostRender()
{
   Super::PostRender();
   _CheckForShowHudChanged();
}

void ATATHUD::DrawHUD()
{
   Super::DrawHUD();

   // Draw very simple crosshair
   if (CrosshairTex != NULL)
   {
      // Center of canvas and half texture size
      const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
      const FVector2D HalfSize(CrosshairTex->GetSizeX() / 2, CrosshairTex->GetSizeY() / 2);
      const FVector2D DrawPos = Center - HalfSize;
      DrawTextureSimple(CrosshairTex, DrawPos.X, DrawPos.Y);
   }

#if OSE_CHEATS_ENABLED
   HUDHelpers::FDebugTextHelper debugText{ this, FVector2D{ 10.0f, 10.0f } };
   if (_debugDrawHUDIndicators)
   {
      if (APlayerController* pc = GetOwningPlayerController())
      {
         if (ULocalPlayer* localPlayer = pc->GetLocalPlayer())
         {
            if (UTATHUDIndicatorSubsystem* indicatorSubsystem = localPlayer->GetSubsystem<UTATHUDIndicatorSubsystem>())
            {
               debugText.AddSection(FString::Printf(TEXT("=== HUD Indicators for %s (%i) ==="),
                  *GetNameSafe(localPlayer),
                  indicatorSubsystem->NumIndicators()));

               const double now = GetWorld()->GetTimeSeconds();
               indicatorSubsystem->ForEachIndicator([&](int32 idx, const FTATHUDIndicatorInfo& info)
               {
                  debugText.PrintLine(FString::Printf(TEXT("[%i] Timer=(%.2f, Elapsed=%.2f), Progress=%.2f, Widget=%s, Handle=%s, Icon=%s"),
                     idx,
                     info.State.GetTimerNormalizedValue(now),
                     info.State.GetTimerElapsedSeconds(now),
                     info.State.Progress,
                     *GetNameSafe(info.Widget),
                     *info.EffectHandle.ToString(),
                     *GetNameSafe(info.State.Icon.Get())),
                     FLinearColor::Green);
               });
            }
         }
      }
   }
#endif
}

void ATATHUD::OnHUDVisibilityChanged_Implementation(UUserWidget* previousHUD, UUserWidget* newHUD)
{
   // bp stub
}

void ATATHUD::_CheckForShowHudChanged()
{
   if (_prevShowHUD != bShowHUD)
   {
      _UpdateHUDLayersVisibility();
      _prevShowHUD = bShowHUD;
   }
}

void ATATHUD::SetHUDLayerWidget(ETATHUDLayers layer, UUserWidget* widget)
{
   int index = (int)layer;
   if ((index < 0) || (index >= _hudLayers.Num()))
   {
      // Looks like this can happen based on order of EndPlay stuff.
      // IE - internal EndPlay clears out the layers, then EndPlay in
      // BP tries to send nullptr for a layer to clear it.
      return;
   }

   UUserWidget* previousHUD = _hudLayers[index];
   if (previousHUD  != nullptr)
   {
      previousHUD->RemoveFromParent();
   }

   if (widget)
   {
      widget->AddToViewport();
   }
   _hudLayers[index] = widget;

   _UpdateHUDLayersVisibility();
}

UUserWidget* ATATHUD::CreateWidgetForHUDIndicator_Implementation(const FTATHUDIndicatorState& indicatorState)
{
   return nullptr;
}

void ATATHUD::_UpdateHUDLayersVisibility()
{
   UUserWidget* previousHUD = _currentVisibleHUD;
   _currentVisibleHUD = nullptr;

   bool foundTopmost = false;
   for (int i = (int)ETATHUDLayers::Num - 1; i >= 0; i--)
   {
      if (_hudLayers[i] != nullptr)
      {
         if (!foundTopmost && bShowHUD)
         {
            foundTopmost = true;
            _currentVisibleHUD = _hudLayers[i];
         }
      }
   }

   if (_currentVisibleHUD != previousHUD)
   {
      OnHUDVisibilityChanged(previousHUD, _currentVisibleHUD);
   }
}

UUserWidget* ATATHUD::GetCurrentReticleWidget_Implementation() const
{
   return nullptr;
}

void ATATHUD::RegisterRadialMenu(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget)
{
   if (radialWidget == nullptr)
   {
      return;
   }

   _radialMenus.Add(radialMenuTag, radialWidget);

   // If this radial has an animated switcher parent widget, we can subscribe to switching events and handle them automatically
   UOSEAnimatedSwitcher* parentSwitcher = Cast<UOSEAnimatedSwitcher>(radialWidget->GetParent());
   if (parentSwitcher != nullptr && !_radialMenuParentSwitchers.Contains(parentSwitcher))
   {
      _radialMenuParentSwitchers.Add(parentSwitcher);
      parentSwitcher->OnActiveWidgetIndexChanged.AddDynamic(this, &ATATHUD::_OnRadialSwitcherChanged);
   }
}

UOSERadialWidget* ATATHUD::GetRadialMenu(FGameplayTag radialMenuTag) const
{
   return _radialMenus.Contains(radialMenuTag) ? _radialMenus[radialMenuTag] : nullptr;
}

bool ATATHUD::SetRadialMenuActive(FGameplayTag radialMenuTag, bool isActive)
{
   UOSERadialWidget** radialWidgetPtr = _radialMenus.Find(radialMenuTag);
   if (radialWidgetPtr == nullptr || *radialWidgetPtr == nullptr)
   {
      return false;
   }
   UOSERadialWidget* radialWidget = *radialWidgetPtr;
   check(radialWidget != nullptr);

   // Don't allow deactivating a radial menu that's not already active
   if (!isActive && !radialWidget->IsRadialMenuEnabled())
   {
      return false;
   }

   // Don't allow setting a new menu active if we're in the middle of switching widgets (eg. if the player tries activating multiple radial menus simultaneously)
   UOSEAnimatedSwitcher* animatedSwitcher = Cast<UOSEAnimatedSwitcher>(radialWidget->GetParent());
   if (animatedSwitcher != nullptr && (animatedSwitcher->IsTransitionPlaying() || animatedSwitcher->IsCurrentlySwitching()))
   {
      return false;
   }

   bool activateInstantly = false;

   // If we had no radials open and are about to open one, fire the callback so the HUD can fade in the widget switcher
   if (isActive && !IsAnyRadialMenuActive())
   {
      OnRadialMenuOpened();

      // Switch directly to this radial without transitioning from a previous one
      activateInstantly = true;
   }

   // Set this radial menu's state
   constexpr bool updateRadialSwitcher = true;
   _SetRadialMenuState(radialMenuTag, radialWidget, isActive, updateRadialSwitcher, activateInstantly);

   // If we're closing the radial and there are no other radials open, fire the callback so the HUD can fade out the widget switcher
   if (!isActive && !IsAnyRadialMenuActive())
   {
      OnRadialMenuClosed();
   }

   return true;
}

bool ATATHUD::CloseAllRadialMenus(bool cancelRadials)
{
   constexpr bool updateRadialSwitcher = false;
   constexpr bool radialActive = false;
   int32 numClosedRadials = 0;
   for (const auto& pair : _radialMenus)
   {
      if (pair.Value && pair.Value->IsRadialMenuEnabled())
      {
         ++numClosedRadials;
         if (cancelRadials)
         {
            pair.Value->CancelRadialMenu();
         }
         _SetRadialMenuState(pair.Key, pair.Value, radialActive, updateRadialSwitcher);
      }
   }

   const bool closedAnyRadials = numClosedRadials > 0;
   if (closedAnyRadials)
   {
      OnRadialMenuClosed();
   }

   return closedAnyRadials;
}

bool ATATHUD::IsRadialMenuActive(FGameplayTag radialMenuTag) const
{
   const UOSERadialWidget* const* radialWidgetPtr = _radialMenus.Find(radialMenuTag);
   if (radialWidgetPtr == nullptr || *radialWidgetPtr == nullptr)
   {
      return false;
   }
   const UOSERadialWidget* radialWidget = *radialWidgetPtr;
   check(radialWidget != nullptr);
   return radialWidget->IsRadialMenuEnabled();
}

bool ATATHUD::IsAnyRadialMenuActive() const
{
   for (const auto& pair : _radialMenus)
   {
      if (pair.Value != nullptr && pair.Value->IsRadialMenuEnabled())
      {
         return true;
      }
   }
   return false;
}

bool ATATHUD::GetCurrentRadialItem(FGameplayTag radialMenuTag, FOSERadialItemInfo& outRadialItem, bool& outWasCancelled) const
{
   const UOSERadialWidget* const* radialWidgetPtr = _radialMenus.Find(radialMenuTag);
   if (radialWidgetPtr == nullptr || *radialWidgetPtr == nullptr || !(*radialWidgetPtr)->IsRadialMenuEnabled())
   {
      outRadialItem = FOSERadialItemInfo{};
      outWasCancelled = false;
      return false;
   }
   const UOSERadialWidget* radialWidget = *radialWidgetPtr;
   check(radialWidget != nullptr);
   bool found = false;
   radialWidget->GetCurrentItemInfo(found, outRadialItem);
   outWasCancelled = radialWidget->WasCanceled();
   if (!found)
   {
      outRadialItem = FOSERadialItemInfo{};
      outWasCancelled = false;
   }
   return found;
}

void ATATHUD::_SetRadialMenuState(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget, bool isActive, bool updateRadialSwitcher, bool activateInstantly)
{
   check(radialWidget != nullptr && _radialMenus.Contains(radialMenuTag) && _radialMenus[radialMenuTag] == radialWidget);

   // If we're activating a radial and it has switcher parent widget, tell it to make this radial menu the active widget
   UWidgetSwitcher* switcher = Cast<UWidgetSwitcher>(radialWidget->GetParent());
   if (isActive && updateRadialSwitcher && switcher != nullptr)
   {
      // If the switcher is an animated switcher, skip the transition if requested
      UOSEAnimatedSwitcher* animatedSwitcher = Cast<UOSEAnimatedSwitcher>(switcher);
      const bool skipTransitionAnimation = activateInstantly && animatedSwitcher != nullptr;

      if (skipTransitionAnimation)
      {
         animatedSwitcher->SetTransitionAnimationEnabled(false);
      }

      switcher->SetActiveWidget(radialWidget);

      if (skipTransitionAnimation)
      {
         animatedSwitcher->SetTransitionAnimationEnabled(true);
      }
   }

   // Normally the widget switcher handles activation, but there are a few cases where a radial is shown or hidden that does not go through that codepath.
   // Since it's totally safe to double up on activate or deactivate calls, we'll also call that here to make sure widgets are set to the correct state.
   if (isActive)
   {
      UOSEUIFunctionLibrary::ActivatableWidgetInterface_Activate(radialWidget);
   }
   else
   {
      UOSEUIFunctionLibrary::ActivatableWidgetInterface_Deactivate(radialWidget);
   }

   // Fire blueprint events
   OnRadialMenuChanged(radialMenuTag, radialWidget, isActive);
   RadialMenuChanged.Broadcast(radialMenuTag, isActive);
}

// Callback fired when any UOSEAnimatedSwitcher widgets for registered radial menus change their displayed widget
void ATATHUD::_OnRadialSwitcherChanged(UWidget* prevWidget, int32 prevWidgetIndex, UWidget* activeWidget, int32 activeWidgetIndex)
{
   auto findRadialTag = [this](UWidget* widget) -> FGameplayTag
   {
      if (widget != nullptr)
      {
         for (const auto& pair : _radialMenus)
         {
            if (pair.Value == widget)
            {
               return pair.Key;
            }
         }
      }
      return FGameplayTag::EmptyTag;
   };

   // Don't update the animated witcher widget when updating state because that would cause an infinite loop (we're here because the switcher updated!)
   constexpr bool updateRadialSwitcher = false;

   const FGameplayTag prevWidgetTag = findRadialTag(prevWidget);
   if (prevWidgetTag.IsValid())
   {
      constexpr bool isActive = false;
      _SetRadialMenuState(prevWidgetTag, _radialMenus[prevWidgetTag], isActive, updateRadialSwitcher);
   }

   const FGameplayTag activeWidgetTag = findRadialTag(activeWidget);
   if (activeWidgetTag.IsValid())
   {
      constexpr bool isActive = true;
      _SetRadialMenuState(activeWidgetTag, _radialMenus[activeWidgetTag], isActive, updateRadialSwitcher);
   }
}

/* static */ void ATATHUD::SetHUDIconsVisible(UObject* contextObject, bool showIcons)
{
   check(contextObject);

   if (UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(contextObject))
   {
      saveGame->SetHUDIconsVisible(showIcons);
   }

   if (APlayerController* playerController = UGameplayStatics::GetPlayerController(contextObject->GetWorld(), 0))
   {
      if (ATATHUD* hud = Cast<ATATHUD>(playerController->GetHUD()))
      {
         hud->_SetHUDIconsVisible(showIcons);
      }
   }
}

void ATATHUD::_SetHUDIconsVisible(bool showIcons)
{
   if (_areHUDIconsVisible != showIcons)
   {
      _areHUDIconsVisible = showIcons;
      OnHUDIconsVisiblilityChanged.Broadcast(_areHUDIconsVisible);
   }
}


