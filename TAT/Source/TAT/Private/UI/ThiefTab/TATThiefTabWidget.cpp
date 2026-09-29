// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/ThiefTab/TATThiefTabWidget.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Items/ToolComponent.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootUtils.h"
#include "Player/TATPlayerState.h"
#include "Tools/TATToolSetComponent.h"
#include "UI/ThiefTab/TATThiefToolEntryWidget.h"

// ue
#include "Components/ListView.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefTabWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefTabWidget, Log, All);


UTATThiefTabWidget::UTATThiefTabWidget()
{
   ReceiveOnLocalPlayerStateAdded = true;
}

#if WITH_EDITOR
EDataValidationResult UTATThiefTabWidget::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!IsValid(ToolEntryWidgetClass))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has unassigned _toolEntryWidgetClass!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void UTATThiefTabWidget::NativeDestruct()
{
   _ClearToolEntryWidgetsFromLayout();
   _ClearAllLootEntryWidgetsFromLayout();

   if (UToolSetComponent* toolSetComponent = _toolSetComponent.Get())
   {
      _UnbindFromToolsetComponent(toolSetComponent);
   }
   if (UTATLootInventoryComponent* lootInventoryComponent = _lootInventoryComponent.Get())
   {
      _UnbindFromLootInventoryComponent(lootInventoryComponent);
   }

   Super::NativeDestruct();
}

void UTATThiefTabWidget::_OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps)
{
   Super::_OnLocalPlayerStateAdded_Implementation(ps);
   check(IsValid(ps));
   
   // Toolset lives on pawn, which may not always be present (i.e. spectating) so soft-fail if not found
   // TODO: we should consider a mechanism for avoiding construction of widgets based on incompatible scenarios like this
   const TScriptInterface<IToolSetInterface> toolSetInterface = ps->GetToolSetInterface();
   if (toolSetInterface)
   {
      if (UTATToolSetComponent* toolSetComponent = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         _toolSetComponent = toolSetComponent;
         _BindToToolsetComponent(_toolSetComponent.Get());

         // Construct tool entry widgets
         _ConstructToolEntryWidgets();
      }
   }
   else
   {
      UE_LOG(LogTATThiefTabWidget, Error, TEXT("_OnLocalPlayerStateAdded() | could not find IToolSetInterface on player %s!"), *ps->GetPlayerName());
   }

   UTATLootInventoryComponent* lootInventoryComponent = ps->GetLootInventoryComponent();
   check(IsValid(lootInventoryComponent));
   _lootInventoryComponent = ps->GetLootInventoryComponent();
   _BindToLootInventoryComponent(_lootInventoryComponent.Get());

   // Construct loot entry widgets
   _ConstructLootEntryWidgets(ETATLootType::MajorLoot);
   _ConstructLootEntryWidgets(ETATLootType::MinorLoot);
}

void UTATThiefTabWidget::_ClearToolEntryWidgetsFromLayout()
{
   // Remove all tool widgets from layout
   const TSet<UTATThiefToolEntryWidget*>& toolEntryWidgets = _toolEntryMgr.GetAllToolWidgets();
   for (UTATThiefToolEntryWidget* toolEntryWidget : toolEntryWidgets)
   {
      RemoveToolEntryWidgetFromLayout(toolEntryWidget);
   }

   // Clear collection
   _toolEntryMgr.ClearToolEntryWidgets();
}

void UTATThiefTabWidget::_ClearLootEntryWidgetsFromLayout(ETATLootType lootType)
{
   UListView* listView = GetListViewForLoot(lootType);
   check(IsValid(listView));
   listView->ClearListItems();

   _lootEntryMgr.RemoveListEntriesOfType(lootType);
}

void UTATThiefTabWidget::_ClearAllLootEntryWidgetsFromLayout()
{
   // Clear collection
   _ClearLootEntryWidgetsFromLayout(ETATLootType::MajorLoot);
   _ClearLootEntryWidgetsFromLayout(ETATLootType::MinorLoot);
}

void UTATThiefTabWidget::_ConstructLootEntryWidgets(ETATLootType lootType)
{
   const UTATLootInventoryComponent* lootInventoryComponent = _lootInventoryComponent.Get();
   check(IsValid(lootInventoryComponent));

   UListView* listView = GetListViewForLoot(lootType);
   check(listView);

   switch (lootType)
   {
   case ETATLootType::MajorLoot:
   {
      for (int32 i = 0; i < lootInventoryComponent->GetMajorLootCount(); i++)
      {
         const FTATLootInstance& majorLoot = lootInventoryComponent->GetMajorLootRefByIndex(i);
         _lootEntryMgr.ConstructListEntry(majorLoot, listView);
      }
      break;
   }

   case ETATLootType::MinorLoot:
      for (const FTATLootIdentifier& minorLootIdentifier : lootInventoryComponent->GetMinorLoot())
      {
         check(minorLootIdentifier.IsValid());
         _lootEntryMgr.ConstructListEntry(minorLootIdentifier, listView);
      }
      break;
   default:
      checkNoEntry();
   }
}

void UTATThiefTabWidget::_ConstructToolEntryWidgets()
{
   const UTATToolSetComponent* toolSetComponent = _toolSetComponent.Get();
   check(toolSetComponent);

   // Create entry widget for each tool
   for (int toolIndex = 0; toolIndex < toolSetComponent->GetNumGearTools(); toolIndex++)
   {
      const UToolComponent* toolComponent = toolSetComponent->GetGearToolAtIndex(toolIndex);
      check(IsValid(toolComponent));

      if (!_ShouldShowTool(toolComponent))
      {
         return;
      }

      UTATThiefToolEntryWidget* toolEntryWidget = _toolEntryMgr.ConstructToolEntryWidget(toolComponent, ToolEntryWidgetClass, this);
      check(toolEntryWidget);
      AddToolEntryWidgetToLayout(toolEntryWidget);
   }
}

void UTATThiefTabWidget::_BindToLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent)
{
   check(IsValid(lootInventoryComponent));
   UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Binding to loot inventory..."));
   lootInventoryComponent->OnMinorLootChanged.AddDynamic(this, &UTATThiefTabWidget::_OnMinorLootChanged);
   lootInventoryComponent->OnMajorLootDataChanged.AddDynamic(this, &UTATThiefTabWidget::_OnMajorLootChanged);
}

void UTATThiefTabWidget::_UnbindFromLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent)
{
   check(IsValid(lootInventoryComponent));
   UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Unbinding from loot inventory..."));
   lootInventoryComponent->OnMinorLootChanged.RemoveAll(this);
   lootInventoryComponent->OnMajorLootDataChanged.RemoveAll(this);
}

void UTATThiefTabWidget::_BindToToolsetComponent(UToolSetComponent* toolsetComponent)
{
   check(IsValid(toolsetComponent));
   UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Binding to toolset..."));
   toolsetComponent->OnToolAdded.AddDynamic(this, &UTATThiefTabWidget::_OnToolAdded);
   toolsetComponent->OnToolRemoved.AddDynamic(this, &UTATThiefTabWidget::_OnToolRemoved);
}

void UTATThiefTabWidget::_UnbindFromToolsetComponent(UToolSetComponent* toolsetComponent)
{
   check(IsValid(toolsetComponent));
   UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Unbinding from toolset..."));
   toolsetComponent->OnToolAdded.RemoveAll(this);
   toolsetComponent->OnToolRemoved.RemoveAll(this);
}

void UTATThiefTabWidget::_OnMinorLootChanged()
{
   // TODO: update existing entries rather than clear + repopulate
   _ClearLootEntryWidgetsFromLayout(ETATLootType::MinorLoot);
   _ConstructLootEntryWidgets(ETATLootType::MinorLoot);
}

void UTATThiefTabWidget::_OnMajorLootChanged(const FTATLootInstance& oldMajorLoot, const FTATLootInstance& newMajorLoot)
{
   _ClearLootEntryWidgetsFromLayout(ETATLootType::MajorLoot);
   _ConstructLootEntryWidgets(ETATLootType::MajorLoot);
}

void UTATThiefTabWidget::_OnToolAdded(UToolComponent* toolComponent)
{
   check(IsValid(toolComponent));

   if (!_ShouldShowTool(toolComponent))
   {
      return;
   }

   // Ensure we don't already have a widget for this tool
#if DO_CHECK
   const UTATThiefToolEntryWidget* existingToolEntryWidget = _toolEntryMgr.GetWidgetForTool(toolComponent);
   check(!IsValid(existingToolEntryWidget));
#endif // DO_CHECK

   UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Tool %s added - constructing entry widget..."), *toolComponent->GetName());

   // Construct widget and add to set
   UTATThiefToolEntryWidget* toolEntryWidget = _toolEntryMgr.ConstructToolEntryWidget(toolComponent, ToolEntryWidgetClass, this);
   check(toolEntryWidget);
   AddToolEntryWidgetToLayout(toolEntryWidget);
}

void UTATThiefTabWidget::_OnToolRemoved(UToolComponent* toolComponent)
{
   // Deliberately avoid usage of IsValid(), as the replicated removal of tools often provides callbacks with pending-kill tools
   check(toolComponent);

   if (!_ShouldShowTool(toolComponent))
   {
      return;
   }

   UTATThiefToolEntryWidget* toolEntryWidget = _toolEntryMgr.GetWidgetForTool(toolComponent);
   if (ensureMsgf(IsValid(toolEntryWidget), TEXT("_OnToolRemoved() called for tool %s with no associated UTATThiefToolEntryWidget!"), *toolComponent->GetName()))
   {
      UE_LOG(LogTATThiefTabWidget, Verbose, TEXT("Tool %s removed - removing entry widget..."), *toolComponent->GetName());

      // Remove from collection and layout
      _toolEntryMgr.RemoveToolEntryWidget(toolEntryWidget);
      RemoveToolEntryWidgetFromLayout(toolEntryWidget);
   }
}

bool UTATThiefTabWidget::_ShouldShowTool(const UToolComponent* toolComponent) const
{
   // Deliberately avoid usage of IsValid(), as the replicated removal of tools often provides callbacks with pending-kill tools
   check(toolComponent);

   // Skip tools with category tag that fails query
   const FOSEAbilityInfo& toolInfo = toolComponent->GetToolInfo();
   if (!ToolRequiredCategoryQuery.IsEmpty())
   {
      return ToolRequiredCategoryQuery.Matches(toolInfo.ToolCategory.GetSingleTagContainer());
   }

   return true;
}
