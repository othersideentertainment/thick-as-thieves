// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once 

#include "CoreMinimal.h"

// tat
#include "UI/TATHUDIndicatorTypes.h"

// ose
#include "UI/OSEHUD.h"
#include "OSECoreCheats.h"

// ue
#include "GameplayTagContainer.h"

#include "TATHUD.generated.h"

class UWidget;
class UTATCompassWidget;
class UOSERadialWidget;
class UUserWidget;
class UOSEAnimatedSwitcher;
struct FOSERadialItemInfo;
class UTATHUDToolWidget;

UENUM(BlueprintTYpe)
enum class ETATHUDLayers : uint8
{
   Default,
   Character,

   Num // Not a realy layer, the total number of layers
};

UCLASS()
class TAT_API ATATHUD : public AOSEHUD
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHUDIconsVisiblilityChanged, bool, showIcons);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionPanelVisibilityChanged, bool, showPanel);

public:
   ATATHUD();

   UFUNCTION(BlueprintCallable, Category = "TAT HUD", meta = (WorldContext = "contextObj"))
   static ATATHUD* TryGetLocalTATHUD(const UObject* contextObj);

   /// Crosshair asset pointer
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = HUD)
   class UTexture2D* CrosshairTex;

   // from AHUD
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void PostInitializeComponents() override;

   /// The Main Draw loop for the hud.  Gets called before any messaging.  Should be subclassed
   virtual void PostRender() override;
   /// Primary draw call for the HUD
   virtual void DrawHUD() override;

   // Returns reference to TATCompass widget. Only valid after BeginPlay has executed.
   UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "TAT HUD")
   UTATCompassWidget* GetTATCompass() const;

   // respect bShowHUD in our widget blueprints
   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnHUDVisibilityChanged(UUserWidget* previousHUD, UUserWidget* newHUD);

   // For use by tutorial highlighting
   // could also be an interface
   UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "TAT HUD", meta = (Categories = "HudElement"))
   UWidget* GetHudElementByTag(FGameplayTag tag) const;

   // hud icons vis
   UFUNCTION(BlueprintCallable, Category = "TAT HUD")
   static void SetHUDIconsVisible(UObject* contextObject, bool showIcons);
   UFUNCTION(BlueprintPure, Category = "TAT HUD")
   bool AreHUDIconsVisible() const { return _areHUDIconsVisible; }
   UPROPERTY(BlueprintAssignable, Category = "TAT HUD")
   FOnHUDIconsVisiblilityChanged OnHUDIconsVisiblilityChanged;
   
   UFUNCTION(BlueprintCallable, Category = "TAT HUD")
   virtual void SetHUDLayerWidget(ETATHUDLayers layer, UUserWidget* widget);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   UUserWidget* CreateWidgetForHUDIndicator(const FTATHUDIndicatorState& indicatorState);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT HUD")
   void OnHUDIndicatorChanged(const FTATHUDIndicatorInfo& indicatorInfo);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT HUD")
   void OnHUDIndicatorRemoved(const FTATHUDIndicatorInfo& indicatorInfo);

#if OSE_CHEATS_ENABLED
   void ToggleDebugDrawHUDIndicators() { _debugDrawHUDIndicators = !_debugDrawHUDIndicators; }
#endif

   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnAddToolWidget(UTATHUDToolWidget* hudToolWidget);
   void OnAddToolWidget_Implementation(UTATHUDToolWidget* hudToolWidget) {}

   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnRemoveToolWidget(UTATHUDToolWidget* hudToolWidget);
   void OnRemoveToolWidget_Implementation(UTATHUDToolWidget* hudToolWidget) {}

   UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "TAT HUD")
   UUserWidget* GetCurrentReticleWidget() const;

   UFUNCTION(BlueprintCallable, Category = "TAT HUD")
   void RegisterRadialMenu(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget);

   UFUNCTION(BlueprintPure, Category = "TAT HUD")
   UOSERadialWidget* GetRadialMenu(FGameplayTag radialMenuTag) const;

   UFUNCTION(BlueprintCallable, Category = "TAT HUD")
   bool SetRadialMenuActive(FGameplayTag radialMenuTag, bool isActive);

   UFUNCTION(BlueprintCallable, Category = "TAT HUD")
   bool CloseAllRadialMenus(bool cancelRadials);

   UFUNCTION(BlueprintPure, Category = "TAT HUD")
   bool IsRadialMenuActive(FGameplayTag radialMenuTag) const;

   UFUNCTION(BlueprintPure, Category = "TAT HUD")
   bool IsAnyRadialMenuActive() const;

   UFUNCTION(BlueprintCallable, BlueprintPure = "false", Category = "TAT HUD")
   bool GetCurrentRadialItem(FGameplayTag radialMenuTag, FOSERadialItemInfo& outRadialItem, bool& outWasCancelled) const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRadialMenuEvent, FGameplayTag, radialMenuTag, bool, isActive);
   UPROPERTY(BlueprintAssignable, Category = "TAT HUD")
   FRadialMenuEvent RadialMenuChanged;

protected:
   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnRadialMenuChanged(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget, bool isActive);
   void OnRadialMenuChanged_Implementation(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget, bool isActive) {}

   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnRadialMenuOpened();
   void OnRadialMenuOpened_Implementation() {}

   UFUNCTION(BlueprintNativeEvent, Category = "TAT HUD")
   void OnRadialMenuClosed();
   void OnRadialMenuClosed_Implementation() {}

private:
   void _SetRadialMenuState(FGameplayTag radialMenuTag, UOSERadialWidget* radialWidget, bool isActive, bool updateRadialSwitcher, bool activateInstantly = false);

   UFUNCTION()
   void _OnRadialSwitcherChanged(UWidget* prevWidget, int32 prevWidgetIndex, UWidget* activeWidget, int32 activeWidgetIndex);

   void _CheckForShowHudChanged();
   void _UpdateHUDLayersVisibility();
   void _SetHUDIconsVisible(bool showIcons);

private:
   // same default as bShowHUD
   bool _prevShowHUD = true;

   UPROPERTY(Transient)
   UUserWidget* _currentVisibleHUD = nullptr;

   // are we currently showing the hud indicators?
   bool _areHUDIconsVisible = true;

   // HUD widgets per layer
   UPROPERTY(Transient)
   TArray<UUserWidget*> _hudLayers;

   // All registered radial menus
   UPROPERTY(Transient)
   TMap<FGameplayTag, UOSERadialWidget*> _radialMenus;

   // Just used to remember which animated switcher widgets we registered the _OnRadialSwitcherChanged callback for, to prevent duplicates
   UPROPERTY(Transient)
   TSet<UOSEAnimatedSwitcher*> _radialMenuParentSwitchers;

#if OSE_CHEATS_ENABLED
   bool _debugDrawHUDIndicators = false;
#endif
};
