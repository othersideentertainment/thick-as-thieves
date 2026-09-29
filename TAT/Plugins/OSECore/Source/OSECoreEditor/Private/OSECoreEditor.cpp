// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSECoreEditor.h"

// ose core
#include "Math/OSEMathFunctionLibrary.h"
#include "UI/OSERadialPaintLibrary.h"

// ose core editor
#include "OSECoreEditorSettings.h"
#include "Editor/OSEBaseEditorToolCustomization.h"
#include "Tools/FixupAbsolutePaths.h"
#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverBucket.h"
#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverConversation.h"
#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverLine.h"
#include "UI/OSERadialPaintCustomization.h"
#include "Math/OSEInterpModeCustomization.h"

// ue4
#include "AnimationUtils.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EngineUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionTerminator.h"
#include "LevelEditor.h"
#include "SourceCodeNavigation.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/AssetManager.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/ScopedSlowTask.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FOSECoreEditor"

DEFINE_LOG_CATEGORY(LogOSECoreEditor);

namespace
{
   template<typename TComponent, typename TVisualizer>
   static void RegisterVisualizer()
   {
      if (GUnrealEd)
      {
         TSharedPtr<FComponentVisualizer> visualizer = MakeShared<TVisualizer>();
         GUnrealEd->RegisterComponentVisualizer(TComponent::StaticClass()->GetFName(), visualizer);
         visualizer->OnRegister();
      }
      else
      {
         UE_LOG(LogOSECoreEditor, Log, TEXT("GUnrealEd is not available, cannot register component visualizer for class %s!"), *TComponent::StaticClass()->GetName());
      }
   }
}

void FOSECoreEditor::StartupModule()
{
   ensureMsgf(FModuleManager::Get().IsModuleLoaded("OSECore"), TEXT("OSECoreEditor depends on the OSECore module."));
   
   _RegisterOSEBaseEditorTools();
   _RegisterClassLayouts();
   _RegisterPropertyTypeLayouts();
   _RegisterAssetTools();
   _RegisterMenuExtensions();
   _RegisterStyles();

   if (!_onPostEngineInitHandle.IsValid())
      _onPostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FOSECoreEditor::_OnPostEngineInit);

   if (!_onPreExitHandle.IsValid())
      _onPreExitHandle = FCoreDelegates::OnPreExit.AddRaw(this, &FOSECoreEditor::_OnPreExit);

   if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
   {
      _AddOSEMenuBar();
   }

   if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
   {
      _AddOSEContentBrowserMenuOptions();
   }


   IAssetTools& assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
   _oseAudioCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("OSEAudio")), FText::FromString(TEXT("OSE Audio")));
   _oseVoiceOverCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("OSEVoiceOver")), FText::FromString(TEXT("OSE Voice Over")));

   // This is here to stop the "may result in hitches during streaming" warnings when it tries to recompress animations from sequence PostLoad
   UAnimBoneCompressionSettings* CompressionSettings = FAnimationUtils::GetDefaultAnimationBoneCompressionSettings();
}

void FOSECoreEditor::ShutdownModule()
{
   _UnRegisterOSEBaseEditorTools();
   _UnRegisterClassLayouts();
   _UnRegisterPropertyTypeLayouts();
   _UnRegisterAssetTools();
   _UnRegisterMenuExtensions();
   _UnRegisterStyles();
}

void FOSECoreEditor::_OnPostEngineInit()
{
   _RegisterComponentVisualizers();
}

void FOSECoreEditor::_OnPreExit()
{
   _UnRegisterComponentVisualizers();
}

TSharedPtr<FExtensibilityManager> FOSECoreEditor::GetMenuExtensibilityManager()
{
   return _menuExtensibilityManager;
}

TSharedPtr<FExtensibilityManager> FOSECoreEditor::GetToolBarExtensibilityManager()
{
   return _toolBarExtensibilityManager;
}

void FOSECoreEditor::_RegisterComponentVisualizers()
{
   if (GUnrealEd)
   {

   }
}

void FOSECoreEditor::_UnRegisterComponentVisualizers()
{
   if (GUnrealEd)
   {

   }
}

void FOSECoreEditor::_RegisterClassLayouts()
{

}

void FOSECoreEditor::_UnRegisterClassLayouts()
{
   if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
   {
      FPropertyEditorModule& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

      // Unregister all classes customized by name
      for (auto It = _registeredClassNames.CreateConstIterator(); It; ++It)
      {
         if (It->IsValid())
         {
            propertyModule.UnregisterCustomClassLayout(*It);
         }
      }

      propertyModule.NotifyCustomizationModuleChanged();
   }
}

void FOSECoreEditor::_RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
   check(ClassName != NAME_None);
   _registeredClassNames.Add(ClassName);

   FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FOSECoreEditor::_RegisterPropertyTypeLayouts()
{
   _RegisterCustomPropertyTypeLayout(FOSELineParams::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOSELineParamsCustomization::MakeInstance));
   _RegisterCustomPropertyTypeLayout(StaticEnum<EOSEInterpMode>()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOSEInterpModeCustomization::MakeInstance));
}

void FOSECoreEditor::_UnRegisterPropertyTypeLayouts()
{
   if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
   {
      // Unregister all property types customized by name
      FPropertyEditorModule& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
      for (const FName& name : _registeredPropertyTypeNames)
      {
         propertyModule.UnregisterCustomPropertyTypeLayout(name);
      }
      propertyModule.NotifyCustomizationModuleChanged();
   }
}

void FOSECoreEditor::_RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance DetailLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier)
{
   check(PropertyTypeName != NAME_None);
   _registeredPropertyTypeNames.Add(PropertyTypeName);
   FPropertyEditorModule& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   propertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, DetailLayoutDelegate, Identifier);
}

void FOSECoreEditor::_RegisterAssetTools()
{
   IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_OSEVoiceOverConversation()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_OSEVoiceOverLine()));
   _RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_OSEVoiceOverBucket()));
}

void FOSECoreEditor::_RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
   AssetTools.RegisterAssetTypeActions(Action);
   _registeredAssetTypeActions.Add(Action);
}

void FOSECoreEditor::_UnRegisterAssetTools()
{
   FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

   if (AssetToolsModule != nullptr)
   {
      IAssetTools& AssetTools = AssetToolsModule->Get();

      for (auto Action : _registeredAssetTypeActions)
      {
         AssetTools.UnregisterAssetTypeActions(Action);
      }
   }
}

void FOSECoreEditor::_RegisterMenuExtensions()
{
   _menuExtensibilityManager = MakeShareable(new FExtensibilityManager);
   _toolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
}

void FOSECoreEditor::_UnRegisterMenuExtensions()
{
   _menuExtensibilityManager.Reset();
   _toolBarExtensibilityManager.Reset();
}

void FOSECoreEditor::_RegisterStyles()
{
   // no re-registration
   if (_slateStyleSet.IsValid())
      return;

   // Create a new style set
   _slateStyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

   const UOSECoreEditorSettings& editorSettings = UOSECoreEditorSettings::Get();
   for (const FOSECustomClassIcon& customClassIcon : editorSettings.CustomClassIcons)
   {
      // NOTE: Intentionally not resolving the class here with a .LoadSynchronous(),
      // since these settings could be populated at the project level and the module is unlikely
      // to be loaded at this time
      FString className = customClassIcon.Class.GetAssetName();

      if (UTexture2D* thumbnailTexture = customClassIcon.ThumbnailImage.LoadSynchronous())
      {
         // this property name is ue4 magic
         FString propertyName = FString::Printf(TEXT("ClassThumbnail.%s"), *className);
         const FVector2D size = FVector2D(thumbnailTexture->GetSizeX(), thumbnailTexture->GetSizeY());
         FSlateImageBrush* brush = new FSlateImageBrush(thumbnailTexture, size);
         _slateStyleSet->Set(FName(propertyName), brush);
      }

      if (UTexture2D* classIconTexture = customClassIcon.ClassImage.LoadSynchronous())
      {
         // this property name is ue4 magic
         FString propertyName = FString::Printf(TEXT("ClassIcon.%s"), *className);
         const FVector2D size = FVector2D(16, 16); // these need to be nice and small otherwise the editor does crazy things
         FSlateImageBrush* brush = new FSlateImageBrush(classIconTexture, size);
         _slateStyleSet->Set(FName(propertyName), brush);
      }
   }

   FSlateStyleRegistry::RegisterSlateStyle(*_slateStyleSet.Get());
}

void FOSECoreEditor::_UnRegisterStyles()
{
   if (_slateStyleSet.Get())
   {
      FSlateStyleRegistry::UnRegisterSlateStyle(_slateStyleSet->GetStyleSetName());
      _slateStyleSet = nullptr;
   }
}

void FOSECoreEditor::_RegisterOSEBaseEditorTools()
{
   FPropertyEditorModule& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   propertyModule.RegisterCustomClassLayout("OSEBaseEditorTool", FOnGetDetailCustomizationInstance::CreateStatic(&FOSEBaseEditorToolCustomization::MakeInstance));
   propertyModule.NotifyCustomizationModuleChanged();
}

void FOSECoreEditor::_UnRegisterOSEBaseEditorTools()
{
   if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
   {
      FPropertyEditorModule& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
      propertyModule.UnregisterCustomClassLayout("OSEBaseEditorTool");
      propertyModule.NotifyCustomizationModuleChanged();
   }
}

void FOSECoreEditor::_AddOSEMenuBar()
{
   if (!_OSEMenuExtender)
   {
      _OSEMenuExtender = MakeShareable(new FExtender);
      _OSEMenuExtender->AddMenuBarExtension(
         TEXT("Help"),
         EExtensionHook::After,
         nullptr,
         FMenuBarExtensionDelegate::CreateRaw(this, &FOSECoreEditor::_AddOSEMenuBarExtension));

      if (FLevelEditorModule* levelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
         levelEditor->GetMenuExtensibilityManager()->AddExtender(_OSEMenuExtender);
   }
}

void FOSECoreEditor::_AddOSEMenuBarExtension(FMenuBarBuilder& menuBarBuilder)
{
   menuBarBuilder.AddPullDownMenu(
      FText::FromString(TEXT("OSE Editor Tools")),
      FText::FromString(TEXT("OSE cross-project shared tools.")),
      FNewMenuDelegate::CreateLambda([this](FMenuBuilder& menuBuilder)
         {
            // World Settings tools
            menuBuilder.BeginSection(NAME_None, FText::FromString("World Settings"));
            menuBuilder.AddMenuEntry(
               FText::FromString("Delete Stale WorldSettings"),
               FText::FromString("Deletes any WorldSettings object that isn't the current WorldSettings class"),
               FSlateIcon(),
               FExecuteAction::CreateRaw(this, &FOSECoreEditor::_DeleteStaleWorldSettings)
            );
            menuBuilder.EndSection();

            OnPullDownMenuCreated.ExecuteIfBound(menuBuilder);
         }),
      TEXT("OSE"));
}

void FOSECoreEditor::_AddOSEContentBrowserMenuOptions()
{
   FContentBrowserModule* contentBrowser = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
   if (!contentBrowser)
      return;

   // Add entries to the ContentBrowser context menu, for when you right-click a directory in the tree
   // SelectedPaths will have the current directory.
   auto& pathExtenders = contentBrowser->GetAllPathViewContextMenuExtenders();
   pathExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda(
      [](const TArray<FString>& selectedPaths)
      {
         TSharedRef<FExtender> extender = MakeShared<FExtender>();
         extender->AddMenuExtension(
            "PathContextBulkOperations", EExtensionHook::After,
            TSharedPtr<FUICommandList>(),
            FMenuExtensionDelegate::CreateLambda(
               [selectedPaths](FMenuBuilder& menuBuilder)
               {
                  menuBuilder.BeginSection("OSE", FText::FromString("OSE"));
                  menuBuilder.AddMenuEntry(
                     FText::FromString(TEXT("Fix up import paths")),
                     FText::FromString(TEXT("Tries to fix up any art/audio assets that have broken paths")),
                     FSlateIcon(),
                     FUIAction(FExecuteAction::CreateLambda(
                        [selectedPaths]()
                        {
                           for(const FString& path : selectedPaths)
                           {
                              FixupArtAbsolutePaths::FixupArtContentAbsolutePathsFromDir(path);
                           }
                        }
                     ))
                  );
                  menuBuilder.EndSection();
               }
            )
         );

         return extender;
      }
   ));

   // Add entries to the ContentBrowser context menu, for when you right-click on a selected asset (or multiple assets).
   TArray<FContentBrowserMenuExtender_SelectedAssets>& assetExtenders = contentBrowser->GetAllAssetViewContextMenuExtenders();
   assetExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
      [](const TArray<FAssetData>& selectedAssets)
      {
         TSharedRef<FExtender> extender = MakeShared<FExtender>();

         extender->AddMenuExtension(
            "CommonAssetActions", EExtensionHook::Before,
            TSharedPtr<FUICommandList>(),
            FMenuExtensionDelegate::CreateLambda(
               [selectedAssets](FMenuBuilder& menuBuilder)
               {
                  menuBuilder.BeginSection("OSE", FText::FromString("OSE"));
                  
                  /*
                  menuBuilder.AddMenuEntry(
                     FText::FromString(TEXT("My Action")),
                     FText::FromString(TEXT("My Action Help Text")),
                     FSlateIcon(),
                     FUIAction(FExecuteAction::CreateLambda(
                        [selectedAssets, this]()
                        {
                           // write some code to do things!
                        }
                     ))
                  );
                  menuBuilder.EndSection();
                  */
               }
            )
         );
         return extender;
      }
   ));
}

void FOSECoreEditor::_DeleteStaleWorldSettings()
{
   FWorldContext& worldContext = GEditor->GetEditorWorldContext();
   if (UWorld* world = worldContext.World())
   {
      AWorldSettings* actualWorldSettings = world->GetWorldSettings();

      for (TActorIterator<AActor> it(world, AWorldSettings::StaticClass()); it; ++it)
      {
         AActor* actor = *it;
         
         if (actor != actualWorldSettings)
         {
            UE_LOG(LogOSECoreEditor, Log, TEXT("%s: Destroying world settings actor %s, resave the map to commit this change!"), ANSI_TO_TCHAR(__FUNCTION__), *actor->GetName());
            actor->Destroy();
         }
      }
   }
   else
   {
      UE_LOG(LogOSECoreEditor, Error, TEXT("%s: No world?"), ANSI_TO_TCHAR(__FUNCTION__));
   }
}

// Editor audit commands

struct FFunctionAudit
{
   const UFunction* Function = nullptr;
   TSet<FName> ReferencingPackages;
};

struct FClassAudit
{
   const UClass* Class = nullptr;
   TMap<FName, FFunctionAudit> FunctionMap;
   TSet<FName> ReferencingPackages;
};

struct FModuleAudit
{
   const UPackage* CodePackage = nullptr;
   TMap<FName, FClassAudit> ClassMap;
   TArray<FName> ReferencingPackages;
};

static FString GetAuditPath(FName PackageName, FName ClassName, FName FieldName)
{
   FString ReturnString = PackageName.ToString() + TEXT(".") + ClassName.ToString();
   if (FieldName != NAME_None)
   {
      ReturnString += TEXT(":");
      ReturnString += FieldName.ToString();
   }
   return ReturnString;
}

static void ReportRefs(TArray<FString>& OutReport, const TSet<FName>& ReferencingPackages, const FString& Prefix, int32 MaxPrint = 15)
{
   int32 Printed = 0;
   for (const FName& PackageName : ReferencingPackages)
   {
      FString ReturnString = Prefix;
      if (Printed < MaxPrint)
      {
         ReturnString += PackageName.ToString();
         OutReport.Add(ReturnString);
         Printed++;
      }
      else
      {
         ReturnString += FString::Printf(TEXT(" + %d more"), ReferencingPackages.Num() - MaxPrint);
         OutReport.Add(ReturnString);
         return;
      }
   }
}

// Copy/paste of UAssetManager::WriteCustomReport, which I think is no longer protected in latest
static bool WriteCustomReport(FString FileName, TArray<FString>& FileLines)
{
   // Has a report been generated
   bool ReportGenerated = false;

   // Ensure we have a log to write
   if (FileLines.Num())
   {
      // Create the file name      
      FString FileLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() + TEXT("Reports/"));
      FString FullPath = FString::Printf(TEXT("%s%s"), *FileLocation, *FileName);

      // save file
      FArchive* LogFile = IFileManager::Get().CreateFileWriter(*FullPath);

      if (LogFile != nullptr)
      {
         for (int32 Index = 0; Index < FileLines.Num(); ++Index)
         {
            FString LogEntry = FString::Printf(TEXT("%s"), *FileLines[Index]) + LINE_TERMINATOR;
            LogFile->Serialize(TCHAR_TO_ANSI(*LogEntry), LogEntry.Len());
         }

         LogFile->Close();
         delete LogFile;

         // A report has been generated
         ReportGenerated = true;
      }
   }

   return ReportGenerated;
}


static void AuditCodeCoverage()
{
   // TODO might need to modify this for 4.25 property changes, not doing property refs until then

   if (!UAssetManager::IsInitialized())
   {
      return;
   }

   UAssetManager& manager = UAssetManager::Get();
   IAssetRegistry& registry = manager.GetAssetRegistry();
   
   TMap<FName, FModuleAudit> moduleMap;
   TSet<FName> allPackages;

   FScopedSlowTask slowTask(0, LOCTEXT("BuildingCodeAudit", "Performing Code Coverage Audit"));
   const bool showCancelButton = false;
   const bool allowInPIE = true;
   slowTask.MakeDialog(showCancelButton, allowInPIE);

   UE_LOG(LogOSECoreEditor, Log, TEXT("Performing Code Coverage Audit:"));

   // First get all functions in modules we care about
   for (TObjectIterator<UPackage> it; it; ++it)
   {
      UPackage* package = *it;
      check(package);

      if (package->HasAnyPackageFlags(PKG_CompiledIn) && !package->HasAnyPackageFlags(PKG_EditorOnly))
      {
         FString packageName = package->GetName();
         static const TCHAR* scriptString = TEXT("/Script/");
         
         // Handle C++ classes specially, as FPackageName::LongPackageNameToFilename won't return the correct path in this case
         const FString moduleName = packageName.RightChop(FCString::Strlen(scriptString));
         FString modulePath;
         if (FSourceCodeNavigation::FindModulePath(moduleName, modulePath))
         {
            // TODO fixup with real OSE plugin path
            if (!modulePath.Contains(TEXT("/Engine/")) || modulePath.Contains(TEXT("/Plugins/OSE/")))
            {
               FModuleAudit& moduleAudit = moduleMap.Add(package->GetFName());

               moduleAudit.CodePackage = package;
               registry.GetReferencers(package->GetFName(), moduleAudit.ReferencingPackages);

               UE_LOG(LogOSECoreEditor, Log, TEXT("Found game code package %s, located at %s with %d refs"), *packageName, *modulePath, moduleAudit.ReferencingPackages.Num());

               allPackages.Append(moduleAudit.ReferencingPackages);

               TArray<UObject*> packageObjects;
               GetObjectsWithOuter(package, packageObjects, false);
               for(UObject* object : packageObjects)
               {
                  UClass* const currentClass = Cast<UClass>(object);
                  if(currentClass)
                  {
                     FClassAudit& classAudit = moduleAudit.ClassMap.Add(currentClass->GetFName());
                     classAudit.Class = currentClass;
                     
                     // Doing it this way to get functions in declaration order for easier cross referencing
                     for (const UFunction* func : TFieldRange<UFunction>(currentClass, EFieldIteratorFlags::ExcludeSuper))
                     {
                        FFunctionAudit& functionAudit = classAudit.FunctionMap.Add(func->GetFName());
                        functionAudit.Function = func;
                     }
                  }
               }
            }
         }
      }
   }

   // Now go through each content package looking for refs
   for (const FName& packageName : allPackages)
   {
      UPackage* foundPackage = FindPackage(nullptr, *packageName.ToString());
      UPackage* packageToReset = nullptr;
      FLinkerLoad* linker = nullptr;

      if (!foundPackage)
      {
         // Try a full load first, this is needed to get k2nodes
         foundPackage = LoadPackage(nullptr, *packageName.ToString(), LOAD_Quiet | LOAD_NoWarn);
      }

      if (foundPackage)
      {
         // Use linker if it's valid, otherwise forcibly load just the linker header
         linker = foundPackage->GetLinker();
      }

      if (!linker)
      {
         // Load package header but not the rest
         FPackagePath packagePath;
         if (FPackagePath::TryFromPackageName(packageName, packagePath))
         {
            linker = GetPackageLinker(nullptr, packagePath, LOAD_Quiet | LOAD_NoWarn, nullptr);
            if (linker != nullptr && linker->LinkerRoot != nullptr)
            {
               packageToReset = linker->LinkerRoot;
            }
         }
      }

      if (linker)
      {
         // TODO do we need to handle soft refs?

         // Go over imports
         for (const FObjectImport& foundImport : linker->ImportMap)
         {
            if (foundImport.ClassName == NAME_Class && foundImport.ClassPackage == GLongCoreUObjectPackageName)
            {
               // We found a class import
               FName refClassName = foundImport.ObjectName;
               FObjectImport* packageImport = linker->ImpPtr(foundImport.OuterIndex);
               if (packageImport && packageImport->ClassName == NAME_Package)
               {
                  FName refPackageName = packageImport->ObjectName;

                  FModuleAudit* foundModuleAudit = moduleMap.Find(refPackageName);
                  if (foundModuleAudit)
                  {
                     // This is one of our audited modules, so add
                     FClassAudit& classAudit = foundModuleAudit->ClassMap.FindOrAdd(refClassName);
                     classAudit.ReferencingPackages.Add(packageName);
                  }
               }
            }

            if (foundImport.ClassName == NAME_Function && foundImport.ClassPackage == GLongCoreUObjectPackageName)
            {
               // We found a function import
               FName refFunctionName = foundImport.ObjectName;
               
               FObjectImport* classImport = linker->ImpPtr(foundImport.OuterIndex);
               if (classImport && classImport->ClassName == NAME_Class)
               {
                  FName refClassName = classImport->ObjectName;
                  FObjectImport* packageImport = linker->ImpPtr(classImport->OuterIndex);
                  if (packageImport && packageImport->ClassName == NAME_Package)
                  {
                     FName refPackageName = packageImport->ObjectName;

                     FModuleAudit* foundModuleAudit = moduleMap.Find(refPackageName);
                     if (foundModuleAudit)
                     {
                        // This is one of our audited modules, so add
                        FClassAudit& classAudit = foundModuleAudit->ClassMap.FindOrAdd(refClassName);
                        FFunctionAudit& functionAudit = classAudit.FunctionMap.FindOrAdd(refFunctionName);
                        functionAudit.ReferencingPackages.Add(packageName);
                     }
                  }
               }
            }
         }
      }

      if (packageToReset)
      {
         ResetLoaders(packageToReset);
      }
   }

   // Need to explicitly look for function call nodes, unfortunately these don't always show up in the linker table
   for (TObjectIterator<UK2Node> it; it; ++it)
   {
      UK2Node* node = *it;
      UPackage* nodePackage = node->GetOutermost();
      UFunction* foundFunction = nullptr;
      
      if (!nodePackage || nodePackage->HasAnyFlags(RF_Transient))
      {
         continue;
      }

      if (UK2Node_CallFunction* callFunctionNode = Cast<UK2Node_CallFunction>(node))
      {
         foundFunction = callFunctionNode->GetTargetFunction();
      }
      else if (UK2Node_Event* eventNode = Cast<UK2Node_Event>(node))
      {
         foundFunction = eventNode->FindEventSignatureFunction();
      }
      else if (UK2Node_FunctionTerminator* terminatorNode = Cast<UK2Node_FunctionTerminator>(node))
      {
         foundFunction = terminatorNode->FunctionReference.ResolveMember<UFunction>(terminatorNode->GetBlueprintClassFromNode());
      }
      /*else if (UK2Node_BaseAsyncTask* AsyncNode = Cast<UK2Node_BaseAsyncTask>(Node))
      {
         // This doesn't seem to be accessible, but should show up as a derived call function
      } */

      if (foundFunction)
      {
         UPackage* functionPackage = foundFunction->GetOutermost();
         FModuleAudit* foundModuleAudit = moduleMap.Find(functionPackage->GetFName());
         if (foundModuleAudit)
         {
            // This is one of our audited modules, so add
            
            UClass* outerClass = Cast<UClass>(foundFunction->GetOuter());
            if (outerClass)
            {
               // This will skip references to delegates signatures, not sure what to do with those
               FName refClassName = outerClass->GetFName();
               FName refFunctionName = foundFunction->GetFName();
            
               FClassAudit& classAudit = foundModuleAudit->ClassMap.FindOrAdd(refClassName);
               FFunctionAudit& functionAudit = classAudit.FunctionMap.FindOrAdd(refFunctionName);

               functionAudit.ReferencingPackages.Add(nodePackage->GetFName());
            }
         }
      }
   }

   // Now write out report
   TArray<FString> reportLines;
   reportLines.Add(FString::Printf(TEXT("Code Coverage Report:")));
   reportLines.Add(FString());

   TArray<FString> functionsToRemove;

   for (const TPair<FName, FModuleAudit>& modulePair : moduleMap)
   {
      reportLines.Add(FString::Printf(TEXT("Audit Code Module %s:"), *modulePair.Key.ToString()));
      for (const TPair<FName, FClassAudit>& classPair : modulePair.Value.ClassMap)
      {
         FString className = classPair.Key.ToString();
         if (classPair.Value.Class == nullptr)
         {
            className += TEXT(" (MISSING CLASS!)");
         }
         reportLines.Add(FString::Printf(TEXT("\tClass %s, %d refs:"), *classPair.Key.ToString(), classPair.Value.ReferencingPackages.Num()));
         TSet<FName> uniqueRefs = classPair.Value.ReferencingPackages;
         for (const TPair<FName, FFunctionAudit>& functionPair : classPair.Value.FunctionMap)
         {
            FString functionName = functionPair.Key.ToString();
            if (functionPair.Value.Function == nullptr)
            {
               functionName += TEXT(" (MISSING FUNCTION!)");
            }
            reportLines.Add(FString::Printf(TEXT("\t\tFunction %s, %d refs:"), *functionName, functionPair.Value.ReferencingPackages.Num()));
            ReportRefs(reportLines, functionPair.Value.ReferencingPackages, TEXT("\t\t\t"));
            uniqueRefs = uniqueRefs.Difference(functionPair.Value.ReferencingPackages);

            if (functionPair.Value.ReferencingPackages.Num() == 0 && functionPair.Value.Function)
            {
               FString fullPath = GetAuditPath(modulePair.Key, classPair.Key, functionPair.Key);
               FString typeString;
               if (functionPair.Value.Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_BlueprintCallable | EFunctionFlags::FUNC_BlueprintEvent | EFunctionFlags::FUNC_BlueprintPure))
               {
                  typeString = TEXT("(BlueprintCallable)");
               }
               else if (functionPair.Value.Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Net))
               {
                  typeString = TEXT("(Networked)");
               }
               functionsToRemove.Add(FString::Printf(TEXT("\t%s %s"), *fullPath, *typeString));
            }
         }

         if (uniqueRefs.Num() > 0)
         {
            reportLines.Add(TEXT("\t\tOther refs:"));
            ReportRefs(reportLines, uniqueRefs, TEXT("\t\t\t"));
         }
      }
      reportLines.Add(FString());
   }

   reportLines.Add(FString::Printf(TEXT("Functions with no asset references:")));
   reportLines.Append(functionsToRemove);

   FString reportFilename = FString::Printf(TEXT("CodeAudit%s.txt"), *FDateTime::Now().ToString());
   if (WriteCustomReport(reportFilename, reportLines))
   {
      UE_LOG(LogOSECoreEditor, Log, TEXT("Wrote audit report to Saved/Reports/%s"), *reportFilename);
   }
   else
   {
      UE_LOG(LogOSECoreEditor, Error, TEXT("Failed to write audit report to Saved/Reports/%s!"), *reportFilename);
   }
}

static FAutoConsoleCommand CVarDumpAssetTypeSummary(
   TEXT("OSE.AuditCodeCoverage"),
   TEXT("Writes out a report that analyzes the usage of all OSE/game specific modules"),
   FConsoleCommandDelegate::CreateStatic(AuditCodeCoverage),
   ECVF_Cheat);


#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOSECoreEditor, OSECoreEditor)
