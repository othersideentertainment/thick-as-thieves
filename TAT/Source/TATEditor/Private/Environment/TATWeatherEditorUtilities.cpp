// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherEditorUtilities.h"

// tat
#include "GameFramework/TATWorldSettings.h"

// ue
#include "CoreMinimal.h"
#include "ClassIconFinder.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorFontGlyphs.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "PropertyEditorModule.h"
#include "Selection.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Styling/StyleColors.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherEditorUtilities, Log, All);

static constexpr const TCHAR* kBlueprintComponentNameSuffix = TEXT("_GEN_VARIABLE");

namespace WeatherEditorUtilities
{
   AActor* GetEditorSelectedActor()
   {
      USelection* selection = GEditor->GetSelectedActors();
      if (selection && selection->Num() == 1)
      {
         return Cast<AActor>(selection->GetSelectedObject(0));
      }
      return nullptr;
   }

   UBlueprint* FindBlueprintAssetForClass(UClass* cls)
   {
      return (cls != nullptr) ? Cast<UBlueprint>(cls->ClassGeneratedBy) : nullptr;
   }

   bool FindAssetInContentBrowser(UClass* cls)
   {
      if (UBlueprint* blueprintCls = FindBlueprintAssetForClass(cls))
      {
         const FAssetData blueprintAsset{ blueprintCls, FAssetData::ECreationFlags::AllowBlueprintClass };
         FContentBrowserModule& contentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
         contentBrowserModule.Get().SyncBrowserToAssets({ blueprintAsset });
         return true;
      }
      return false;
   }

   bool OpenAssetEditorForClass(UClass* cls)
   {
      if (UAssetEditorSubsystem* assetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
      {
         if (UBlueprint* blueprintCls = FindBlueprintAssetForClass(cls))
         {
            return assetEditorSubsystem->OpenEditorForAsset(blueprintCls);
         }
      }
      return false;
   }

   bool OpenAssetEditorForDataTable(UDataTable* dataTable)
   {
      if (UAssetEditorSubsystem* assetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
      {
         return assetEditorSubsystem->OpenEditorForAsset(dataTable);
      }
      return false;
   }

   void CompileBlueprintClassAsset(UClass* cls)
   {
      if (UBlueprint* blueprintCls = FindBlueprintAssetForClass(cls))
      {
         FKismetEditorUtilities::CompileBlueprint(blueprintCls, EBlueprintCompileOptions::SkipSave);
      }
   }

   bool IsBlueprintClassAssetDirty(UClass* cls)
   {
      if (cls != nullptr)
      {
         if (UPackage* package = cls->GetPackage())
         {
            return package->IsDirty();
         }
      }
      return false;
   }

   bool SaveBlueprintClassAsset(UClass* cls)
   {
      if (UBlueprint* blueprintCls = FindBlueprintAssetForClass(cls))
      {
         if (UPackage* package = blueprintCls->GetPackage())
         {
            // Compile the blueprint first to avoid data not updating correctly (which can happen with structs for some reason)
            FKismetEditorUtilities::CompileBlueprint(blueprintCls, EBlueprintCompileOptions::SkipSave);

            TArray<UPackage*> packagesToSave{ package };
            constexpr bool checkDirty = false;
            constexpr bool promptToSave = false;
            const FEditorFileUtils::EPromptReturnCode returnCode = FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, checkDirty, promptToSave);
            return returnCode == FEditorFileUtils::PR_Success;
         }
      }
      return false;
   }

   bool GetPropertyValueAsString(UObject* obj, FName propName, FString& outPropValueString)
   {
      outPropValueString.Reset();

      check(obj != nullptr);
      UClass* cls = obj->GetClass();
      check(cls != nullptr);

      const FProperty* prop = cls->FindPropertyByName(propName);
      if (prop == nullptr)
      {
         return false;
      }

      const void* valuePtr = prop->ContainerPtrToValuePtr<void>(obj);
      check(valuePtr != nullptr);

      prop->ExportTextItem_Direct(outPropValueString, valuePtr, nullptr, obj, 0);
      return true;
   }

   bool SetPropertyValueAsString(UObject* obj, FName propName, const FString& newPropValue, FString* outErrorMessage)
   {
      check(obj != nullptr);
      UClass* cls = obj->GetClass();
      check(cls != nullptr);

      FProperty* prop = cls->FindPropertyByName(propName);
      if (prop == nullptr)
      {
         if (outErrorMessage != nullptr)
         {
            *outErrorMessage = FString::Printf(TEXT("Object '%s' has no such property named '%s'"), *obj->GetName(), *propName.ToString());
         }
         return false;
      }

      void* valuePtr = prop->ContainerPtrToValuePtr<void>(obj);
      check(valuePtr != nullptr);

      // Set the value by string in the same way that the editor allows copying property values by string
      // (eg. if this property was an FVector, this would accept the string "(X=0,Y=0,Z=0)")
      FStringOutputDevice errors;
      prop->ImportText_Direct(*newPropValue, valuePtr, obj, 0, &errors);
      if (outErrorMessage != nullptr)
      {
         *outErrorMessage = errors;
      }
      return errors.Len() == 0;
   }

   int32 CopyComponentProps(UActorComponent* targetComponent, UActorComponent* sourceComponent, TFunctionRef<bool(const FProperty*)> shouldCopyProp)
   {
      check(targetComponent != nullptr);
      check(sourceComponent != nullptr);
      TSubclassOf<UActorComponent> componentClass = targetComponent->GetClass();
      check(componentClass != nullptr);
      check(sourceComponent->IsA(componentClass));

      UE_LOG(LogTATWeatherEditorUtilities, Log, TEXT("Copy '%s' -> '%s'"), *sourceComponent->GetName(), *targetComponent->GetName());

      int32 numSetProps = 0;

      FString propValueString;
      FString propSetError;
      for (TPropertyValueIterator<FProperty> propIter(componentClass, sourceComponent, EPropertyValueIteratorFlags::NoRecursion); propIter; ++propIter)
      {
         const FProperty* srcProp = propIter->Key;
         check(srcProp != nullptr);
         if (!srcProp->HasAnyPropertyFlags(CPF_Edit) || srcProp->HasAnyPropertyFlags(CPF_Transient | CPF_InstancedReference | CPF_Deprecated))
         {
            continue;
         }
         if (!shouldCopyProp(srcProp))
         {
            continue;
         }
         const FName propName = srcProp->GetFName();

         if (GetPropertyValueAsString(sourceComponent, propName, propValueString))
         {
            if (SetPropertyValueAsString(targetComponent, propName, propValueString, &propSetError))
            {
               UE_LOG(LogTATWeatherEditorUtilities, Log, TEXT("Copying prop '%s' from %s to %s"), *propName.ToString(), *sourceComponent->GetName(), *targetComponent->GetName());
               ++numSetProps;
            }
            else
            {
               UE_LOG(LogTATWeatherEditorUtilities, Error, TEXT("Error copying prop '%s' from %s to %s: %s"), *propName.ToString(), *sourceComponent->GetName(), *targetComponent->GetName(), *propSetError);
            }
         }
      }

      return numSetProps;
   }

   int32 CopyComponentPropsToCDO(TSubclassOf<UActorComponent> componentClass, TSubclassOf<AActor> destClass, AActor* sourceActor, TFunctionRef<bool(const FProperty*)> shouldCopyProp)
   {
      check(componentClass != nullptr);
      check(destClass != nullptr);
      check(sourceActor != nullptr);

      AActor* actorCDO = destClass->GetDefaultObject<AActor>();
      check(actorCDO != nullptr);

      UActorComponent* sourceComponent = sourceActor->FindComponentByClass(componentClass);
      if (sourceComponent == nullptr)
      {
         return 0;
      }

      UActorComponent* targetComponent = actorCDO->FindComponentByClass(componentClass);
      if (targetComponent == nullptr)
      {
         return 0;
      }

      const int32 numSetProps = CopyComponentProps(targetComponent, sourceComponent, shouldCopyProp);
      if (numSetProps > 0)
      {
         actorCDO->MarkPackageDirty();
      }
      return numSetProps;
   }

   AActor* FindFirstActorOfTypeInPersistentLevel(UClass* actorClass, UWorld* world)
   {
      check(actorClass != nullptr);
      check(actorClass->IsChildOf(AActor::StaticClass()));
      if (world == nullptr)
      {
         world = GEditor->GetEditorWorldContext().World();
      }
      if (world == nullptr)
      {
         return nullptr;
      }
      ULevel* level = world->PersistentLevel;
      if (level == nullptr)
      {
         return nullptr;
      }

      // special case for world settings
      if (actorClass->IsChildOf(AWorldSettings::StaticClass()))
      {
         constexpr bool checked = false;
         AWorldSettings* worldSettings = level->GetWorldSettings(checked);
         if (worldSettings != nullptr && worldSettings->IsA(actorClass))
         {
            return worldSettings;
         }
      }

      for (AActor* actor : level->Actors)
      {
         if (actor != nullptr && actor->IsA(actorClass))
         {
            return actor;
         }
      }

      return nullptr;
   }

   void ForEachComponentInActorCDO(TSubclassOf<AActor> actorClass, TFunctionRef<void(UActorComponent*, const FObjectProperty*, USCS_Node*)> callback)
   {
      if (actorClass == nullptr)
      {
         return;
      }

      AActor* cdo = actorClass->GetDefaultObject<AActor>();
      if (cdo == nullptr)
      {
         return;
      }

      TSet<UObject*, DefaultKeyFuncs<UObject*>, TInlineSetAllocator<16>> visitedComponents;

      for (TPropertyValueIterator<FObjectProperty> propIt(actorClass, cdo, EPropertyValueIteratorFlags::NoRecursion); propIt; ++propIt)
      {
         const FObjectProperty* prop = propIt->Key;
         check(prop != nullptr);
         if (UActorComponent* comp = Cast<UActorComponent>(prop->GetPropertyValue(propIt->Value)))
         {
            if (!visitedComponents.Contains(comp))
            {
               callback(comp, prop, nullptr);
               visitedComponents.Add(comp);
            }
         }
      }

      UBlueprintGeneratedClass* blueprintClass = Cast<UBlueprintGeneratedClass>(actorClass);
      while (blueprintClass != nullptr)
      {
         USimpleConstructionScript* classSCS = blueprintClass->SimpleConstructionScript;
         if (classSCS)
         {
            for (USCS_Node* node : classSCS->GetAllNodes())
            {
               if (node->ComponentTemplate != nullptr && !visitedComponents.Contains(node->ComponentTemplate))
               {
                  callback(node->ComponentTemplate, nullptr, node);
                  visitedComponents.Add(node->ComponentTemplate);
               }
            }
         }

         if (UInheritableComponentHandler* ich = blueprintClass->InheritableComponentHandler)
         {
            for (auto recordIter = ich->CreateRecordIterator(); recordIter; ++recordIter)
            {
               const FComponentOverrideRecord& record = *recordIter;
               if (record.ComponentTemplate != nullptr && !visitedComponents.Contains(record.ComponentTemplate))
               {
                  callback(record.ComponentTemplate, nullptr, record.ComponentKey.FindSCSNode());
                  visitedComponents.Add(record.ComponentTemplate);
               }
            }
         }

         blueprintClass = Cast<UBlueprintGeneratedClass>(blueprintClass->GetSuperClass());
      }
   }

   FText MakeBlueprintAssetLabel(UClass* cls)
   {
      if (cls == nullptr)
      {
         return FText::GetEmpty();
      }
      FString name = cls->GetName();
      name.RemoveFromEnd(TEXT("_C"));
      return FText::FromString(name);
   }

   FText MakeBlueprintAssetLabel(const FSoftObjectPath& path)
   {
      if (UClass* cls = Cast<UClass>(path.ResolveObject()))
      {
         return MakeBlueprintAssetLabel(cls);
      }
      FString name = path.GetAssetName();
      name.RemoveFromEnd(TEXT("_C"));
      return FText::FromString(name);
   }

   const FSlateBrush* GetIconBrush(TSubclassOf<UObject> cls)
   {
      TSubclassOf<AActor> actorCls(cls);
      if (actorCls != nullptr)
      {
         if (AActor* actorCDO = actorCls->GetDefaultObject<AActor>())
         {
            return FClassIconFinder::FindIconForActor(actorCDO);
         }
      }
      return FSlateIconFinder::FindIconBrushForClass(cls);
   }

   TSharedRef<SWidget> MakeFontAwesomeIcon(const FText& icon, const TOptional<FText>& tooltipText, const TOptional<FLinearColor>& iconColor)
   {
      return SNew(SBox)
         .VAlign(VAlign_Center)
         .HAlign(HAlign_Center)
         [
            SNew(STextBlock)
            .Text(icon)
            .Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
            .ColorAndOpacity(iconColor ? FSlateColor(*iconColor) : FSlateColor(EStyleColor::Foreground))
            .ToolTipText(tooltipText.Get(FText::GetEmpty()))
         ];
   }

   TSharedRef<SWidget> MakeFontAwesomeIcon(TCHAR codepoint, const TOptional<FText>& tooltipText, const TOptional<FLinearColor>& iconColor)
   {
      const TCHAR iconString[] = { codepoint, TEXT('\0') };
      return MakeFontAwesomeIcon(FText::FromString(iconString), tooltipText, iconColor);
   }

   TSharedRef<SWidget> MakeSimpleButton(const FText& label, const FText& tooltipText, const TFunction<void()>& onClickCallback, float margin)
   {
      return SNew(SBox)
         .Padding(FMargin(margin))
         [
            SNew(SButton)
            .Text(label)
            .ToolTipText(tooltipText)
            .OnClicked(FOnClicked::CreateLambda([onClickCallback]() -> FReply
            {
               onClickCallback();
               return FReply::Handled();
            }))
         ];
   }

   TSharedRef<SWidget> MakeIconButton(const FText& icon, const FText& tooltipText, const TFunction<void()>& onClickCallback, const TOptional<FLinearColor>& iconColor, float margin)
   {
      return SNew(SBox)
         .Padding(FMargin(margin))
         [
            SNew(SButton)
            .ToolTipText(tooltipText)
            .OnClicked(FOnClicked::CreateLambda([onClickCallback]() -> FReply
            {
               onClickCallback();
               return FReply::Handled();
            }))
            [
               SNew(STextBlock)
               .Text(icon)
               .Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
               .ColorAndOpacity(iconColor ? FSlateColor(*iconColor) : FSlateColor(EStyleColor::Foreground))
            ]
         ];
   }

   TSharedRef<SWidget> MakeIconButton(TCHAR codepoint, const FText& tooltipText, const TFunction<void()>& onClickCallback, const TOptional<FLinearColor>& iconColor, float margin)
   {
      const TCHAR iconString[] = { codepoint, TEXT('\0') };
      return MakeIconButton(FText::FromString(iconString), tooltipText, onClickCallback, iconColor, margin);
   }

   TSharedRef<SWidget> MakeSimpleCheckBox(const FText& label, const FText& tooltipText, TAttribute<bool> isChecked, const TFunction<void(bool)>& onCheckStateChangeCallback)
   {
      TAttribute<ECheckBoxState>::FGetter dynamicValueGetter;
      dynamicValueGetter.BindLambda([](TAttribute<bool> checked) -> ECheckBoxState
      {
         return checked.Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
      }, isChecked);
      return SNew(SBox)
         .Padding(FMargin(2.0f))
         [
            SNew(SCheckBox)
            .IsChecked(TAttribute<ECheckBoxState>::Create(dynamicValueGetter))
            .ToolTipText(tooltipText)
            .OnCheckStateChanged_Lambda([onCheckStateChangeCallback](ECheckBoxState state) { onCheckStateChangeCallback(state == ECheckBoxState::Checked); })
            [
               SNew(STextBlock)
               .Text(label)
            ]
         ];
   }

   TSharedRef<SWidget> MakeVisibilityButton(const FOnClicked& onClickDelegate, TAttribute<bool> isVisible)
   {
      TAttribute<FText>::FGetter dynamicVisibilityGetter;
      dynamicVisibilityGetter.BindLambda([](TAttribute<bool> enabled) -> FText
      {
         return enabled.Get() ? FEditorFontGlyphs::Eye : FEditorFontGlyphs::Eye_Slash;
      }, isVisible);

      TAttribute<FSlateColor>::FGetter dynamicColorGetter;
      dynamicColorGetter.BindLambda([](TAttribute<bool> enabled) -> FSlateColor
      {
         return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, enabled.Get() ? 1.0f : 0.25f));
      }, isVisible);

      return SNew(SButton)
         .OnClicked(onClickDelegate)
         .IsEnabled(true)
         .IsFocusable(false)
         .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
         .ToolTipText(FText::FromString(TEXT("Toggle Visibility")))
         .ContentPadding(2.0f)
         .ForegroundColor(FSlateColor::UseForeground())
         [
            SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
               SNew(STextBlock)
               .Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
               .Text(TAttribute<FText>::Create(dynamicVisibilityGetter))
               .ColorAndOpacity(TAttribute<FSlateColor>::Create(dynamicColorGetter))
            ]
         ];
   }

   void ShowContextMenuAtCursorLocation(const TSharedRef<SWidget>& parentWidget, const TSharedRef<SWidget>& contextMenu)
   {
      FSlateApplication::Get().PushMenu(
         parentWidget,
         FWidgetPath(),
         contextMenu,
         FSlateApplication::Get().GetCursorPos(),
         FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
   }

   TSharedRef<IDetailsView> MakePropertyEditorWidget(UObject* editingObject, const TOptional<FDetailsViewArgs>& args, const TOptional<FIsPropertyVisible>& propVisible)
   {
      FPropertyEditorModule& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
      FDetailsViewArgs viewArgs;
      if (args)
      {
         viewArgs = *args;
      }
      else
      {
         viewArgs.bHideSelectionTip = true;
      }
      TSharedRef<IDetailsView> detailView = propertyEditorModule.CreateDetailView(viewArgs);
      if (propVisible)
      {
         detailView->SetIsPropertyVisibleDelegate(*propVisible);
      }
      detailView->SetObject(editingObject);
      return detailView;
   }

   FSimpleActorEditor::FSimpleActorEditor()
   {
      _detailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
      _detailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;
      _detailsViewArgs.bShowPropertyMatrixButton = false;
   }

   // static
   auto FSimpleActorEditor::_MakeObjectEditorState(UObject* templateObject, const TOptional<FIsPropertyVisible>& isPropertyVisible, int32 indentLevel) -> TSharedPtr<FObjectEditorState>
   {
      TSharedPtr<FObjectEditorState> state = MakeShared<FObjectEditorState>();
      state->Cls = (templateObject != nullptr) ? templateObject->GetClass() : nullptr;
      state->Template = templateObject;
      state->IndentLevel = indentLevel;
      state->IsPropertyVisibleDelegate = isPropertyVisible;
      const bool isActorCDO = templateObject->IsA<AActor>() && templateObject->HasAnyFlags(RF_ClassDefaultObject);
      if (templateObject != nullptr)
      {
         if (isActorCDO)
         {
            state->NameLabel = MakeBlueprintAssetLabel(templateObject->GetClass());
         }
         else
         {
            FString componentName = templateObject->GetName();
            if (componentName.EndsWith(kBlueprintComponentNameSuffix))
            {
               componentName.RemoveFromEnd(kBlueprintComponentNameSuffix);
            }
            state->NameLabel = FText::FromString(componentName);
         }
      }
      if (state->Cls != nullptr && !isActorCDO)
      {
         FString className = state->Cls->GetName();
         if (className.EndsWith(TEXT("_C")))
         {
            className.RemoveFromEnd(TEXT("_C"));
         }
         state->TypeLabel = FText::FromString(className);
      }
      return state;
   }

   void FSimpleActorEditor::SetActorClass(TSubclassOf<AActor> inActorClass, const TOptional<FIsPropertyVisible>& isPropertyVisible, bool autoAddComponents)
   {
      _actorClass = inActorClass;

      _propertyEditorOuter.Reset();
      _objectList.Reset();

      if (inActorClass == nullptr)
      {
         return;
      }

      constexpr int32 topLevelIndent = 0;
      _objectList.Add(_MakeObjectEditorState(inActorClass->GetDefaultObject<AActor>(), isPropertyVisible, topLevelIndent));

      if (autoAddComponents)
      {
         AActor* cdo = _actorClass->GetDefaultObject<AActor>();
         check(cdo != nullptr);
         ForEachComponentInActorCDO(inActorClass,
            [&](UActorComponent* comp, const FObjectProperty* prop, USCS_Node* node)
            {
               check(comp != nullptr);
               if (comp == cdo->GetRootComponent() || comp->IsEditorOnly() || comp->HasAnyFlags(RF_Transient))
               {
                  return;
               }
               constexpr int32 componentIndentLevel = 1;
               AddActorComponentEditor(comp, isPropertyVisible, NullOpt, componentIndentLevel);
            });
      }
   }

   void FSimpleActorEditor::AddActorComponentEditor(UActorComponent* componentTemplate, const TOptional<FIsPropertyVisible>& isPropertyVisible, const TOptional<TFunction<bool()>>& isComponentVisible, int32 indentLevel)
   {
      if (componentTemplate == nullptr)
      {
         return;
      }
      if (_objectList.Num() == 1)
      {
         TSharedPtr<FObjectEditorState> separator = MakeShared<FObjectEditorState>();
         separator->IsSeparator = true;
         _objectList.Add(separator);
      }

      TSharedPtr<FObjectEditorState> newObject = _MakeObjectEditorState(componentTemplate, isPropertyVisible, indentLevel);
      if (isComponentVisible)
      {
         newObject->IsObjectVisible = *isComponentVisible;
      }
      _objectList.Add(newObject);
   }

   TSharedRef<SWidget> FSimpleActorEditor::Construct()
   {
      _propertyEditorOuter = SNew(SBox);

      // Set the first item selected by default
      if (_objectList.Num() > 0)
      {
         _SetPropertyEditor(_objectList[0]);
      }

      return SNew(SSplitter)
         .Orientation(Orient_Vertical)
         .Style(FAppStyle::Get(), "SplitterDark")
         .MinimumSlotHeight(12.0f)
         +SSplitter::Slot()
         .SizeRule(SSplitter::FractionOfParent)
         .Value(0.25f)
         [
            SNew(SScrollBox)
            .Orientation(Orient_Vertical)
            +SScrollBox::Slot()
            [
               SNew(SListView<TSharedPtr<FObjectEditorState>>)
               .Orientation(Orient_Vertical)
               .ListItemsSource(&_objectList)
               .SelectionMode(ESelectionMode::SingleToggle)
               .OnSelectionChanged_Lambda([this](TSharedPtr<FObjectEditorState> item, ESelectInfo::Type selectionType)
               {
                  _SetPropertyEditor(item);
               })
               .OnGenerateRow_Lambda([this](const TSharedPtr<FObjectEditorState>& item, const TSharedRef<STableViewBase>& ownerTable) -> TSharedRef<ITableRow>
               {
                  check(item.IsValid());

                  if (item->IsSeparator)
                  {
                     return SNew(STableRow<TSharedPtr<FObjectEditorState>>, ownerTable)
                        .Padding(FMargin(2.0f))
                        .IsEnabled(false)
                        [
                           SNew(SSeparator)
                           .Orientation(Orient_Horizontal)
                        ];
                  }

                  check(item->Cls != nullptr);
                  TWeakPtr<FObjectEditorState> weakItem = item;
                  return SNew(STableRow<TSharedPtr<FObjectEditorState>>, ownerTable)
                     .Padding(FMargin(0.0f))
                     [
                        SNew(SHorizontalBox)
                        .Visibility_Lambda([weakItem]() -> EVisibility
                        {
                           TSharedPtr<FObjectEditorState> item = weakItem.Pin();
                           if (item && item->IsObjectVisible)
                           {
                              return item->IsObjectVisible() ? EVisibility::Visible : EVisibility::Collapsed;
                           }
                           return EVisibility::Visible;
                        })
                        +SHorizontalBox::Slot()
                        .FillWidth(0.6f)
                        [
                           SNew(SHorizontalBox)
                           +SHorizontalBox::Slot()
                           .AutoWidth()
                           .Padding(FMargin(10.0f * FMath::Max(0.0f, static_cast<float>(item->IndentLevel)), 0.0f))
                           //[
                           //]
                           +SHorizontalBox::Slot()
                           .AutoWidth()
                           .VAlign(VAlign_Center)
                           .HAlign(HAlign_Center)
                           .Padding(FMargin(4.0f, 4.0f, 8.0f, 4.0f))
                           [
                              SNew(SImage)
                              .Image(GetIconBrush(item->Cls))
                           ]
                           +SHorizontalBox::Slot()
                           .FillWidth(1.0f)
                           .VAlign(VAlign_Center)
                           .HAlign(HAlign_Fill)
                           [
                              SNew(STextBlock)
                              .Text(item->NameLabel)
                              .Justification(ETextJustify::Left)
                           ]
                        ]
                        +SHorizontalBox::Slot()
                        .FillWidth(0.4f)
                        .VAlign(VAlign_Center)
                        .HAlign(HAlign_Fill)
                        [
                           SNew(STextBlock)
                           .Text(item->TypeLabel)
                           .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
                           .Justification(ETextJustify::Left)
                        ]
                     ]
                  ;
               })
            ]
         ]
         +SSplitter::Slot()
         .SizeRule(SSplitter::FractionOfParent)
         .Value(0.75f)
         [
            _propertyEditorOuter.ToSharedRef()
         ]
      ;
   }

   void FSimpleActorEditor::_SetPropertyEditor(const TSharedPtr<FObjectEditorState>& item)
   {
      if (!_propertyEditorOuter.IsValid())
      {
         return;
      }
      if (item.IsValid())
      {
         TSharedPtr<SWidget> editorWidget;
         if (UObject* objectTemplate = item->Template.Get())
         {
            TSharedRef<IDetailsView> propEditor = MakePropertyEditorWidget(objectTemplate, _detailsViewArgs, item->IsPropertyVisibleDelegate);
            if (_propertyChangedFn)
            {
               propEditor->OnFinishedChangingProperties().AddLambda([this, weakObject = MakeWeakObjectPtr(objectTemplate)](const FPropertyChangedEvent& propertyChangedEvent)
               {
                  UObject* obj = weakObject.Get();
                  if (obj != nullptr && _propertyChangedFn)
                  {
                     _propertyChangedFn(obj, propertyChangedEvent);
                  }
               });
            }
            editorWidget = propEditor;
         }
         else
         {
            editorWidget = SNew(SBox);
         }

         _propertyEditorOuter->SetContent(
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            [
               SNew(STextBlock)
               .Text(item->NameLabel)
            ]
            +SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
               editorWidget.ToSharedRef()
            ]
         );
      }
      else
      {
         _propertyEditorOuter->SetContent(SNew(SBox));
      }
   }

} // namespace WeatherEditorUtilities
