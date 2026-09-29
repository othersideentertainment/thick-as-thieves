// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSERadialWidget.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"
#include "Input/OSEInputFunctionLibrary.h"
#include "Player/OSEPlayerController.h"
#include "UI/OSERadialWidgetItem.h"
#include "UI/OSERadialBackground.h"

// ue4
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERadialWidget)

DEFINE_LOG_CATEGORY_STATIC(LogOSERadialWidget, Log, All);

//---------------------------------------------------------------------------------------
// FOSERadialItemInfo
//---------------------------------------------------------------------------------------

void FOSERadialItemInfo::UpdateInPlace(const FOSERadialItemInfo& newInfo, bool allowUpdateWeight)
{
   const int32 oldWeight = Weight;
   const FFloatInterval oldArcAngle = _radialAngleArcDegrees;
   *this = newInfo;
   if (!allowUpdateWeight)
   {
      Weight = oldWeight;
   }
   _radialAngleArcDegrees = oldArcAngle;
}

//---------------------------------------------------------------------------------------
// UOSERadialWidget
//---------------------------------------------------------------------------------------

UOSERadialWidget::UOSERadialWidget()
   : Super()
{
   WidgetSize = FVector2D(512.0f, 512.0f);
   WidgetItemSize = FVector2D(128.0f, 128.0f);
}

void UOSERadialWidget::NativeConstruct()
{
   Super::NativeConstruct();

   // bind to our input actions
   if (UEnhancedInputComponent* enhancedInputComponent = _GetInputComponent())
   {
      enhancedInputComponent->BindAction(Settings->RadialSettings.DirectionalInput, ETriggerEvent::Triggered, this, &UOSERadialWidget::_OnDirectionalInput);
      enhancedInputComponent->BindAction(Settings->RadialSettings.CancelInput, ETriggerEvent::Triggered, this, &UOSERadialWidget::_OnCancelInput);

      // bind to all optional input actions
      for (int32 i = 0; i < static_cast<int32>(EOSERadialActionType::MAX); i++)
      {
         if (UInputAction* inputAction = Settings->RadialSettings.RadialActionInputs[i])
         {
            const EOSERadialActionType actionType = static_cast<EOSERadialActionType>(i);
            enhancedInputComponent->BindAction(inputAction, ETriggerEvent::Triggered, this, &UOSERadialWidget::_FireRadialActionWithType, actionType);
         }
      }
   }
}

void UOSERadialWidget::NativeDestruct()
{
   Super::NativeDestruct();
}

void UOSERadialWidget::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);
   
   if (_isEnabled)
   {
      _TickRestrictMouseMovement();
      _TickUpdateHoveredItems();
   }
}

void UOSERadialWidget::_FireRadialActionWithType(const FInputActionValue& inputActionValue, EOSERadialActionType actionType)
{
   if (const FOSERadialItemInfo* itemInfo = _TryGetRadialItemInfo(_currentHoveredIndex))
   {
      _OnRadialAction(actionType, inputActionValue, true, _currentHoveredIndex, *itemInfo);
   }
   else
   {
      _OnRadialAction(actionType, inputActionValue, false, INDEX_NONE, FOSERadialItemInfo());
   }
}

#if WITH_EDITOR
EDataValidationResult UOSERadialWidget::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);
   if (!Settings)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Radial widget %s is missing it's radial settings asset!"), *GetName())));
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void UOSERadialWidget::BuildRadialMenu(const FOSERadialWidgetBuildParams& params)
{
   check(Settings); // required by data validation so we should be able to assume this exists.
   _params = params;

   // Update total item weight for the new set of items
   _totalItemWeight = 0;
   for (const FOSERadialItemInfo& item : _params.Items)
   {
      _totalItemWeight += item.GetWeight();
   }

   // Cache the starting angle for each item
   const float angleIncrementDegrees = 360.0f / static_cast<float>(_totalItemWeight);
   float lastItemEndAngleDegrees = RadialOffsetDegrees;
   auto calcRadialItemAngle = [&lastItemEndAngleDegrees, angleIncrementDegrees](FOSERadialItemInfo& item)
   {
      item._radialAngleArcDegrees = FFloatInterval(
         lastItemEndAngleDegrees,
         lastItemEndAngleDegrees + (angleIncrementDegrees * static_cast<float>(item.GetWeight())));
      lastItemEndAngleDegrees = item._radialAngleArcDegrees.Max;
   };
   if (ReverseRadialItemOrder)
   {
      for (int32 i = _params.Items.Num() - 1; i >= 0; --i)
      {
         calcRadialItemAngle(_params.Items[i]);
      }
   }
   else
   {
      for (FOSERadialItemInfo& item : _params.Items)
      {
         calcRadialItemAngle(item);
      }
   }

   _ClearRadialWidget();
   _BuildRadialWidget();
}

void UOSERadialWidget::CancelRadialMenu()
{
   if (!_isEnabled)
   {
      return;
   }

   // set a flag so other systems can check to see if we were cancelled
   _MarkRadialMenuCanceled();

   // let's assume the blueprint will handle this and disable/hide the radial menu if it supports that
   _OnCanceled();
}

AOSEPlayerController* UOSERadialWidget::_GetPlayerController() const
{
   return Cast<AOSEPlayerController>(GetOwningPlayer());
}

UEnhancedInputComponent* UOSERadialWidget::_GetInputComponent() const
{
   if (AOSEPlayerController* pc = _GetPlayerController())
   {
      return Cast<UEnhancedInputComponent>(pc->InputComponent);
   }
   return nullptr;
}

UEnhancedPlayerInput* UOSERadialWidget::_GetPlayerInput() const
{
   if (AOSEPlayerController* pc = _GetPlayerController())
   {
      if (UEnhancedInputLocalPlayerSubsystem* enhancedInputSubsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
      {
         return enhancedInputSubsys->GetPlayerInput();
      }
   }
   return nullptr;
}

void UOSERadialWidget::_RadialItemToBackgroundSlice(const FOSERadialItemInfo& itemInfo, FOSERadialSlice& bgItem) const
{
   check(_params.RadialBackground != nullptr);

   // Shrink the background item's arc range for visual purposes
   // (we don't apply this offset to _radialAngleArcDegrees to avoid dead-zones between items in the radial wheel)
   FFloatInterval bgItemArcRangeDegrees = itemInfo._radialAngleArcDegrees;
   if (RadialItemSizeAdjustDegrees != 0)
   {
      const float halfAdjustAngle = RadialItemSizeAdjustDegrees * 0.5f;
      bgItemArcRangeDegrees.Min += halfAdjustAngle;
      bgItemArcRangeDegrees.Max -= halfAdjustAngle;
   }

   _params.RadialBackground->GetItemTemplate(itemInfo.BackgroundItemTemplate, bgItem);
   bgItem.IsHidden = itemInfo.IsHidden;
   bgItem.IsDisabled = itemInfo.IsDisabled;
   bgItem.Background.ArcRangeDegrees = bgItemArcRangeDegrees;
}

void UOSERadialWidget::_ClearRadialWidget()
{
   for(UOSERadialWidgetItem* item : _runtimeWidgetItems)
   {
      if (_params.CanvasPanel)
      {
         _params.CanvasPanel->RemoveChild(item);
      }
   }
   _runtimeWidgetItems.Reset();

   if (_params.RadialBackground != nullptr)
   {
      _params.RadialBackground->ClearBackgroundItems();
   }
}

void UOSERadialWidget::_BuildRadialWidget()
{
   TArray<FOSERadialSlice> backgroundItems;
   const bool setupBackgroundItems = _params.RadialBackground != nullptr;

   for (int32 idx = 0; idx < _params.Items.Num(); ++idx)
   {
      const FOSERadialItemInfo& itemInfo = _params.Items[idx];

      // let the background widget know about the item
      if (setupBackgroundItems)
      {
         FOSERadialSlice& bgItem = backgroundItems.AddDefaulted_GetRef();
         _RadialItemToBackgroundSlice(itemInfo, bgItem);
         const FFloatInterval bgItemArcRangeDegrees = bgItem.Background.ArcRangeDegrees;

         // Blueprint callback to allow customizing the background item
         _OnRadialBackgroundItemCreated(itemInfo, bgItem, idx, itemInfo._radialAngleArcDegrees);

         // Re-apply ArcRangeDegrees to bgItem.
         // A blueprint handling _OnRadialBackgroundItemCreated may want to overwrite the whole bgItem value (eg. for a certain type of item, use a different style
         // entirely), and in that case we still want to maintain the correct angle for the item. Theoretically a blueprint could just set this value itself - it is
         // passed in as a callback argument - but in practice it's really annoying to set a struct field inside another struct field in an on-ref param.
         bgItem.Background.ArcRangeDegrees = bgItemArcRangeDegrees;
      }

      // Don't create widgets for hidden items
      if (itemInfo.IsHidden)
      {
         continue;
      }

      // If we don't have a widget class, don't create per-slice widgets
      if (!WidgetItemClass)
      {
         continue;
      }

      // create the widget
      if (UOSERadialWidgetItem* widget = CreateWidget<UOSERadialWidgetItem>(GetWorld(), WidgetItemClass))
      {
         // keep it alive w/ us
         _runtimeWidgetItems.Add(widget);

         // place it in the correct location
         const float width = WidgetItemSize.X / WidthOffset;
         const FVector2D widthDirection = FVector2D(width, 0.0f);

         const float itemCenterDegrees = UOSERadialPaintLibrary::LerpArcAngleDegrees(itemInfo._radialAngleArcDegrees, 0.5f);
         const FVector2D rotatedVec = UKismetMathLibrary::GetRotated2D(widthDirection, itemCenterDegrees);
         const FVector2D offsetFromCenter = rotatedVec + (WidgetSize / 2.0f);
         const FVector2D offsetFromCenterAdjusted = offsetFromCenter - (WidgetItemSize / 2.0f);

         // add to the canvas panel
         if (_params.CanvasPanel)
         {
            _params.CanvasPanel->AddChild(widget);
         }

         if (UCanvasPanelSlot* canvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(widget))
         {
            canvasPanelSlot->SetAnchors(FAnchors(0.0f));
            canvasPanelSlot->SetSize(WidgetItemSize);
            canvasPanelSlot->SetPosition(offsetFromCenterAdjusted);
         }

         // let the item know to set itself up
         widget->InitRadialItem(itemInfo, idx, itemInfo._radialAngleArcDegrees);

         // let our blueprint subclass know we made an item
         _OnRadialItemCreated(itemInfo, widget, idx, itemInfo._radialAngleArcDegrees);
      }
   }

   if (setupBackgroundItems)
   {
      _params.RadialBackground->SetBackgroundItems(backgroundItems);
   }
}

void UOSERadialWidget::_MarkRadialMenuCanceled()
{
   if (!_isEnabled)
   {
      return;
   }
   _wasCanceled = true;
}

void UOSERadialWidget::_SetRadialMenuInputsEnabled(bool enabled)
{
   if (enabled != _isEnabled)
   {
      _OnAboutToChangeEnabled(_isEnabled, enabled);

      _isEnabled = enabled;

      if (AOSEPlayerController* pc = _GetPlayerController())
      {
         if (Settings->RadialSettings.IgnoreLookInputWhileEnabled)
         {
            pc->SetIgnoreLookInput(_isEnabled);
         }

         if (Settings->RadialSettings.ResetMousePositionOnEnable && _isEnabled)
         {
            // reset the cursor to the center of the screen
            int32 sizeX, sizeY;
            pc->GetViewportSize(sizeX, sizeY);
            const FVector2D viewportSize = FVector2D(sizeX, sizeY);
            const FVector2D viewportCenter = viewportSize / 2.0f;
            pc->SetMouseLocation(viewportCenter.X, viewportCenter.Y);
         }

         // disable GAS input while this is up
         // locally disable GAS while UI is up
         pc->SetIgnoreAbilityInput(_isEnabled);
      }

      // reset whenever we re-enable
      if (_isEnabled)
      {
         _wasCanceled = false;
         _lastGoodMousePos = FVector2D::ZeroVector;
         _lastGoodGamepadStickDir = FVector2D::ZeroVector;
         _previousGamepadDistanceFromCenter = 0.0f;
         _wasLastInputValid = false;
         _SetCurrentHoveredIndex(INDEX_NONE);
      }

      _OnEnabledChanged(_isEnabled);

      // Fire event so blueprints can handle the case of a radial item being selected while it's closed but not canceled
      if (!_isEnabled && !_wasCanceled)
      {
         if (const FOSERadialItemInfo* itemInfo = _TryGetRadialItemInfo(_currentHoveredIndex))
         {
            _OnRadialSelectAndRelease(_currentHoveredIndex, *itemInfo);
         }
      }
   }
}

bool UOSERadialWidget::_UpdateRadialItem(int32 index, const FOSERadialItemInfo& newInfo)
{
   if (!_params.Items.IsValidIndex(index))
   {
      return false;
   }

   // Update our own copy of the item
   constexpr bool updateWeight = false;
   _params.Items[index].UpdateInPlace(newInfo, updateWeight);

   // Update the item widget's copy
   if (_runtimeWidgetItems.IsValidIndex(index) && _runtimeWidgetItems[index] != nullptr)
   {
      _runtimeWidgetItems[index]->UpdateRadialItem(newInfo);
   }

   // Update the background slice representing this item
   if (_params.RadialBackground != nullptr)
   {
      FOSERadialSlice slice;
      if (_params.RadialBackground->GetBackgroundItem(index, slice))
      {
         const FFloatInterval origArcRangeDegrees = slice.Background.ArcRangeDegrees;
         _RadialItemToBackgroundSlice(_params.Items[index], slice);
         slice.Background.ArcRangeDegrees = origArcRangeDegrees;
         _params.RadialBackground->SetBackgroundItem(index, slice);
      }
   }
   return true;
}

void UOSERadialWidget::GetCurrentItemInfo(bool& found, FOSERadialItemInfo& itemInfo) const
{
   found = false;
   if (_runtimeWidgetItems.IsValidIndex(_currentHoveredIndex))
   {
      UOSERadialWidgetItem* widget = _runtimeWidgetItems[_currentHoveredIndex];
      itemInfo = widget->GetRadialItemInfo();
      found = true;
   }
}

void UOSERadialWidget::_TickUpdateHoveredItems()
{
   AOSEPlayerController* pc = _GetPlayerController();
   if (!pc)
      return;

   float rotDeg = 0.0f;
   bool isInputValid = false;
   switch(pc->GetCurrentInputHardwareType())
   {
   case EOSEInputHardwareType::Gamepad:
      {
         isInputValid = _GetRotationFromGamepadThumbstick(rotDeg);
      }
      break;
   case EOSEInputHardwareType::KeyboardMouse:
      {
         isInputValid = _GetRotationFromMousePosition(rotDeg);
      }
      break;
   }

   if (_params.CursorImage)
   {
      _params.CursorImage->SetRenderTransformAngle(rotDeg);
      _params.CursorImage->SetVisibility(isInputValid ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
   }

   if (_params.CursorWidget)
   {
      _params.CursorWidget->SetRenderTransformAngle(rotDeg);
      _params.CursorWidget->SetVisibility(isInputValid ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
   }

   // Force the selected background item to be hovered.
   // We do this because the radial item slice can be slightly smaller than the actual item slice (to add visual separation between items),
   // but we don't want to add a small deadzone between slices where nothing is actually selected.
   constexpr bool forceSelectBackgroundIndex = true;

   // only update the current index when the input is valid
   if (isInputValid)
   {
      const float positiveRotDeg = FMath::UnwindDegrees(rotDeg - 90.0f);
      const int32 selectedItemIndex = _AngleToItemIndex(positiveRotDeg);
      _SetCurrentHoveredIndex(selectedItemIndex);

      if (_params.RadialBackground != nullptr)
      {
         _params.RadialBackground->UpdateHoverState(true, positiveRotDeg, forceSelectBackgroundIndex, selectedItemIndex);
      }
   }
   else
   {
      constexpr int32 selectedItemIndex = INDEX_NONE;
      _SetCurrentHoveredIndex(selectedItemIndex);

      if (_params.RadialBackground != nullptr)
      {
         _params.RadialBackground->UpdateHoverState(false, 0.0f, forceSelectBackgroundIndex, selectedItemIndex);
      }
   }

   // for next frame
   _wasLastInputValid = isInputValid;
}

void UOSERadialWidget::_TickRestrictMouseMovement()
{
   if (!Settings->RadialSettings.RestrictMousePositionWhileEnabled)
      return;

   AOSEPlayerController* pc = _GetPlayerController();
   if (!pc)
      return;

   const float maxDistanceFromViewportCenterSquared = FMath::Square(Settings->RadialSettings.RestrictMousePositionDistanceFromCenter);

   // keep the cursor somewhat locked to the center of the screen
   int32 sizeX, sizeY;
   pc->GetViewportSize(sizeX, sizeY);
   const FVector2D viewportSize = FVector2D(sizeX, sizeY);
   const FVector2D viewportCenter = viewportSize / 2.0f;
   float mouseX, mouseY;
   if(pc->GetMousePosition(mouseX, mouseY))
   {
      const FVector2D mousePos = FVector2D(mouseX, mouseY);
      const float distFromCenterSquared = FMath::Abs(FVector2D::DistSquared(mousePos, viewportCenter));
      const bool isValidPos = distFromCenterSquared <= maxDistanceFromViewportCenterSquared;
      if (isValidPos)
      {
         _lastGoodMousePos = FVector2D(mouseX, mouseY);
      }
      else
      {
         // jump back between the center and our last good pos to give ourselves some room to breath again
         if (_lastGoodMousePos.X > 0 && _lastGoodMousePos.Y > 0)
         {
            const FVector2D pointDirection = (_lastGoodMousePos - viewportCenter).GetSafeNormal();
            const float newOffsetFromCenter = Settings->RadialSettings.RestrictMousePositionDistanceFromCenter * 0.25f;
            const FVector2D newMousePosition = viewportCenter + (pointDirection * newOffsetFromCenter);
            pc->SetMouseLocation(newMousePosition.X, newMousePosition.Y);
         }
         else
         {
            pc->SetMouseLocation(viewportCenter.X, viewportCenter.Y);
         }
      }
   }
}

void UOSERadialWidget::_OnDirectionalInput(const FInputActionValue& value)
{
   // we can just query this directly as needed, but I'm leaving this here because it's handy for debugging!
   if (_isEnabled)
   {

   }
}

void UOSERadialWidget::_OnCancelInput(const FInputActionValue& value)
{
   CancelRadialMenu();
}

bool UOSERadialWidget::_GetRotationFromMousePosition(float& rotDeg)
{
   const FVector2D mousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
   const FGeometry geom = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
   const FVector2D viewportSize = geom.GetLocalSize();
   const FVector2D viewportCenter = viewportSize / 2.0f;
   
   FVector2D mouseDir = (mousePos - viewportCenter);
   mouseDir.Normalize();

   const FVector2D viewportUp = FVector2D(viewportCenter.X, 0.0f);
   FVector2D viewportUpDir = viewportUp - viewportCenter;
   viewportUpDir.Normalize();

   const bool isRHS = (mousePos.X - viewportCenter.X) >= 0.0f;
   const float multiplier = isRHS ? 1.0f : -1.0f;
   const float dot = mouseDir | viewportUpDir;
   rotDeg = UKismetMathLibrary::DegAcos(dot) * multiplier;
   return true;
}

bool UOSERadialWidget::_GetRotationFromGamepadThumbstick(float& rotDeg)
{
   UEnhancedPlayerInput* playerInput = _GetPlayerInput();
   if (!playerInput)
      return false;

   UInputAction* inputAction = Settings->RadialSettings.DirectionalInput;
   check(inputAction);

   FInputActionValue value = playerInput->GetActionValue(inputAction);

   // Stick values are:
   //   -1
   // -1 0 1
   //    1
   float stickX = value[0];
   float stickY = value[1];

   FVector2D stickDir = FVector2D(stickX, stickY);
   stickDir.Normalize();

   // We are within the deadzone, don't do anything with this value
   if (stickDir.IsNearlyZero())
   {
      return false;
   }

   float distFromCenter = FVector2D::Distance(FVector2D(0.0f, 0.0f), stickDir.GetAbs());
   float deltaDistance = distFromCenter - _previousGamepadDistanceFromCenter;

   UE_LOG(LogOSERadialWidget, Verbose, TEXT("Raw %.02f %.02f, Distance From Center = %.02f, Delta Distance = %.02f"), stickX, stickY, distFromCenter, deltaDistance);

   // If the stick is retracting, use the last good position we had to keep our index solid
   static const float kTolerance = 0.01f;
   _previousGamepadDistanceFromCenter = distFromCenter; // for next frame
   if (deltaDistance < 0.0f && !FMath::IsNearlyEqual(deltaDistance, 0.0f, kTolerance) && distFromCenter >= Settings->RadialSettings.GamepadAnalogStickRetractionDeadzone)
   {
      UE_LOG(LogOSERadialWidget, Verbose, TEXT("Previous distance = %.02f, Current distance = %.02f, Delta distance = %.02f"), _previousGamepadDistanceFromCenter, distFromCenter, deltaDistance);
      stickDir = _lastGoodGamepadStickDir;
   }

   const FVector2D stickUpDir = FVector2D(0.0f, -1.0f);
   const bool isRHS = (stickDir.X) >= 0.0f;
   const float multiplier = isRHS ? 1.0f : -1.0f;
   const float dot = stickDir | stickUpDir;      
   rotDeg = UKismetMathLibrary::DegAcos(dot) * multiplier;
   _lastGoodGamepadStickDir = stickDir; // for next frame
   return true;
}

void UOSERadialWidget::_SetCurrentHoveredIndex(int index)
{
   // only broadcast changes when they actually happen to reduce WBP response work
   if (index != _currentHoveredIndex)
   {
      UE_LOG(LogOSERadialWidget, Verbose, TEXT("Old index = %d, new index = %d"), _currentHoveredIndex, index);

      if (_runtimeWidgetItems.IsValidIndex(_currentHoveredIndex))
      {
         UOSERadialWidgetItem* widget = _runtimeWidgetItems[_currentHoveredIndex];
         widget->SetHovered(false);
      }

      _currentHoveredIndex = index;

      if (_runtimeWidgetItems.IsValidIndex(_currentHoveredIndex))
      {
         UOSERadialWidgetItem* widget = _runtimeWidgetItems[_currentHoveredIndex];
         widget->SetHovered(true);
         _OnHoveredItemChanged(widget->GetRadialItemInfo(), widget, _currentHoveredIndex);
      }
      else
      {
         _OnHoveredItemChanged(FOSERadialItemInfo(), nullptr, INDEX_NONE);
      }
   }
}

int32 UOSERadialWidget::_AngleToItemIndex(float angleDeg) const
{
   if(_params.Items.Num() == 1)
   {
      return 0;
   }
   for (int32 idx = 0; idx < _params.Items.Num(); ++idx)
   {
      if (UOSERadialPaintLibrary::AngleContainedInRadialArc(angleDeg, _params.Items[idx]._radialAngleArcDegrees))
      {
         return idx;
      }
   }
   return INDEX_NONE;
}

const FOSERadialItemInfo* UOSERadialWidget::_TryGetRadialItemInfo(int32 index) const
{
   if (_runtimeWidgetItems.IsValidIndex(index))
   {
      if (UOSERadialWidgetItem* widget = _runtimeWidgetItems[_currentHoveredIndex])
      {
         return &widget->GetRadialItemInfo();
      }
   }
   return nullptr;
}

//---------------------------------------------------------------------------------------
// UOSERadialSettingsAsset
//---------------------------------------------------------------------------------------

#if WITH_EDITOR
EDataValidationResult UOSERadialSettingsAsset::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);
   if (!RadialSettings.DirectionalInput)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Radial settings %s requires a directional input action!"), *GetName())));
   }
   if (!RadialSettings.CancelInput)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Radial settings %s requires a cancel input action!"), *GetName())));
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

