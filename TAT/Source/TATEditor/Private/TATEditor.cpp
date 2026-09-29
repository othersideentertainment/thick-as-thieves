// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATEditor.h"

// tat
#include "AI/Patrol/PatrolPath.h"
#include "AI/Patrol/PatrolPathComponent.h"
#include "AI/Utility/TATConsiderationInputs.h"
#include "Developer/TATActorDependencyVisComponent.h"
#include "GameFramework/TATEndgameActionComponent.h"
#include "Interactables/Electrical/TATPowerSourceVisComponent.h"
#include "Interactables/Electrical/TATPowerSwitch.h"
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/TATExclusiveToggleGroup.h"
#include "Interactables/TATTimedSwitch.h"
#include "Lockpicking/TATLockCombinationName.h"
#include "Loot/TATContainerSpawnerComponent.h"
#include "Loot/TATLootTypes.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATClueSpawner.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "Variation/SceneVariants/TATSceneRequirementVisComponent.h"
#include "Variation/SceneVariants/TATSceneVariantActorSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSelfDestructRequirementComponent.h"
#include "Thiefsign/TATThiefsignTypes.h"
#include "Traps/Old/EmitterComponent_SpawnActor.h"

// tat editor
#include "AI/PatrolPathComponentVisualizer.h"
#include "AI/TATPatrolPointOverrideCustomization.h"
#include "AssetTypeActions/AssetTypeActions_TATUpgrades.h"
#include "AssetTypeActions/AssetTypeActions_TATMapVariation.h"
#include "Editor/TATWorldAssetTags.h"
#include "Editor/TATActorDetailsExtensions.h"
#include "Interactables/TATActorDependencyVisualizer.h"
#include "Interactables/TATPowerSourceComponentVisualizer.h"
#include "Interactables/TATPowerNetworkComponentVisualizer.h"
#include "Interactables/TATPowerSwitchComponentVisualizer.h"
#include "Interactables/TATTimedSwitchCustomization.h"
#include "Interactables/TATToggleGroupVisualizer.h"
#include "Loot/TATContainerSpawnerComponentVisualizer.h"
#include "Loot/TATLootIdentifierCustomization.h"
#include "Variation/ClueSpawnerComponentVisualizer.h"
#include "Variation/QuestSpawnerComponentVisualizer.h"
#include "Variation/SceneRequirementComponentVisualizer.h"
#include "Variation/SceneVariantActorSetVisualizer.h"
#include "Variation/SpawnerComponentVisualizer.h"
#include "Variation/MapVariationValidationEditorTool.h"
#include "Variation/TATClueCustomization.h"
#include "Variation/TATCombinationLockCustomization.h"
#include "Variation/TATSceneVariantCustomization.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"
#include "Thiefsign/TATThiefsignMaterialParamKeyCustomization.h"
#include "Traps/SpawnEmitterComponentVisualizer.h"
#include "Environment/TATWeatherTool.h"

// ose
#include "AI/Utility/ConsiderationInput.h"

// ose editor
#include "OSECoreEditor.h"
#include "Editor/OSEBaseEditorTool.h"

// ue5
#include "Editor/StringTableEditor/Public/IStringTableEditor.h"
#include "Interactables/TATShuffledToggleBindings.h"
#include "Interactables/TATShuffledToggleCustomization.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DataTableEditorUtils.h"
#include "GameplayTagsManager.h"
#include "LevelEditor.h"
#include "MessageLogModule.h"
#include "UnrealEd.h"
#include "Engine/World.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"


DEFINE_LOG_CATEGORY(LogTATEditor);

namespace
{
   template<typename TComponent, typename TVisualizer, typename... TArgs>
   static void RegisterVisualizer(TArgs&&... args)
   {
      if (GUnrealEd)
      {
         TSharedPtr<FComponentVisualizer> visualizer = MakeShared<TVisualizer>(Forward<TArgs>(args)...);
         GUnrealEd->RegisterComponentVisualizer(TComponent::StaticClass()->GetFName(), visualizer);
         visualizer->OnRegister();
      }
      else
      {
         UE_LOG(LogTATEditor, Log, TEXT("GUnrealEd is not available, cannot register component visualizer for class %s!"), *TComponent::StaticClass()->GetName());
      }
   }

   template<typename TComponent>
   static void RegisterSceneRequirementVisualizer()
   {
      RegisterVisualizer<TComponent, FTATSceneRequirementComponentVisualizer>(TSceneRequirementVisAdapter<TComponent>());
   }

   static void OnGetCategoriesMetaFromPropertyHandle(TSharedPtr<IPropertyHandle> propertyHandle, FString& metaString)
   {
      TSharedPtr<IPropertyHandle> parentHandle = propertyHandle->GetParentHandle();
      if (!parentHandle.IsValid())
      {
         return;
      }

      // delegate to the categories meta of of the loot identifier parent if it has one
      // The default is to walk up the chain. It is just that the loot identifier already has one
      // This does not change behavior if there is no categories meta on the field
      const FStructProperty* structProperty = CastField<FStructProperty>(parentHandle->GetProperty());
      if (structProperty && structProperty->Struct == StaticStruct<FTATLootIdentifier>())
      {
         metaString = UGameplayTagsManager::StaticGetCategoriesMetaFromPropertyHandle(parentHandle);
      }
   }

   static void SortDataTableHeaders(TArray<const FProperty*>& properties)
   {
      auto getPriority = [](const FProperty* prop) {
         static const FName kSortProperty("DataTableHeaderPriority");
         return prop->HasMetaData(kSortProperty) ? prop->GetIntMetaData(kSortProperty) : 0;
         };

      Algo::StableSortBy(properties, getPriority);
   }
}

namespace
{
   // Extend the Content Browser context menu for String Table assets with a keyword search tool.
   // Results are displayed in the Message Log under the "StringTableSearch" listing.
   TSharedRef<FExtender> ExtendStringTableContextMenu(const TArray<FAssetData>& SelectedAssets)
   {
      TSharedRef<FExtender> Extender = MakeShared<FExtender>();

      // Only extend the context menu if at least one selected asset is a String Table
      bool bHasStringTable = false;
      for (const FAssetData& AssetData : SelectedAssets)
      {
         if (AssetData.AssetClassPath == UStringTable::StaticClass()->GetClassPathName())
         {
            bHasStringTable = true;
            break;
         }
      }

      if (!bHasStringTable)
      {
         return Extender;
      }

      Extender->AddMenuExtension(
         "GetAssetActions",
         EExtensionHook::After,
         nullptr,
         FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& MenuBuilder)
         {
            MenuBuilder.BeginSection("TATStringTableActions", FText::FromString("TAT Actions"));
            MenuBuilder.AddMenuEntry(
               FText::FromString("Search String Tables"),
               FText::FromString("Search all string tables for a keyword"),
               FSlateIcon(),
               FUIAction(FExecuteAction::CreateLambda([]()
               {
                  // Searches all registered String Tables for the given keyword and logs results to the Message Log.
                  // Each result includes a clickable hyperlink that opens the String Table editor and highlights the matching row.
                  auto PerformSearch = [](const FString& Keyword)
                  {
                     FMessageLog messageLog("StringTableSearch");

                     // Each search creates a new page in the Message Log, keeping previous searches available for a sort of search history.
                     // Page count is capped in the RegisterLogListing() call in StartupModule().
                     messageLog.NewPage(FText::FromString(FString::Printf(TEXT("Search: '%s'"), *Keyword)));
                     bool noResultsFound = true;

                     FStringTableRegistry::Get().EnumerateStringTables([&Keyword, &messageLog, &noResultsFound](const FName& tableId, const FStringTableConstRef& stringTable) -> bool
                     {
                        stringTable->EnumerateSourceStrings([&Keyword, &tableId, &messageLog, &noResultsFound](const FString& key, const FString& sourceString) -> bool
                        {
                           if (sourceString.Contains(Keyword, ESearchCase::IgnoreCase))
                           {
                              noResultsFound = false;

                              TSharedRef<FTokenizedMessage> message = FTokenizedMessage::Create(EMessageSeverity::Info);

                              // Create a clickable hyperlink token.  When clicked, opens the specified String Table Asset in Editor.
                              message->AddToken(FActionToken::Create(
                                 FText::FromString(FString::Printf(TEXT("[%s] Key: %s"), *tableId.ToString(), *key)),
                                 FText::FromString(TEXT("Open String Table")),
                                 FOnActionTokenExecuted::CreateLambda([tableId, key]()
                                 {
                                    if (UObject* asset = StaticLoadObject(UStringTable::StaticClass(), nullptr, *tableId.ToString()))
                                    {
                                       UAssetEditorSubsystem* assetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
                                       assetEditorSubsystem->OpenEditorForAsset(asset);

                                       // After opening, highlight the matching row in the Editor
                                       IAssetEditorInstance* editorInstance = assetEditorSubsystem->FindEditorForAsset(asset, false);
                                       if (IStringTableEditor* stringTableEditor = static_cast<IStringTableEditor*>(editorInstance))
                                       {
                                          stringTableEditor->RefreshStringTableEditor(key);
                                       }
                                    }
                                 }),
                                 true
                              ));

                              // Print the String Table Entry so people can quickly glance if this is the kind of string they're looking for.
                              message->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("- %s"), *sourceString))));
                              messageLog.AddMessage(message);
                           }
                           return true;
                        });
                        return true;
                     });

                     if (noResultsFound)
                     {
                        messageLog.Warning(FText::FromString(FString::Printf(TEXT("No results found with keyword: %s"), *Keyword)));
                     }

                     // Open the Message Log and focus the StringTableSearch listing
                     messageLog.Open(EMessageSeverity::Info, true);
                  };

                  // If the search window is already open, bring it to the front instead of opening a new one
                  static TWeakPtr<SWindow> ExistingWindow;
                  if (TSharedPtr<SWindow> PinnedWindow = ExistingWindow.Pin())
                  {
                     PinnedWindow->BringToFront();
                     return;
                  }

                  // Keyword input supports both Enter key and Search button to trigger the search
                  TSharedRef<SEditableTextBox> KeywordInput = SNew(SEditableTextBox)
                     .HintText(FText::FromString("Enter keyword..."))
                     .OnTextCommitted_Lambda([PerformSearch](const FText& Text, ETextCommit::Type CommitType)
                     {
                        if (CommitType == ETextCommit::OnEnter)
                        {
                           PerformSearch(Text.ToString());
                        }
                     });

                  TSharedRef<SWindow> Window = SNew(SWindow)
                     .Title(FText::FromString("Search String Tables"))
                     .ClientSize(FVector2D(400, 80))
                     .SupportsMaximize(false)
                     .SupportsMinimize(false)
                     [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .Padding(10)
                        [
                           KeywordInput
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(10)
                        [
                           SNew(SButton)
                           .Text(FText::FromString("Search"))
                           .OnClicked_Lambda([PerformSearch, KeywordInput]() -> FReply
                           {
                              PerformSearch(KeywordInput->GetText().ToString());
                              return FReply::Handled();
                           })
                        ]
                     ];

                  // Cache a weak reference so we can detect if the window is already open
                  ExistingWindow = Window;
                  FSlateApplication::Get().AddWindow(Window);
               }))
            );
            MenuBuilder.EndSection();
         })
      );

      return Extender;
   }
}

void FTATEditor::StartupModule()
{
   ensureMsgf(FModuleManager::Get().IsModuleLoaded("TAT"), TEXT("TATEditor depends on the TAT module."));
   ensureMsgf(FModuleManager::Get().IsModuleLoaded("OSECoreEditor"), TEXT("TATEditor depends on the OSECoreEditor module."));

   _RegisterMenuExtensions();
   _InitLogCategories();

   if (!_onPostEngineInitHandle.IsValid())
      _onPostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTATEditor::_OnPostEngineInit);

   if (!_onPreExitHandle.IsValid())
      _onPreExitHandle = FCoreDelegates::OnPreExit.AddRaw(this, &FTATEditor::_OnPreExit);

   if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
   {
      _AddTATMenuBar();

      FLevelEditorModule& levelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
      _tabManagerChangedHandle = levelEditorModule.OnTabManagerChanged().AddLambda([this]()
      {
         FLevelEditorModule& levelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
         if (TSharedPtr<FTabManager> levelEditorTabManager = levelEditor.GetLevelEditorTabManager())
         {
            _RegisterLevelEditorTabs(levelEditorTabManager);
         }
      });
   }
   
   _actorDetailsHandle = OnExtendActorDetails.AddStatic(&TATActorDetailsExtensions::ExtendActorDetails);

   _gameplayTagMetaHandle = UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.AddStatic(&OnGetCategoriesMetaFromPropertyHandle);

   if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
   {
      _AddTATContentBrowserMenuOptions();
   }

   if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
   {
      FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
      FMessageLogInitializationOptions options;
      options.MaxPageCount = 10;
      options.bDiscardDuplicates = true;
      MessageLogModule.RegisterLogListing(FName("StringTableSearch"), FText::FromString("String Table Search Results"), options);
   }

   IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
   _tatUpgradeCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("TATUpgrades")), FText::FromString(TEXT("TAT Upgrades")));
   _tatMapVariationCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("TATMapVariation")), FText::FromString(TEXT("TAT Map Variation")));
   _tatQuestCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("TATQuests")), FText::FromString(TEXT("TAT Quests")));

   _RegisterPropertyCustomization();
   _RegisterAssetTools();
   _RegisterConsiderationFixup();

   FWorldDelegates::GetAssetTagsWithContext.AddStatic(&TATWorldAssetTags::AddWorldAssetTags);

   FDataTableEditorUtils::GetOnSortHeaderPropertiesDelegate().AddStatic(&SortDataTableHeaders);
}

void FTATEditor::ShutdownModule()
{
   _UnRegisterMenuExtensions();
   _UnRegisterAssetTools();
   _UnRegisterPropertyCustomization();

   OnExtendActorDetails.Remove(_actorDetailsHandle);

   if (UGameplayTagsManager* tagManager = UGameplayTagsManager::GetIfAllocated())
   {
      tagManager->OnGetCategoriesMetaFromPropertyHandle.Remove(_gameplayTagMetaHandle);
   }

   if (FLevelEditorModule* levelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
   {
      levelEditor->OnTabManagerChanged().Remove(_tabManagerChangedHandle);
      if (TSharedPtr<FTabManager> levelEditorTabManager = levelEditor->GetLevelEditorTabManager())
      {
         _UnregisterLevelEditorTabs(levelEditorTabManager);
      }
   }

   if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
   {
      _RemoveTATContentBrowserMenuOptions();
   }

   if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
   {
      FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
      MessageLogModule.UnregisterLogListing(FName("StringTableSearch"));
   }
}

void FTATEditor::_OnPostEngineInit()
{
   _RegisterVisualizers();
}

void FTATEditor::_OnPreExit()
{
   _UnRegisterVisualizers();
}

TSharedPtr<FExtensibilityManager> FTATEditor::GetMenuExtensibilityManager()
{
   return _menuExtensibilityManager;
}

TSharedPtr<FExtensibilityManager> FTATEditor::GetToolBarExtensibilityManager()
{
   return _toolBarExtensibilityManager;
}

void FTATEditor::_RegisterMenuExtensions()
{
   _menuExtensibilityManager = MakeShareable(new FExtensibilityManager);
   _toolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
}

void FTATEditor::_UnRegisterMenuExtensions()
{
   _menuExtensibilityManager.Reset();
   _toolBarExtensibilityManager.Reset();
}

void FTATEditor::_RegisterLevelEditorTabs(const TSharedPtr<FTabManager>& tabManager)
{
   check(tabManager.IsValid());
   UOSEBaseEditorTool::AddToolToTab(UTATMapVariationValidationEditorTool::StaticClass(), tabManager.ToSharedRef(), _menuGroup.ToSharedRef());
   UOSEBaseEditorTool::AddToolToTab(UTATWeatherTool::StaticClass(), tabManager.ToSharedRef(), _menuGroup.ToSharedRef(), FSlateIcon(FOSECoreEditor::GetStyleSetName(), "ClassIcon.TATWeatherManager"));
}

void FTATEditor::_UnregisterLevelEditorTabs(const TSharedPtr<FTabManager>& tabManager)
{
   check(tabManager.IsValid());
   UOSEBaseEditorTool::RemoveToolFromTab(UTATMapVariationValidationEditorTool::StaticClass(), tabManager.ToSharedRef());
   UOSEBaseEditorTool::RemoveToolFromTab(UTATWeatherTool::StaticClass(), tabManager.ToSharedRef());
}

void FTATEditor::_RegisterVisualizers()
{
   RegisterVisualizer<UEmitterComponent_SpawnActor_Old, FSpawnEmitterComponentVisualizer>();
   RegisterVisualizer<UPatrolPathComponent, FPatrolPathComponentVisualizer>();
   RegisterVisualizer<UTATSpawnerComponent, FTATSpawnerComponentVisualizer>();
   RegisterVisualizer<UTATContainerSpawnerComponent, FTATContainerSpawnerComponentVisualizer>();
   RegisterVisualizer<UTATQuestActorSpawnerComponent, FTATQuestSpawnerComponentVisualizer>();
   RegisterVisualizer<UTATClueSpawnerComponent, FTATClueSpawnerComponentVisualizer>();
   RegisterVisualizer<UTATPowerSourceVisComponent, FTATPowerSourceComponentVisualizer>();
   RegisterVisualizer<UTATPowerSwitch, FTATPowerSwitchComponentVisualizer>();
   RegisterVisualizer<UTATPowerNetworkComponent, FTATPowerNetworkComponentVisualizer>();
   RegisterVisualizer<UTATExclusiveToggleRequirementComponent, FTATExclusiveToggleRequirementVisualizer>();
   RegisterVisualizer<UTATExclusiveToggleGroupVisComponent, FTATExclusiveToggleGroupVisualizer>();
   RegisterVisualizer<UTATSceneVariantActorSetVisComponent, FTATSceneVariantActorSetVisualizer>();
   RegisterVisualizer<UTATActorDependencyVisComponent, FTATActorDependencyVisualizer>();
   RegisterSceneRequirementVisualizer<UTATSceneRequirementVisComponent>();
   RegisterSceneRequirementVisualizer<UTATSelfDestructRequirementComponent>();
   RegisterSceneRequirementVisualizer<UTATEndgameActionComponent>();
}

void FTATEditor::_UnRegisterVisualizers()
{
   if (GUnrealEd)
   {
      GUnrealEd->UnregisterComponentVisualizer(UEmitterComponent_SpawnActor_Old::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UPatrolPathComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATSpawnerComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATContainerSpawnerComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATQuestActorSpawnerComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATClueSpawnerComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATSceneRequirementVisComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATSelfDestructRequirementComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATEndgameActionComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATPowerSourceVisComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATPowerSwitch::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATPowerNetworkComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATExclusiveToggleRequirementComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATExclusiveToggleGroupVisComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATSceneVariantActorSetVisComponent::StaticClass()->GetFName());
      GUnrealEd->UnregisterComponentVisualizer(UTATActorDependencyVisComponent::StaticClass()->GetFName());
   }
}

void FTATEditor::_RegisterPropertyCustomization()
{
   FPropertyEditorModule& propertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
   
   propertyModule.RegisterCustomPropertyTypeLayout(FPatrolPoint::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATPatrolPointOverrideCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATSceneSpawnerOverride::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATSceneSpawnerOverrideCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATSceneRequirement::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATSceneRequirementCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATLockCombinationName::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATLockCombinationNameCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATLockCombinationNameRef::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATLockCombinationNameRefCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATThiefsignMaterialParamKey::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATThiefsignMaterialParamKeyCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATLootIdentifier::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATLootIdentifierCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATShuffledToggleBindingRef::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FShuffledToggleBindingRefCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATShuffledToggleTargetRef::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FShuffledToggleTargetRefCustomization::MakeInstance));
   propertyModule.RegisterCustomPropertyTypeLayout(FTATClueSetEntry::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTATClueSetEntryCustomization::MakeInstance));
   
   propertyModule.RegisterCustomClassLayout(ATATTimedSwitch::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTATTimedSwitchDetailsCustomization::MakeInstance));
}

void FTATEditor::_UnRegisterPropertyCustomization()
{
   FPropertyEditorModule* propertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

   if (propertyModule)
   {
      propertyModule->UnregisterCustomPropertyTypeLayout(FPatrolPoint::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATSceneSpawnerOverride::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATSceneRequirement::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATLockCombinationName::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATLockCombinationNameRef::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATThiefsignMaterialParamKey::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATLootIdentifier::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATShuffledToggleBindingRef::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATShuffledToggleTargetRef::StaticStruct()->GetFName());
      propertyModule->UnregisterCustomPropertyTypeLayout(FTATClueSetEntry::StaticStruct()->GetFName());
      
      propertyModule->UnregisterCustomClassLayout(ATATTimedSwitch::StaticClass()->GetFName());
   }
}

void FTATEditor::_RegisterAssetTools()
{
   IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();

   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATUpgradeType()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATUpgradeGraph()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATSpawnData()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATSceneSet()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATSceneAsset()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATSceneVariant()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATClueSet()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATCompoundClueSet()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATMatchQuestDescription()));
   //_RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TATStringTable()));
}

void FTATEditor::_RegisterAssetTypeAction(IAssetTools& assetTools, TSharedRef<IAssetTypeActions> action)
{
   assetTools.RegisterAssetTypeActions(action);
   _registeredAssetTypeActions.Add(action);
}

void FTATEditor::_UnRegisterAssetTools()
{
   FAssetToolsModule* assetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

   if (assetToolsModule != nullptr)
   {
      IAssetTools& assetTools = assetToolsModule->Get();

      for (auto action : _registeredAssetTypeActions)
      {
         assetTools.UnregisterAssetTypeActions(action);
      }
   }
}

void FTATEditor::_RegisterConsiderationFixup()
{   
   // give us a chance to fix up game-side considerations
   FBehaviorConsideration::OnFixupBehaviorConsiderationInput.BindLambda(
      [](UConsiderationInput* input, UObject* outerAsset)
      {
         return TATConsiderationFixup::FixupConsideration(input, outerAsset);
      }
   );
}

void FTATEditor::_AddTATMenuBar()
{
   if (!_TATMenuExtender)
   {
      _TATMenuExtender = MakeShareable(new FExtender);
      _TATMenuExtender->AddMenuBarExtension(
         TEXT("Help"),
         EExtensionHook::After,
         nullptr,
         FMenuBarExtensionDelegate::CreateRaw(this, &FTATEditor::_AddTATMenuBarExtension));

      if (FLevelEditorModule* levelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
         levelEditor->GetMenuExtensibilityManager()->AddExtender(_TATMenuExtender);

      _menuGroup = WorkspaceMenu::GetMenuStructure().GetToolsCategory()->AddGroup(
         FText::FromString(TEXT("TAT Tools")),
         FText::FromString(TEXT("TAT-specific editor tools")),
         FSlateIcon(FAppStyle::GetAppStyleSetName(), "WorkspaceMenu.AdditionalUI"),
         true);
   }
}

void FTATEditor::_AddTATMenuBarExtension(FMenuBarBuilder& menuBarBuilder)
{
   // Leaving this boilerplate code for the future, but for now we have nothing to put in this menu
   /*
   menuBarBuilder.AddPullDownMenu(
      FText::FromString(TEXT("TAT Editor Tools")),
      FText::FromString(TEXT("Editor tools for project TAT")),
      FNewMenuDelegate::CreateLambda([this](FMenuBuilder& menuBuilder)
         {
            // Map Variation Tools
            // menuBuilder.BeginSection(NAME_None, FText::FromString("Map Variation Tools"));
            // UOSEBaseEditorTool::AddToolToMenu(UTATMapVariationValidationEditorTool::StaticClass(), menuBuilder);
            // menuBuilder.EndSection();

            // ...
         }),
      TEXT("TAT"));
   */
}

void FTATEditor::_AddTATContentBrowserMenuOptions()
{
   FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
   _stringTableContextMenuExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&ExtendStringTableContextMenu);
   ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(_stringTableContextMenuExtenderDelegate);
}

void FTATEditor::_RemoveTATContentBrowserMenuOptions()
{
   FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
   ContentBrowserModule.GetAllAssetViewContextMenuExtenders().RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
   {
       return Delegate.GetHandle() == _stringTableContextMenuExtenderDelegate.GetHandle();
   });
}

void FTATEditor::_InitLogCategories()
{
   MapVariationValidationHelper::InitLog();
}

IMPLEMENT_GAME_MODULE(FTATEditor, TATEditor)
