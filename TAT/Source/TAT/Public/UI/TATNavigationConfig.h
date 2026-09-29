// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Framework/Application/NavigationConfig.h"

class UInputAction;
class UTATUserWidget;
class UTATScreenMgr;
class UTATScreenWidget;

// An extended set of navigation actions, for situations that can't be gracefully handled by EUINavigationAction or EUINavigation
UENUM(BlueprintType)
enum class EUIExtendedNavigationAction : uint8
{
   TabNext,
   TabPrevious,
   SubTabNext,
   SubTabPrevious,
   
   Pause,

   DestructiveAction,
   Interact,

   Invalid
};

class TAT_API FTATNavigationConfig : public FNavigationConfig
{
public:
   FTATNavigationConfig();
   virtual ~FTATNavigationConfig() {}

   // static 
   static TSharedRef<FTATNavigationConfig> GetInstance();

   // from FNavigationConfig
   virtual void OnNavigationChangedFocus(TSharedPtr<SWidget> oldWidget, TSharedPtr<SWidget> newWidget, FFocusEvent focusEvent) override;
   EUIExtendedNavigationAction GetExtendedNavigationActionFromKey(const FKeyEvent& InKeyEvent, const UTATUserWidget* widget) const;


private:
   UTATScreenMgr* _TryGetTATScreenMgr() const;
   
   // Climb through widget's parent tree checking for FTATScreenMetadata, returning associated TATScrenWidget or nullptr if none found
   UTATScreenWidget* _TryFindOwningScreenWidget(TSharedPtr<SWidget> widget) const;

private:
   // static
   static TSharedPtr<FTATNavigationConfig> _sInstance;

   TMap<FKey, EUIExtendedNavigationAction> _extendedKeyEventRules;
};

