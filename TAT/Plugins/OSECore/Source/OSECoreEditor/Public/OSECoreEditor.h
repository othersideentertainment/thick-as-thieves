// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "IAssetTypeActions.h"
#include "IAssetTools.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Toolkits/AssetEditorToolkit.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSECoreEditor, Log, All);

class FOSECoreEditor : public IModuleInterface
   , public IHasToolBarExtensibility
   , public IHasMenuExtensibility
{
public:

   DECLARE_DELEGATE_OneParam(FPullDownMenuCreated, FMenuBuilder&)
   
   // from IModuleInterface implementation
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override { return true; }

   // from IHasMenuExtensibility
   virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override;

   // from IHasToolBarExtensibility
   virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override;

   FPullDownMenuCreated OnPullDownMenuCreated;
   
   EAssetTypeCategories::Type GetOESAudioAssetCategoryBit() const { return _oseAudioCategoryBit; }
   EAssetTypeCategories::Type GetOSEVoiceOverAssetCategoryBit() const { return _oseVoiceOverCategoryBit; }

   static inline FName GetStyleSetName()
   {
      static const FName styleSetName = FName("TOWEditorModuleStyle");
      return styleSetName;
   }

private:

   /// Static init / shutdown methods
   void _RegisterComponentVisualizers();
   void _UnRegisterComponentVisualizers();

private:

   void _RegisterClassLayouts();
   void _UnRegisterClassLayouts();
   void _RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate);

   void _RegisterPropertyTypeLayouts();
   void _UnRegisterPropertyTypeLayouts();
   void _RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance DetailLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier = nullptr);

   void _RegisterAssetTools();
   void _RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);
   void _UnRegisterAssetTools();

   void _RegisterMenuExtensions();
   void _UnRegisterMenuExtensions();

   void _RegisterStyles();
   void _UnRegisterStyles();

   void _RegisterOSEBaseEditorTools();
   void _UnRegisterOSEBaseEditorTools();

   void _AddOSEMenuBar();
   void _AddOSEMenuBarExtension(FMenuBarBuilder& menuBarBuilder);

   void _AddOSEContentBrowserMenuOptions();

private:
   // Tools
   void _DeleteStaleWorldSettings();
   
   // Delegate for FCoreDelegates::OnPostEngineInit
   void _OnPostEngineInit();
   
   // Delegate for FCoreDelegates::OnPreExit
   void _OnPreExit();

private:
   TSet<FName> _registeredClassNames;
   TSet<FName> _registeredPropertyTypeNames;

   // Holds the menu extensibility manager.
   TSharedPtr<FExtensibilityManager> _menuExtensibilityManager;

   // The collection of registered asset type actions.
   TArray<TSharedRef<IAssetTypeActions>> _registeredAssetTypeActions;

   // Holds the tool bar extensibility manager.
   TSharedPtr<FExtensibilityManager> _toolBarExtensibilityManager;

   // "OSE" dropdown
   TSharedPtr<FExtender> _OSEMenuExtender;

   // style set for custom class icons etc
   TSharedPtr<FSlateStyleSet> _slateStyleSet;

   // Handles
   FDelegateHandle _onPreExitHandle;
   FDelegateHandle _onPostEngineInitHandle;

   EAssetTypeCategories::Type _oseAudioCategoryBit;
   EAssetTypeCategories::Type _oseVoiceOverCategoryBit;

};
