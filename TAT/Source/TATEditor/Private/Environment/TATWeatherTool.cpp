// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherTool.h"

// ose
#include "OSECoreEditor.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherEditorSubsystem.h"
#include "Environment/TATWeatherEditorUtilities.h"
#include "Environment/TATWeatherTypeInfo.h"
#include "Environment/TATWeatherManager.h"
#include "Environment/TATWeatherPreset.h"
#include "GameFramework/TATWorldSettings.h"

// ue
#include "EditorFontGlyphs.h"
#include "ISettingsModule.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "NiagaraComponent.h"
#include "ScopedTransaction.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SListView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherTool)
DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherTool, Log, All);

static const FName kHideInWeatherEditorTag = FName(TEXT("HideInWeatherEditor"));

UTATWeatherTool::UTATWeatherTool()
   : Super()
{
   ToolName = FText::FromString(TEXT("Weather Tool"));
   ToolHelpText = FText::FromString(TEXT("Preview and edit weather presets"));
}

void UTATWeatherTool::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   const FName propertyName = propertyChangedEvent.GetPropertyName();
}

void UTATWeatherTool::InitEditorTool()
{
   Super::InitEditorTool();

   const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
   _linkPreviewAndSelectedPresets = editorSettings.AutoVisibilityDefault;
   _showPresetEditor = editorSettings.ShowPresetEditorDefault;

   _worldAddedDelegateHandle = GEditor->OnWorldAdded().AddLambda([weakThis = MakeWeakObjectPtr(this)](UWorld* newWorld)
   {
      if (newWorld == _GetEditorWorld())
      {
         if (UTATWeatherTool* self = weakThis.Get())
         {
            self->_RefreshWeatherTypes();
         }
      }
   });
}

void UTATWeatherTool::OnEditorToolClosed()
{
   GEditor->OnWorldAdded().Remove(_worldAddedDelegateHandle);

   if (UTATWeatherEditorWorldExtension* worldExt = _GetWeatherWorldExtension())
   {
      worldExt->SetEditorDefaultWeatherPresetForLevel();
   }

   Super::OnEditorToolClosed();
}

TSharedRef<SWidget> UTATWeatherTool::CreateEditorToolWidget()
{
   _RefreshWeatherTypes();

   return SAssignNew(_weatherEditorRootWidget, SVerticalBox)
      +SVerticalBox::Slot()
      .AutoHeight()
      [
         SNew(SHorizontalBox)
         +SHorizontalBox::Slot().AutoWidth()
         [
            WeatherEditorUtilities::MakeIconButton(
               FEditorFontGlyphs::Refresh,
               FText::FromString(TEXT("Rebuilds the weather types list.\nUseful if you have made changes to the weather types data table or custom weather presets (in editor per-user settings).")),
               [this] { _RefreshWeatherTypes(); })
         ]
         +SHorizontalBox::Slot().AutoWidth()
         [
            WeatherEditorUtilities::MakeIconButton(
               WeatherIconGlyph::Image(),
               FText::FromString(TEXT("Regenerate the level's depthmap texture")),
               [] { _RefreshWeatherManagerSceneDepthTexture(); })
         ]
         +SHorizontalBox::Slot().AutoWidth()
         [
            WeatherEditorUtilities::MakeSimpleCheckBox(
               FText::FromString(TEXT("Auto-visibility")),
               FText::FromString(TEXT("Automatically set presets as visible when they are selected")),
               TAttribute<bool>::CreateLambda([this]() { return _linkPreviewAndSelectedPresets; }),
               [this](bool newChecked) { _linkPreviewAndSelectedPresets = newChecked; })
         ]
         +SHorizontalBox::Slot().AutoWidth()
         [
            WeatherEditorUtilities::MakeSimpleCheckBox(
               FText::FromString(TEXT("Edit Mode")),
               FText::FromString(TEXT("Toggle visiblity of the weather tool's builtin preset editor when a preset is selected")),
               TAttribute<bool>::CreateLambda([this]() { return _showPresetEditor; }),
               [this](bool newChecked) { _showPresetEditor = newChecked; })
         ]
         +SHorizontalBox::Slot().AutoWidth()
         [
            WeatherEditorUtilities::MakeSimpleCheckBox(
               FText::FromString(TEXT("Show All")),
               FText::FromString(TEXT("Show weather types that are disabled for this level and not available in-game")),
               TAttribute<bool>::CreateLambda([this]() { return _showDisabledWeatherTypes; }),
               [this](bool newChecked) { _showDisabledWeatherTypes = newChecked; })
         ]
         +SHorizontalBox::Slot()
         .FillWidth(1.0f)
         [
            SNew(SBox) //WeatherEditorUtilities::MakeSimpleButton(TEXT("Open Data Table"), NullOpt, [this]() { _OpenWeatherTypesDataTable(); })
         ]
         +SHorizontalBox::Slot()
         .AutoWidth()
         [
            WeatherEditorUtilities::MakeSimpleButton(FText::FromString(TEXT("Tools")), FText::GetEmpty(), [this]()
            {
               if (!_weatherEditorRootWidget.IsValid())
               {
                  return;
               }

               FMenuBuilder menu(true, nullptr);
               menu.AddMenuEntry(
                  FText::FromString(TEXT("Find Weather Manager Actor")),
                  FText::FromString(TEXT("Selects the Weather Manager actor in the editor viewport")),
                  FSlateIcon(FOSECoreEditor::GetStyleSetName(), "ClassIcon.TATWeatherManager"),
                  FUIAction(FExecuteAction::CreateLambda([this]()
                  {
                     if (ATATWeatherManager* weatherManager = WeatherEditorUtilities::FindFirstActorOfTypeInPersistentLevel<ATATWeatherManager>())
                     {
                        constexpr bool selected = true;
                        constexpr bool notify = true;
                        constexpr bool selectIfHidden = true;
                        constexpr bool forceRefresh = true;
                        GEditor->SelectActor(weatherManager, selected, notify, selectIfHidden, forceRefresh);
                     }
                  })));
               menu.AddMenuEntry(
                  FText::FromString(TEXT("Open Weather Types Data Table Asset")),
                  FText::GetEmpty(),
                  FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"),
                  FUIAction(FExecuteAction::CreateLambda([this]() { _OpenWeatherTypesDataTable(); })));

               menu.AddSeparator();

               menu.AddMenuEntry(
                  FText::FromString(TEXT("Weather Project Settings")),
                  FText::FromString(TEXT("Opens project settings to the weather settings section.")),
                  FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
                  FUIAction(FExecuteAction::CreateLambda([this]()
                  {
                     FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Game", "TATWeatherSettings");
                  })));
               menu.AddMenuEntry(
                  FText::FromString(TEXT("Weather Editor Per-User Preferences")),
                  FText::FromString(TEXT("Opens the editor per-user settings to the weather settings section.")),
                  FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorPreferences.TabIcon"),
                  FUIAction(FExecuteAction::CreateLambda([this]()
                  {
                     FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "General", "TATEditorSettings");
                  })));

               WeatherEditorUtilities::ShowContextMenuAtCursorLocation(_weatherEditorRootWidget.ToSharedRef(), menu.MakeWidget());
            })
         ]
      ]
      +SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
         SAssignNew(_weatherTypesEditorOuter, SBox)
         [
            _CreateWeatherTypesEditor()
         ]
      ];
}

// static
UWorld* UTATWeatherTool::_GetEditorWorld()
{
   check(GEditor != nullptr);
   return GEditor->GetEditorWorldContext().World();
}

// static
UTATWeatherEditorWorldExtension* UTATWeatherTool::_GetWeatherWorldExtension()
{
   UWorld* world = _GetEditorWorld();
   if (world == nullptr)
   {
      return nullptr;
   }
   UTATWeatherEditorSubsystem* weatherEditorSubsystem = GEditor->GetEditorSubsystem<UTATWeatherEditorSubsystem>();
   if (weatherEditorSubsystem == nullptr)
   {
      return nullptr;
   }
   return weatherEditorSubsystem->GetWeatherEditorWorldExtension(world);
}

static const FName kTableColumn_Visibility = FName(TEXT("Visibility"));
static const FName kTableColumn_State = FName(TEXT("State"));
static const FName kTableColumn_Name = FName(TEXT("Name"));
static const FName kTableColumn_Preset = FName(TEXT("Preset"));

TSharedRef<SWidget> UTATWeatherTool::_CreateWeatherTypesEditor()
{
   return SNew(SSplitter)
      .Orientation(Orient_Vertical)
      .IsEnabled_Lambda([]() -> bool
      {
         return !GEditor->IsPlayingSessionInEditor();
      })
      +SSplitter::Slot()
      .SizeRule(SSplitter::FractionOfParent)
      .Value(0.2f)
      .MinSize(20.0f)
      [
         SNew(SScrollBox)
         .Orientation(Orient_Vertical)
         +SScrollBox::Slot()
         [
            SNew(SListView<TSharedPtr<FWeatherTypeListItem>>)
            //.ItemHeight(28)
            .ListViewStyle(FAppStyle::Get(), "SimpleListView")
            .SelectionMode(ESelectionMode::SingleToggle)
            .OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
            {
               if (TSharedPtr<FWeatherTypeListItem> selectedItem = _weatherPresetListSelectedItem.Pin())
               {
                  return _MakeWeatherPresetContextMenu(selectedItem->PresetType.LoadSynchronous());
               }
               return nullptr;
            })
            .Orientation(Orient_Vertical)
            .HeaderRow(
               SNew(SHeaderRow)
               +SHeaderRow::Column(kTableColumn_Visibility)
               .FixedWidth(25.0f)
               .DefaultLabel(FText::FromString(TEXT("Visibility")))
               +SHeaderRow::Column(kTableColumn_State)
               .FixedWidth(25.0f)
               .DefaultLabel(FText::FromString(TEXT("State")))
               +SHeaderRow::Column(kTableColumn_Name)
               .FillWidth(0.5f)
               .DefaultLabel(FText::FromString(TEXT("Name")))
               +SHeaderRow::Column(kTableColumn_Preset)
               .FillWidth(0.5f)
               .DefaultLabel(FText::FromString(TEXT("Preset Class")))
               )
            .ListItemsSource(&_weatherPresetListItems)
            .OnGenerateRow_UObject(this, &UTATWeatherTool::_GenerateWeatherPresetListRowWidget)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FWeatherTypeListItem> item, ESelectInfo::Type type)
            {
               _SetEditingWeatherType(item);
            })
         ]
      ]
      +SSplitter::Slot()
      .SizeRule(SSplitter::FractionOfParent)
      .Value(0.8f)
      [
         SNew(SBox)
         .Visibility_Lambda([this]()
         {
            return (_showPresetEditor && _weatherPresetListSelectedItem != nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
         })
         [
            SAssignNew(_weatherTypeDetailsPane, SBox)
         ]
      ]
   ;
}

template<typename ItemType>
class SWeatherPresetTableRow : public SMultiColumnTableRow<ItemType>
{
   using FColumnWidgetFn = TFunction<TSharedRef<SWidget>(const FName&, const ItemType&)>;
   ItemType _item;
   FColumnWidgetFn _columnWidgetFn;
public:
   SLATE_BEGIN_ARGS(SWeatherPresetTableRow){}
      SLATE_ARGUMENT(ItemType, Item)
      SLATE_ARGUMENT(FColumnWidgetFn, ColumnWidgetFn)
      SLATE_ARGUMENT(FMargin, Padding)
   SLATE_END_ARGS()

   void Construct(const FArguments& inArgs, const TSharedRef<STableViewBase>& inOwnerTableView)
   {
      _item = inArgs._Item;
      _columnWidgetFn = inArgs._ColumnWidgetFn;
      using FRowArguments = typename SMultiColumnTableRow<ItemType>::FSuperRowType::FArguments;
      SMultiColumnTableRow<ItemType>::Construct(FRowArguments().Padding(inArgs._Padding), inOwnerTableView);
   }

   virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& columnName) override
   {
      return _columnWidgetFn(columnName, _item);
   }
};

TSharedRef<ITableRow> UTATWeatherTool::_GenerateWeatherPresetListRowWidget(TSharedPtr<FWeatherTypeListItem> item, const TSharedRef<STableViewBase>& ownerTable)
{
   TWeakPtr<FWeatherTypeListItem> weakItem = item;
   return SNew(SWeatherPresetTableRow<TSharedPtr<FWeatherTypeListItem>>, ownerTable)
      .Item(item)
      .Padding(FMargin(2.0f))
      .Visibility_Lambda([this, weakItem]() -> EVisibility
      {
         if (!_showDisabledWeatherTypes)
         {
            if (TSharedPtr<FWeatherTypeListItem> item = weakItem.Pin())
            {
               return !item->HideByDefault ? EVisibility::Visible : EVisibility::Collapsed;
            }
         }
         return EVisibility::Visible;
      })
      .ColumnWidgetFn([this](const FName& columnName, const TSharedPtr<FWeatherTypeListItem>& item) -> TSharedRef<SWidget>
      {
         TWeakPtr<FWeatherTypeListItem> weakItem = item;
         if (columnName == kTableColumn_Visibility)
         {
            return SNew(SBox)
               .Padding(FMargin(1.0f))
               [
                  WeatherEditorUtilities::MakeVisibilityButton(
                     FOnClicked::CreateLambda([this, weakItem]() -> FReply
                     {
                        if (TSharedPtr<FWeatherTypeListItem> item = weakItem.Pin())
                        {
                           const bool isPreviewing = item != _weatherPresetListPreviewItem;
                           _SetWeatherListItemPreviewState(item, isPreviewing);
                           return FReply::Handled();
                        }
                        return FReply::Unhandled();
                     }),
                     TAttribute<bool>::CreateLambda([this, weakItem]()
                     {
                        return weakItem.IsValid() && _weatherPresetListPreviewItem == weakItem;
                     }))
               ];
         }
         if (columnName == kTableColumn_State)
         {
            if (item->StateIcon)
            {
               return WeatherEditorUtilities::MakeFontAwesomeIcon(*item->StateIcon, item->StateTooltipText, item->StateColor);
            }
            return SNew(SBox);
         }
         if (columnName == kTableColumn_Name)
         {
            return SNew(STextBlock)
               .Text(item->Label);
         }
         if (columnName == kTableColumn_Preset)
         {
            return SNew(STextBlock)
               .Text_Lambda([weakItem]() -> FText
               {
                  if (TSharedPtr<FWeatherTypeListItem> item = weakItem.Pin())
                  {
                     const FText assetLabel = WeatherEditorUtilities::MakeBlueprintAssetLabel(item->PresetType.ToSoftObjectPath());
                     return WeatherEditorUtilities::IsBlueprintClassAssetDirty(item->PresetType.Get())
                        ? FText::FormatOrdered(FTextFormat(FText::FromString(TEXT("{0} *"))), assetLabel)
                        : assetLabel;
                  }
                  return FText::GetEmpty();
               });
         }
         return SNew(SBox);
      });
}

void UTATWeatherTool::_RefreshWeatherTypes()
{
   _weatherPresetListItems.Reset();

   const TArray<TSoftClassPtr<ATATWeatherPreset>>& customWeatherPresets = UTATEditorSettings::Get().CustomWeatherPresets;
   for (int32 i = 0; i < customWeatherPresets.Num(); i++)
   {
      const TSoftClassPtr<ATATWeatherPreset>& presetClass = customWeatherPresets[i];
      if (!presetClass.IsNull())
      {
         static const FText customItemTooltip = FText::FromString(TEXT("Custom per-user weather preset (Editor Preferences -> [TAT] Editor Per User Settings -> Custom Weather Presets)"));
         TSharedPtr<FWeatherTypeListItem> customPresetItem = MakeShared<FWeatherTypeListItem>();
         customPresetItem->Label = FText::FormatOrdered(FTextFormat(FText::FromString(TEXT("[Editor] Custom Preset {0}"))), i);
         customPresetItem->PresetType = presetClass;
         customPresetItem->StateIcon = WeatherIconGlyph::Star();
         customPresetItem->StateColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f);
         customPresetItem->StateTooltipText = customItemTooltip;
         _weatherPresetListItems.Add(customPresetItem);
      }
   }

   ATATWeatherManager* weatherManager = nullptr;
   if (UWorld* world = _GetEditorWorld())
   {
      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
      {
         if (worldSettings->WeatherManager != nullptr)
         {
            weatherManager = worldSettings->WeatherManager;
         }

         if (!worldSettings->EditorDefaultWeatherPreset.IsNull())
         {
            static const FText editorDefaultTooltip = FText::FromString(TEXT("The default preset when this level is loaded in the editor (WorldSettings -> EditorDefaultWeatherPreset)"));
            TSharedPtr<FWeatherTypeListItem> defaultItem = MakeShared<FWeatherTypeListItem>();
            defaultItem->Label = FText::FromString(TEXT("[Editor] Level Default"));
            defaultItem->PresetType = worldSettings->EditorDefaultWeatherPreset;
            defaultItem->StateIcon = WeatherIconGlyph::Cube();
            defaultItem->StateTooltipText = editorDefaultTooltip;
            _weatherPresetListItems.Add(defaultItem);
         }
      }
   }

   if (UDataTable* dataTable = UTATWeatherSettings::Get().WeatherTypeDataTable.LoadSynchronous())
   {
      dataTable->ForeachRow<FTATWeatherTypeInfo>(TEXT("UTATWeatherTool::_RefreshWeatherTypes"),
         [&](const FName& rowName, const FTATWeatherTypeInfo& info)
         {
            TSharedPtr<FWeatherTypeListItem> entry = MakeShared<FWeatherTypeListItem>();
            entry->WeatherType = info.WeatherType;
            entry->Label = FText::FromString(info.WeatherType.ToString());

            if (info.IsWeatherTypeAllowedInLevel(_GetEditorWorld()))
            {
               entry->StateIcon = FEditorFontGlyphs::Check;
               entry->StateColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);
            }
            else
            {
               static const FText notAllowedInLevelTooltip = FText::FromString(TEXT("Weather type disabled in this level by the weather types data table"));
               entry->HideByDefault = true;
               entry->StateIcon = WeatherIconGlyph::Ban();
               entry->StateColor = FLinearColor::Red;
               entry->StateTooltipText = notAllowedInLevelTooltip;
            }

            // If the current level has an override for this weather type, show the override preset instead
            if (weatherManager != nullptr && weatherManager->PresetOverrides.Contains(info.WeatherType))
            {
               entry->PresetType = weatherManager->PresetOverrides[info.WeatherType];
            }
            else
            {
               entry->PresetType = info.DefaultPreset;
            }

            _weatherPresetListItems.Add(entry);
         });
   }

   // If the current preset is already being previewed, mark it as such in the UI
   UTATWeatherEditorWorldExtension* worldExt = _GetWeatherWorldExtension();
   if (ATATWeatherPreset* currentPresetActor = (worldExt != nullptr) ? worldExt->GetWeatherPresetPreviewActor() : nullptr)
   {
      for (const TSharedPtr<FWeatherTypeListItem>& item : _weatherPresetListItems)
      {
         UClass* cls = item->PresetType.Get();
         if (cls != nullptr && currentPresetActor->GetClass() == cls)
         {
            _weatherPresetListPreviewItem = item;
            break;
         }
      }
   }

   if (_weatherTypesEditorOuter.IsValid())
   {
      _weatherTypesEditorOuter->SetContent(_CreateWeatherTypesEditor());
      if (TSharedPtr<FWeatherTypeListItem> selectedItem = _weatherPresetListSelectedItem.Pin())
      {
         _SetEditingWeatherType(selectedItem);
      }
   }
}

void UTATWeatherTool::_OpenWeatherTypesDataTable()
{
   TSoftObjectPtr<UDataTable> dataTable = UTATWeatherSettings::Get().WeatherTypeDataTable;
   if (!dataTable.IsNull())
   {
      WeatherEditorUtilities::OpenAssetEditorForDataTable(dataTable.LoadSynchronous());
   }
}

// static
void UTATWeatherTool::_RefreshWeatherManagerSceneDepthTexture()
{
   if (UTATWeatherEditorWorldExtension* worldExt = _GetWeatherWorldExtension())
   {
      FScopedTransaction transaction(FText::FromString(TEXT("WeatherTool.RefreshWeatherManagerSceneDepthTexture")));
      FEditorScriptExecutionGuard scriptGuard;
      constexpr bool forceUpdate = true;
      worldExt->UpdateSceneDepthTexture(forceUpdate);
   }
}

TSharedRef<SWidget> UTATWeatherTool::_MakeWeatherPresetContextMenu(TSubclassOf<ATATWeatherPreset> presetClass) const
{
   auto makeComponentCopyAction = [this, presetClass](TSubclassOf<AActor> requiredActorType, TSubclassOf<UActorComponent> componentType) -> FUIAction
   {
      return FUIAction(
         FExecuteAction::CreateLambda([this, presetClass, requiredActorType, componentType]()
         {
            AActor* selectedActor = WeatherEditorUtilities::GetEditorSelectedActor();
            if (selectedActor != nullptr && selectedActor->IsA(requiredActorType))
            {
               FScopedTransaction transaction(FText::FromString(TEXT("WeatherTool.CopyComponentProps")));
               WeatherEditorUtilities::CopyComponentPropsToCDO(componentType, presetClass, selectedActor, [](const FProperty* prop) { return true; });
            }
         }),
         FCanExecuteAction::CreateLambda([this, presetClass, requiredActorType]() -> bool
         {
            if (presetClass == nullptr)
            {
               return false;
            }
            AActor* selectedActor = WeatherEditorUtilities::GetEditorSelectedActor();
            return selectedActor != nullptr && selectedActor->IsA(requiredActorType);
         })
      );
   };

   constexpr bool closeAfterSelection = true;
   FMenuBuilder menuBuilder(closeAfterSelection, nullptr);

   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Save")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "AssetEditor.SaveAsset"),
      FUIAction(
         FExecuteAction::CreateLambda([presetClass]() { WeatherEditorUtilities::SaveBlueprintClassAsset(presetClass); }),
         FCanExecuteAction::CreateLambda([presetClass]() { return WeatherEditorUtilities::IsBlueprintClassAssetDirty(presetClass); })
         ));

   menuBuilder.AddSeparator();

   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Find in Content Browser")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.BrowseContent"),
      FUIAction(
         FExecuteAction::CreateLambda([presetClass]() { WeatherEditorUtilities::FindAssetInContentBrowser(presetClass); }),
         FCanExecuteAction::CreateLambda([presetClass]() { return presetClass != nullptr; })
         ));

   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Open Blueprint Editor")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.BlueprintCore"),
      FUIAction(
         FExecuteAction::CreateLambda([presetClass]() { WeatherEditorUtilities::OpenAssetEditorForClass(presetClass); }),
         FCanExecuteAction::CreateLambda([presetClass]() { return presetClass != nullptr; })
         ));

   menuBuilder.AddSeparator();

   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Copy Directional Light Properties from Selected Actor")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.DirectionalLight"),
      makeComponentCopyAction(ADirectionalLight::StaticClass(), UDirectionalLightComponent::StaticClass()));
   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Copy Exponential Height Fog Properties from Selected Actor")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.ExponentialHeightFog"),
      makeComponentCopyAction(AExponentialHeightFog::StaticClass(), UExponentialHeightFogComponent::StaticClass()));
   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Copy Sky Light Properties from Selected Actor")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.SkyLight"),
      makeComponentCopyAction(ASkyLight::StaticClass(), USkyLightComponent::StaticClass()));
   menuBuilder.AddMenuEntry(
      FText::FromString(TEXT("Copy Sky Atmosphere Properties from Selected Actor")),
      FText::GetEmpty(),
      FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.SkyAtmosphere"),
      makeComponentCopyAction(ASkyAtmosphere::StaticClass(), USkyAtmosphereComponent::StaticClass()));
   return menuBuilder.MakeWidget();
}

void UTATWeatherTool::_SetWeatherListItemPreviewState(const TSharedPtr<FWeatherTypeListItem>& item, bool isPreviewing)
{
   _weatherPresetListPreviewItem = isPreviewing ? item : nullptr;

   UTATWeatherEditorWorldExtension* worldExt = _GetWeatherWorldExtension();
   if (worldExt == nullptr)
   {
      return;
   }

   check(item.IsValid());
   if (isPreviewing)
   {
      worldExt->SetWeatherPreset(item->PresetType.LoadSynchronous());
   }
   else
   {
      // If we're making an item no longer visible, then just clear the current weather preset
      if (ATATWeatherPreset* presetActor = worldExt->GetWeatherPresetPreviewActor())
      {
         if (presetActor->GetClass() == item->PresetType.LoadSynchronous())
         {
            worldExt->SetWeatherPreset(nullptr);
         }
      }
   }
}

void UTATWeatherTool::_SetEditingWeatherType(const TSharedPtr<FWeatherTypeListItem>& item)
{
   _weatherPresetListSelectedItem = item;

   auto clearEditor = [this]()
   {
      if (_weatherTypeDetailsPane.IsValid())
      {
         _weatherTypeDetailsPane->SetContent(SNew(SBox));
      }
   };

   if (_weatherTypeDetailsPane.IsValid())
   {
      if (!_actorEditorWidget.IsValid())
      {
         _actorEditorWidget = MakeShared<WeatherEditorUtilities::FSimpleActorEditor>();
      }

      // Build an editor widget for the preset class
      if (item.IsValid())
      {
         TSubclassOf<ATATWeatherPreset> actorClass = item->PresetType.LoadSynchronous();
         const FIsPropertyVisible propVisibilityDelegate = FIsPropertyVisible::CreateUObject(this, &UTATWeatherTool::_IsWeatherPresetPropertyVisible);

         constexpr bool autoAddComponents = false;
         _actorEditorWidget->SetActorClass(actorClass, propVisibilityDelegate, autoAddComponents);

         // Add all the component templates to the actor that we want to be editable in the weather editor
         ATATWeatherPreset* cdo = actorClass->GetDefaultObject<ATATWeatherPreset>();
         check(cdo != nullptr);

         USceneComponent* rootComponent = cdo->GetRootComponent();
         WeatherEditorUtilities::ForEachComponentInActorCDO(actorClass,
            [&](UActorComponent* comp, const FObjectProperty* nativeProp, USCS_Node* blueprintProp)
            {
               if (comp == nullptr
                  || comp == rootComponent
                  || comp->IsEditorOnly()
                  || comp->HasAnyFlags(RF_Transient)
                  || (comp->ComponentHasTag(kHideInWeatherEditorTag))
                  || (nativeProp != nullptr && nativeProp->HasMetaData(kHideInWeatherEditorTag))
                  )
               {
                  return;
               }

               // For rain and wind niagara components, only make them visible if rain or wind is enabled in the preset.
               // This is mostly to cut down on visual noise in the weather tool.
               TOptional<TFunction<bool()>> isComponentVisible;
               if (comp == cdo->RainNiagaraComponent)
               {
                  isComponentVisible = [weakCdo = MakeWeakObjectPtr(cdo)]
                  {
                     ATATWeatherPreset* cdo = weakCdo.Get();
                     return (cdo != nullptr) ? cdo->EnableRain : true;
                  };
               }
               else if (comp == cdo->WindNiagaraComponent)
               {
                  isComponentVisible = [weakCdo = MakeWeakObjectPtr(cdo)]
                  {
                     ATATWeatherPreset* cdo = weakCdo.Get();
                     return (cdo != nullptr) ? cdo->EnableWind : true;
                  };
               }

               constexpr int32 componentIndentLevel = 1;
               _actorEditorWidget->AddActorComponentEditor(comp, propVisibilityDelegate, isComponentVisible, componentIndentLevel);
            });

         _weatherTypeDetailsPane->SetContent(_actorEditorWidget->Construct());
      }
      else
      {
         clearEditor();
      }
   }

   if (_linkPreviewAndSelectedPresets && item.IsValid())
   {
      _SetWeatherListItemPreviewState(item, true);
   }
}

bool UTATWeatherTool::_IsWeatherPresetPropertyVisible(const FPropertyAndParent& prop)
{
   static const FName categoryKey = FName("Category");
   static const FName hideInWeatherEditorKey = FName("HideInWeatherEditor");
   if (prop.Property.HasMetaData(hideInWeatherEditorKey))
   {
      return false;
   }

   const bool isPublicVariable = !prop.Property.HasAnyPropertyFlags(CPF_DisableEditOnInstance);
   const bool isComponent = prop.Property.HasAnyPropertyFlags(CPF_InstancedReference);
   if (!isPublicVariable || isComponent)
   {
      return false;
   }

   const FString& category = prop.Property.GetMetaData(categoryKey);
   if (category == TEXT("Actor")
      || category == TEXT("Tick")
      || category == TEXT("ComponentTick")
      || category == TEXT("HLOD")
      || category == TEXT("WorldPartition")
      || category == TEXT("Networking")
      || category == TEXT("DataLayers")
      )
   {
      return false;
   }

   return true;
}
