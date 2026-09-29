// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"
#include "ContentBrowserDelegates.h"

class IAssetTools;
class IAssetTypeActions;

class FTATEditor : public IModuleInterface
   , public IHasToolBarExtensibility
   , public IHasMenuExtensibility
{
public:

   // from IModuleInterface implementation
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override { return true; }

   // from IHasMenuExtensibility
   virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override;

   // from IHasToolBarExtensibility
   virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override;


   EAssetTypeCategories::Type GetTATUpgradeCategoryBit() const { return _tatUpgradeCategoryBit; }
   EAssetTypeCategories::Type GetTATMapVariationCategoryBit() const { return _tatMapVariationCategoryBit; }
   EAssetTypeCategories::Type GetTATQuestCategoryBit() const { return _tatQuestCategoryBit; }

private:

   // menu/toolbar extensions
   void _RegisterMenuExtensions();
   void _UnRegisterMenuExtensions();

   // level editor tabs
   void _RegisterLevelEditorTabs(const TSharedPtr<FTabManager>& tabManager);
   void _UnregisterLevelEditorTabs(const TSharedPtr<FTabManager>& tabManager);

   // visualizers
   void _RegisterVisualizers();
   void _UnRegisterVisualizers();

   void _RegisterPropertyCustomization();
   void _UnRegisterPropertyCustomization();

   // asset tools
   void _RegisterAssetTools();
   void _RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);
   void _UnRegisterAssetTools();

   // consideration fixup
   void _RegisterConsiderationFixup();

private:
   // menu bars
   void _AddTATMenuBar();
   void _AddTATMenuBarExtension(FMenuBarBuilder& menuBarBuilder);

   // content browser bars
   void _AddTATContentBrowserMenuOptions();
   void _RemoveTATContentBrowserMenuOptions();

   // log categories
   void _InitLogCategories();

   // Delegate for FCoreDelegates::OnPostEngineInit
   void _OnPostEngineInit();

   // Delegate for FCoreDelegates::OnPreExit
   void _OnPreExit();

private:
   // Holds the menu extensibility manager.
   TSharedPtr<FExtensibilityManager> _menuExtensibilityManager;

   // Holds the tool bar extensibility manager.
   TSharedPtr<FExtensibilityManager> _toolBarExtensibilityManager;

   // "TAT" menu in the content browser dropdown
   TSharedPtr<FExtender> _TATMenuExtender;

   // The collection of registered asset type actions.
   TArray<TSharedRef<IAssetTypeActions>> _registeredAssetTypeActions;

   TSharedPtr<FWorkspaceItem> _menuGroup;

   // Handles
   FDelegateHandle _onPreExitHandle;
   FDelegateHandle _onPostEngineInitHandle;
   FDelegateHandle _tabManagerChangedHandle;
   FDelegateHandle _gameplayTagMetaHandle;
   FDelegateHandle _actorDetailsHandle;

   // Categories
   EAssetTypeCategories::Type _tatUpgradeCategoryBit;
   EAssetTypeCategories::Type _tatMapVariationCategoryBit;
   EAssetTypeCategories::Type _tatQuestCategoryBit;

   FContentBrowserMenuExtender_SelectedAssets _stringTableContextMenuExtenderDelegate;
};

DECLARE_LOG_CATEGORY_EXTERN(LogTATEditor, Log, All);
