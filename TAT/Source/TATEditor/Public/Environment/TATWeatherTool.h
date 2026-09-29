// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATWeatherEditorUtilities.h"

// ose
#include "Editor/OSEBaseEditorTool.h"

// ue
#include "GameplayTagContainer.h"

#include "TATWeatherTool.generated.h"

class ATATWeatherPreset;
class UTATWeatherEditorWorldExtension;

UCLASS()
class TATEDITOR_API UTATWeatherTool : public UOSEBaseEditorTool
{
   GENERATED_BODY()

public:
   UTATWeatherTool();

public:
   // from UObject
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;

   // from UOSEBaseEditorTool
   virtual void InitEditorTool() override;
   virtual void OnEditorToolClosed() override;
   virtual TSharedRef<SWidget> CreateEditorToolWidget() override;

private:
   struct FWeatherTypeListItem;

   static UWorld* _GetEditorWorld();
   static UTATWeatherEditorWorldExtension* _GetWeatherWorldExtension();

   TSharedRef<SWidget> _CreateWeatherTypesEditor();
   TSharedRef<ITableRow> _GenerateWeatherPresetListRowWidget(TSharedPtr<FWeatherTypeListItem> item, const TSharedRef<STableViewBase>& ownerTable);
   void _RefreshWeatherTypes();
   void _OpenWeatherTypesDataTable();
   static void _RefreshWeatherManagerSceneDepthTexture();
   TSharedRef<SWidget> _MakeWeatherPresetContextMenu(TSubclassOf<ATATWeatherPreset> presetClass) const;
   void _SetWeatherListItemPreviewState(const TSharedPtr<FWeatherTypeListItem>& item, bool isPreviewing);
   void _SetEditingWeatherType(const TSharedPtr<FWeatherTypeListItem>& item);
   bool _IsWeatherPresetPropertyVisible(const FPropertyAndParent& prop);

   FDelegateHandle _worldAddedDelegateHandle;

   TSharedPtr<SWidget> _weatherEditorRootWidget;
   TSharedPtr<SBox> _weatherTypesEditorOuter;

   TSharedPtr<WeatherEditorUtilities::FSimpleActorEditor> _actorEditorWidget;

   TSharedPtr<SBox> _weatherTypeDetailsPane;

   struct FWeatherTypeListItem
   {
      FText Label;
      FGameplayTag WeatherType;
      bool HideByDefault = false; // Hide this item unless _showDisabledWeatherTypes is true
      TOptional<FText> StateIcon; // FontAwesome icon to show in the state column
      TOptional<FLinearColor> StateColor;
      TOptional<FText> StateTooltipText;
      TSoftClassPtr<ATATWeatherPreset> PresetType;
   };
   TArray<TSharedPtr<FWeatherTypeListItem>> _weatherPresetListItems;

   // If true, selecting a preset also sets it to visible
   bool _linkPreviewAndSelectedPresets = true;

   // If true, shows the preset actor CDO editor
   bool _showPresetEditor = false;

   // If true, show weather types that are disabled for the current level
   bool _showDisabledWeatherTypes = false;

   // what list item is currently being edited
   TWeakPtr<FWeatherTypeListItem> _weatherPresetListSelectedItem;

   // what list item is currently being previewed in the editor
   TWeakPtr<FWeatherTypeListItem> _weatherPresetListPreviewItem;
};
