// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "OSEUserWidget.h"
#include "UI/OSERadialPaintLibrary.h"

// ue4
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "OSERadialWidget.generated.h"

class AOSEPlayerController;
class UCanvasPanel;
class UEnhancedInputComponent;
class UEnhancedPlayerInput;
class UImage;
class UInputAction;
class UInputMappingContext;
class UPaperSprite;
class UOSERadialWidgetItem;
class UOSERadialSettingsAsset;
struct FInputActionValue;
class UOSERadialBackground;

//---------------------------------------------------------------------------------------
// FOSERadialItemInfo
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialItemInfo
{
   GENERATED_BODY()

public:
   // Each item in the wheel is represented by a gameplay tag; the expected usage here is for users of this generic system to
   // use this to look up into some kind of it's own data to figure out how to represent this info
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   FGameplayTag Tag;

   // An alternative/adjunct to the tag, for cases where unique tags are not available
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   UObject* Object = nullptr;

   // Generic user-defined metadata for this radial item
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   int32 Index = INDEX_NONE;

   // Name of the item template to use when setting up a OSERadialBackgroundWidget
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   FName BackgroundItemTemplate = NAME_None;

   // How many slots this item should take up (proportionally)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   int32 Weight = 1;

   // Should this item be grayed out and non-interactable?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   bool IsDisabled = false;

   // Should this item take up space but not be visible otherwise?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Item Info")
   bool IsHidden = false;

   // Cached item angle so we don't have to keep recalculating it (just used internally in UOSERadialWidget)
   FFloatInterval _radialAngleArcDegrees;

   // Get the clamped weight value (will never return a value less than one)
   FORCEINLINE int32 GetWeight() const { return FMath::Max(1, Weight); }

   // Replace params with a new set of params, maintaining the old arc angle and weight (unless allowUpdateWeight is false)
   void UpdateInPlace(const FOSERadialItemInfo& newInfo, bool allowUpdateWeight);
};

//---------------------------------------------------------------------------------------
// FOSERadialWidgetBuildParams
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialWidgetBuildParams
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite)
   UCanvasPanel* CanvasPanel = nullptr;
   UPROPERTY(BlueprintReadWrite)
   UOSERadialBackground* RadialBackground = nullptr;
   // TODO: Eventually remove CursorImage
   UPROPERTY(BlueprintReadWrite)
   UImage* CursorImage = nullptr;
   UPROPERTY(BlueprintReadWrite)
   UWidget* CursorWidget = nullptr;
   UPROPERTY(BlueprintReadWrite)
   TArray<FOSERadialItemInfo> Items;
};

//---------------------------------------------------------------------------------------
// EOSERadialActionType
//---------------------------------------------------------------------------------------

/// Custom input actions that can be fired while a radial wheel is active
UENUM(BlueprintType)
enum class EOSERadialActionType : uint8
{
   NextRadial UMETA(Tooltip = "Indicates that the next radial wheel in a sequence should be shown"),
   PrevRadial UMETA(Tooltip = "Indicates that the previous radial wheel in a sequence should be shown"),
   Action01 UMETA(Tooltip = "Custom primary input action fired while the radial wheel is active"),
   Action02 UMETA(Tooltip = "Custom secondary input action fired while the radial wheel is active"),
   Action03 UMETA(Tooltip = "Additional custom input action fired while the radial wheel is active"),
   Action04 UMETA(Tooltip = "Additional custom input action fired while the radial wheel is active"),
   MAX UMETA(Hidden)
};

//---------------------------------------------------------------------------------------
// FOSERadialSettings
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialSettings
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget")
   UInputAction* DirectionalInput = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget")
   UInputAction* CancelInput = nullptr;

   // Additional optional inputs to bind to radial actions (see UOSERadialWidget::_OnRadialAction)
   UPROPERTY(EditDefaultsOnly, Category = "Radial Widget", meta = (ArraySizeEnum = "/Script/OSECore.EOSERadialActionType"))
   UInputAction* RadialActionInputs[static_cast<uint32>(EOSERadialActionType::MAX)] = { nullptr };

   // Do we ignore look input while the radial menu is up?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget|Mouse And Keyboard")
   bool IgnoreLookInputWhileEnabled = true;

   // Do we reset the cursor to 0 when the radial menu is brought up?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget|Mouse And Keyboard")
   bool ResetMousePositionOnEnable = true;

   // Do we restrict the mouse position while the radial menu is up?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget|Mouse And Keyboard")
   bool RestrictMousePositionWhileEnabled = true;

   // How much do we restrict the mouse position while the radial menu is up?  May be useful to tune menus of different sizes.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget|Mouse And Keyboard")
   float RestrictMousePositionDistanceFromCenter = 100.0f;

   // When the analog stick is retracting, what size deadzone should we use to determine when to keep an item selected (in addition to the normal deadzone)?
   // A lower value is useful when you expect the player to always pick an item.
   // A higher value is useful when you want to make it easy for the player to let go of the analog stick to deselect all items.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget|Gamepad", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
   float GamepadAnalogStickRetractionDeadzone = 0.01f;
};

//---------------------------------------------------------------------------------------
// UOSERadialWidget
//---------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSERadialWidget : public UOSEUserWidget
{
   GENERATED_BODY()

public:
   UOSERadialWidget();

   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;

   UFUNCTION(BlueprintCallable)
   void BuildRadialMenu(const FOSERadialWidgetBuildParams& params);

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Radial Widget")
   void SetRadialMenuEnabled(bool enabled);
   void SetRadialMenuEnabled_Implementation(bool enabled) { _SetRadialMenuInputsEnabled(enabled); }

   UFUNCTION(BlueprintCallable, Category = "Radial Widget")
   void CancelRadialMenu();

   UFUNCTION(BlueprintPure, Category = "Radial Widget")
   bool IsRadialMenuEnabled() const { return _isEnabled; }

   UFUNCTION(BlueprintPure)
   void GetCurrentItemInfo(bool& found, FOSERadialItemInfo& itemInfo) const;

   UFUNCTION(BlueprintPure)
   bool WasCanceled() const { return _wasCanceled; }

   UFUNCTION(BlueprintCallable, Category = "Radial Widget")
   UCanvasPanel* GetRadialCanvasPanel() const { return _params.CanvasPanel; }

protected:

   // Play with this number to change the size of the radial menu
   UPROPERTY(EditDefaultsOnly, Category = "Radial Widget", meta = (ClampMin = "0.01", UIMin = "0.01"))
   float WidthOffset = 1.0;

   // Which items do we spawn into the menu
   UPROPERTY(EditDefaultsOnly, Category = "Radial Widget")
   TSubclassOf<UOSERadialWidgetItem> WidgetItemClass;

   // How big are the widget items?
   UPROPERTY(EditDefaultsOnly, Category = "Radial Widget")
   FVector2D WidgetItemSize;

   // How big is this radial menu?
   UPROPERTY(EditDefaultsOnly, Category = "Radial Widget")
   FVector2D WidgetSize;

   // Add items counter-clockwise instead of clockwise.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget")
   bool ReverseRadialItemOrder = false;

   // Starting angle to add items at
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget", meta = (UIMin = "-360.0", UIMax = "360.0", ForceUnits = "degrees"))
   float RadialOffsetDegrees = -90.0f;

   // Shrinks items by half this amount on each side to give them some padding
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "180.0", UIMax = "25.0", ForceUnits = "degrees"))
   float RadialItemSizeAdjustDegrees = 2.0f;

   // The radial settings to use for this widget; exposed as a data asset to share configuration w/ all radials of a given type.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget")
   UOSERadialSettingsAsset* Settings = nullptr;

protected:
   // Sets the canceled flag for this radial menu to true (without causing any other side effects)
   UFUNCTION(BlueprintCallable, Category = "Radial Widget")
   void _MarkRadialMenuCanceled();

   UFUNCTION(BlueprintCallable, Category = "Radial Widget")
   void _SetRadialMenuInputsEnabled(bool enabled);

   // Updates the parameters of an existing radial item
   // Note that this will not allow updating the weight, as that would require recomputing the location of all other items
   UFUNCTION(BlueprintCallable, Category = "Radial Widget")
   bool _UpdateRadialItem(int32 index, const FOSERadialItemInfo& newInfo);

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnRadialBackgroundItemCreated(const FOSERadialItemInfo& item, UPARAM(ref) FOSERadialSlice& backgroundItem, int32 index, const FFloatInterval& itemArcAngleDegrees);
   void _OnRadialBackgroundItemCreated_Implementation(const FOSERadialItemInfo& item, FOSERadialSlice& backgroundItem, int32 index, const FFloatInterval& itemArcAngleDegrees) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnRadialItemCreated(const FOSERadialItemInfo& item, UOSERadialWidgetItem* widget, int index, const FFloatInterval& itemArcAngleDegrees);
   void _OnRadialItemCreated_Implementation(const FOSERadialItemInfo& item, UOSERadialWidgetItem* widget, int index, const FFloatInterval& itemArcAngleDegrees) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnHoveredItemChanged(const FOSERadialItemInfo& item, UOSERadialWidgetItem* widget, int index);
   void _OnHoveredItemChanged_Implementation(const FOSERadialItemInfo& item, UOSERadialWidgetItem* widget, int index) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnAboutToChangeEnabled(bool oldEnabled, bool newEnabled);
   void _OnAboutToChangeEnabled_Implementation(bool oldEnabled, bool newEnabled) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnEnabledChanged(bool newEnabled);
   void _OnEnabledChanged_Implementation(bool newEnabled) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnCanceled();
   void _OnCanceled_Implementation() { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnRadialSelectAndRelease(int32 itemIndex, const FOSERadialItemInfo& item);
   void _OnRadialSelectAndRelease_Implementation(int32 itemIndex, const FOSERadialItemInfo& item) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget")
   void _OnRadialAction(EOSERadialActionType actionType, const FInputActionValue& inputActionValue, bool haveValidItem, int32 itemIndex, const FOSERadialItemInfo& item);
   void _OnRadialAction_Implementation(EOSERadialActionType actionType, const FInputActionValue& inputActionValue, bool haveValidItem, int32 itemIndex, const FOSERadialItemInfo& item) { }

   // Helper to fire _OnRadialAction with the currently selected item index and item
   void _FireRadialActionWithType(const FInputActionValue& inputActionValue, EOSERadialActionType actionType);

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

private:
   AOSEPlayerController* _GetPlayerController() const;
   UEnhancedInputComponent* _GetInputComponent() const;
   UEnhancedPlayerInput* _GetPlayerInput() const;
   void _RadialItemToBackgroundSlice(const FOSERadialItemInfo& itemInfo, FOSERadialSlice& bgItem) const;
   void _ClearRadialWidget();
   void _BuildRadialWidget();
   void _TickUpdateHoveredItems();
   void _TickRestrictMouseMovement();
   void _OnDirectionalInput(const FInputActionValue& value);
   void _OnCancelInput(const FInputActionValue& value);
   bool _GetRotationFromMousePosition(float& rotDeg);
   bool _GetRotationFromGamepadThumbstick(float& rotDeg);
   void _SetCurrentHoveredIndex(int index);
   int32 _AngleToItemIndex(float angleDeg) const;
   const FOSERadialItemInfo* _TryGetRadialItemInfo(int32 index) const;

private:
   UPROPERTY(Transient)
   TArray<UOSERadialWidgetItem*> _runtimeWidgetItems;
   UPROPERTY(Transient)
   FOSERadialWidgetBuildParams _params;

   // Sum of the weight value for all items
   int32 _totalItemWeight = 0;

   // widget state
   bool _isEnabled = false;
   int _currentHoveredIndex = INDEX_NONE;
   bool _wasCanceled = false;

   // input states:

   // m/k specific state
   FVector2D _lastGoodMousePos;
   
   // gamepad specific state
   FVector2D _lastGoodGamepadStickDir;
   float _previousGamepadDistanceFromCenter = 0.0f;

   // shared state
   bool _wasLastInputValid = false;
};

//---------------------------------------------------------------------------------------
// UOSERadialSettingsAsset
//---------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSECORE_API UOSERadialSettingsAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Widget")
   FOSERadialSettings RadialSettings;

   /// Reads values from the FOSERadialSettings::RadialActionInputs array.
   /// (The array is fixed-size array, which means it can't be marked BlueprintReadOnly)
   UFUNCTION(BlueprintPure, Category = "Radial Widget")
   static UInputAction* GetRadialSettingsRadialActionInput(const FOSERadialSettings& settings, EOSERadialActionType actionType)
   {
      const uint32 index = static_cast<uint32>(actionType);
      return (index < static_cast<uint32>(EOSERadialActionType::MAX)) ? settings.RadialActionInputs[index] : nullptr;
   }

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
