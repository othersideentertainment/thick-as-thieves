// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Upgrades/TATUpgradeGraph.h"

// tat
#include "SaveGame/TATSaveGame.h"
#include "Upgrades/TATUpgradeCurrency.h"
#include "Progression/TATProgressionSettings.h"

// ose
#include "UI/Slate/SOSERadialBackground.h"

// ue
#include "PaperSprite.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DataValidation.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeGraph)

#if WITH_EDITOR
namespace UpgradeGraphHelpers
{
   static constexpr int32 kGraphIconSize = 48;

   void RefreshCachedSlateBrush(FSlateBrush& brush, TSoftObjectPtr<UPaperSprite> sprite, FVector2D iconSize)
   {
      if (sprite.IsNull())
      {
         brush = FSlateBrush();
         return;
      }

      if (sprite == brush.GetResourceObject())
      {
         return;
      }

      brush = OSERadialHelpers::MakeBrushFromSprite(sprite.LoadSynchronous(), iconSize);
   }

   void RefreshCachedSlateBrushSoft(FSlateBrush& brush, const TSoftObjectPtr<UPaperSprite>& softSprite, FVector2D iconSize)
   {
      TObjectPtr<UPaperSprite> sprite;
      if (!softSprite.IsNull())
      {
         sprite = softSprite.LoadSynchronous();
      }
      RefreshCachedSlateBrush(brush, sprite, iconSize);
   }

   template<int32 Size>
   void BuildCostListString(int32 moneyCost, const TArray<FTATUpgradeCurrencyCost>& costList, const TCHAR* separator, TStringBuilder<Size>& str)
   {
      int32 numLines = 0;

      if (moneyCost > 0)
      {
         str << TEXT("£") << FText::AsNumber(moneyCost).ToString();
         ++numLines;
      }

      for (const FTATUpgradeCurrencyCost& cost : costList)
      {
         if (numLines > 0)
         {
            str << separator;
         }
         FString tag = cost.CurrencyTag.ToString();
         tag.RemoveFromStart(TEXT("UpgradeCurrency."));
         str << FText::AsNumber(cost.Amount).ToString() << TEXT("x ") << tag;
         ++numLines;
      }
   }
} // namespace UpgradeGraphHelpers
#endif // WITH_EDITOR

UTATUpgradeGraphEdge::UTATUpgradeGraphEdge()
{
}

UTATUpgradeGraphNode::UTATUpgradeGraphNode()
{
#if WITH_EDITORONLY_DATA
   CompatibleGraphType = UTATUpgradeGraph::StaticClass();
#endif
}

#if WITH_EDITOR
void UTATUpgradeGraphNode::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   const FName propName = propertyChangedEvent.GetPropertyName();
   if (propName == "UpgradeType" || propName == "UseIconOverride" || propName == "IconOverride")
   {
      UpgradeGraphHelpers::RefreshCachedSlateBrush(_iconBrush, GetUpgradeIcon(), FVector2D(UpgradeGraphHelpers::kGraphIconSize));
   }
}
#endif

FText UTATUpgradeGraphNode::GetUpgradeName() const
{
   if (UseNameOverride)
   {
      return NameOverride;
   }
   if (UpgradeType != nullptr)
   {
      return UpgradeType->Name;
   }
   return FText::GetEmpty();
}

FText UTATUpgradeGraphNode::GetUpgradeDescription() const
{
   if (UseDescriptionOverride)
   {
      return DescriptionOverride;
   }
   if (UpgradeType != nullptr)
   {
      return UpgradeType->Description;
   }
   return FText::GetEmpty();
}

TSoftObjectPtr<UPaperSprite> UTATUpgradeGraphNode::GetUpgradeIcon() const
{
   if (UseIconOverride)
   {
      return IconOverride;
   }
   if (UpgradeType != nullptr)
   {
      return UpgradeType->Icon;
   }
   return TSoftObjectPtr<UPaperSprite>{};
}

FGameplayTag UTATUpgradeGraphNode::GetUpgradeTag() const
{
   return (UpgradeType != nullptr) ? UpgradeType->UpgradeTag : FGameplayTag::EmptyTag;
}

int32 UTATUpgradeGraphNode::GetUpgradeLevel() const
{
   return UpgradeLevel;
}

ETATUpgradeProgressionType UTATUpgradeGraphNode::GetUpgradeProgressionType() const
{
   return (UpgradeType != nullptr) ? UpgradeType->ProgressionType : ETATUpgradeProgressionType::None;
}

#if WITH_EDITOR

void UTATUpgradeGraphNode::GetCustomGraphContextMenuActions(TArray<FOSEGenericGraphNodeCustomAction>& outActions) const
{
   Super::GetCustomGraphContextMenuActions(outActions);

   auto makeFriendlyAssetName = [](FName assetName) -> FString
   {
      FString name = assetName.ToString();
      name.RemoveFromStart(TEXT("DA_"));
      name.RemoveFromStart(TEXT("UpgradeType_"));
      constexpr bool isBool = false;
      return FName::NameToDisplayString(name, isBool);
   };

   // Add a "create node" context menu entry for each upgrade type data asset that auto-populates the UpgradeType field.
   const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   TArray<FAssetData> allUpgradeTypeDataAssets;
   assetRegistryModule.Get().GetAssetsByClass(UTATUpgradeType::StaticClass()->GetClassPathName(), allUpgradeTypeDataAssets);
   outActions.Reserve(outActions.Num() + allUpgradeTypeDataAssets.Num());
   for (const FAssetData& dataAsset : allUpgradeTypeDataAssets)
   {
      if (UTATUpgradeType* upgradeType = Cast<UTATUpgradeType>(dataAsset.GetAsset()))
      {
         FOSEGenericGraphNodeCustomAction& action = outActions.Emplace_GetRef();
         action.Category = FText::FromString(TEXT("Upgrade Types"));

         if (!upgradeType->Name.IsEmptyOrWhitespace())
         {
            action.Label = FText::FormatOrdered(FText::FromString(TEXT("{0} ({1})")),
               FText::FromString(makeFriendlyAssetName(dataAsset.AssetName)), FText::FromString(upgradeType->UpgradeTag.ToString()));
         }
         else
         {
            action.Label = FText::FromString(makeFriendlyAssetName(dataAsset.AssetName));
         }

         action.TooltipText = FText::FormatOrdered(FText::FromString(TEXT("Add a new '{0}' node\n\nDescription: {1}")),
            upgradeType->Name, upgradeType->Description);

         // Set up a function that will create this node when requested
         TSoftObjectPtr<UTATUpgradeType> upgradeSoft = upgradeType;
         action.CreateNode = [upgradeSoft](UObject* outer) -> UOSEGenericGraphNode*
         {
            UTATUpgradeGraphNode* newNode = NewObject<UTATUpgradeGraphNode>(outer);
            check(newNode != nullptr);
            newNode->UpgradeType = upgradeSoft.LoadSynchronous();
            return newNode;
         };
      }
   }
}

EOSEGenericGraphNodeStyle UTATUpgradeGraphNode::GetNodeStyle() const
{
   if (UpgradeType != nullptr)
   {
      return UTATProgressionSettings::Get().UpgradeEditorProgressionNodeStyles[static_cast<int32>(UpgradeType->ProgressionType)].Style;
   }
   return EOSEGenericGraphNodeStyle::Flat;
}

FLinearColor UTATUpgradeGraphNode::GetBackgroundColor() const
{
   if (UpgradeType != nullptr)
   {
      return UTATProgressionSettings::Get().UpgradeEditorProgressionNodeStyles[static_cast<int32>(UpgradeType->ProgressionType)].Color;
   }
   return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

const FSlateBrush* UTATUpgradeGraphNode::GetNodeIcon() const
{
   UpgradeGraphHelpers::RefreshCachedSlateBrush(const_cast<UTATUpgradeGraphNode*>(this)->_iconBrush, GetUpgradeIcon(),
      FVector2D(UpgradeGraphHelpers::kGraphIconSize));
   return (_iconBrush.GetResourceObject() != nullptr) ? &_iconBrush : Super::GetNodeIcon();
}

FText UTATUpgradeGraphNode::GetNodeDisplayTitle() const
{
   const FText name = GetUpgradeName();
   return !name.IsEmptyOrWhitespace() ? name : INVTEXT("Untitled");
}

FText UTATUpgradeGraphNode::GetNodeDisplaySubtitle() const
{
   return FText::FromString(GetUpgradeTag().ToString());
}

FText UTATUpgradeGraphNode::GetNodeListViewTitle() const
{
   return FText::FormatOrdered(INVTEXT("{0} - {1} (Level {2})"), GetUpgradeName(), FText::FromString(GetUpgradeTag().ToString()), UpgradeLevel);
}

FText UTATUpgradeGraphNode::GetEditableTitle() const
{
   return GetUpgradeName();
}

void UTATUpgradeGraphNode::SetEditableTitle(const FText& newName)
{
   UseNameOverride = !newName.IsEmpty() && (UpgradeType == nullptr || !newName.EqualToCaseIgnored(UpgradeType->Name));
   NameOverride = UseNameOverride ? newName : FText::GetEmpty();
}

void UTATUpgradeGraphNode::OnNodeDoubleClicked()
{
   Super::OnNodeDoubleClicked();
   check(GEditor != nullptr);

   // Open the data class asset editor for the current upgrade type when the node is double-clicked
   if (UpgradeType != nullptr)
   {
      if (UAssetEditorSubsystem* assetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
      {
         assetEditorSubsystem->OpenEditorForAsset(UpgradeType);
      }
   }
}

TSharedPtr<SWidget> UTATUpgradeGraphNode::ConstructNodeBodyWidget()
{
   const FMargin slotPadding = FMargin(2.0f);
   const FMargin textBoxPadding = FMargin(6.0f);
   const FSlateBrush* textBoxBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground");
   const FLinearColor textBoxBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
   const FSlateColor textBoxTextColor = FSlateColor(FColor::White);

   TSharedPtr<SVerticalBox> body = SNew(SVerticalBox);

   auto addContentSlot = [&](EHorizontalAlignment horizAlign, float backgroundAlpha, TAttribute<EVisibility> visibilityAttr, const TSharedRef<SWidget>& content)
   {
      FLinearColor bgColor = textBoxBackgroundColor;
      bgColor.A = backgroundAlpha;
      body->AddSlot()
         .AutoHeight()
         .Padding(slotPadding)
         .HAlign(horizAlign)
         [
            SNew(SBorder)
            .Padding(textBoxPadding)
            .BorderImage(textBoxBrush)
            .BorderBackgroundColor(FSlateColor(bgColor))
            .ForegroundColor(textBoxTextColor)
            .Visibility(visibilityAttr)
            [
               content
            ]
         ];
   };

#define TAT_MAKE_VISIBLE_IF(VISIBLE_EXPR) \
      TAttribute<EVisibility>::CreateLambda([weakThis = MakeWeakObjectPtr(this)]() -> EVisibility \
      { \
         UTATUpgradeGraphNode* self = weakThis.Get(); \
         return (self != nullptr && (VISIBLE_EXPR)) ? EVisibility::Visible : EVisibility::Collapsed; \
      })

   // Show upgrade level
   addContentSlot(HAlign_Center, 0.75f, TAttribute<EVisibility>(),
         SNew(STextBlock)
         .Font(FAppStyle::GetFontStyle("BoldFont"))
         .Justification(ETextJustify::Center)
         .Text_Lambda([weakThis = MakeWeakObjectPtr(this)]() -> FText
         {
            int32 level = 0;
            if (UTATUpgradeGraphNode* self = weakThis.Get())
            {
               level = self->GetUpgradeLevel();
            }
            return FText::Format(INVTEXT("Level {0}"), level);
         })
      );

   // Show disabled message
   addContentSlot(HAlign_Center, 0.75f, TAT_MAKE_VISIBLE_IF(self->DisabledInUI),
         SNew(STextBlock)
         .Font(FAppStyle::GetFontStyle("BoldFont"))
         .Justification(ETextJustify::Center)
         .ColorAndOpacity(FSlateColor(FLinearColor::Red))
         .Text(INVTEXT("DISABLED IN UI"))
      );

   // Show upgrade costs
   addContentSlot(HAlign_Fill, 0.75f, TAT_MAKE_VISIBLE_IF(self->MoneyCost > 0 || self->Cost.Num() > 0),
         SNew(SVerticalBox)
         +SVerticalBox::Slot()
         .AutoHeight()
         [
            SNew(STextBlock)
            .Text(INVTEXT("Unlock Costs"))
            .TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
         ]
         +SVerticalBox::Slot()
         .AutoHeight()
         [
            SNew(STextBlock)
            .Text_Lambda([weakThis = MakeWeakObjectPtr(this)]() -> FText
            {
               if (UTATUpgradeGraphNode* self = weakThis.Get())
               {
                  TStringBuilder<127> costLabel;
                  UpgradeGraphHelpers::BuildCostListString(self->MoneyCost, self->Cost, TEXT("\n"), costLabel);
                  return FText::FromString(*costLabel);
               }
               return FText::GetEmpty();
            })
         ]
      );

   // Show upgrade description
   addContentSlot(HAlign_Fill, 0.25f, TAT_MAKE_VISIBLE_IF(!self->GetUpgradeDescription().IsEmptyOrWhitespace()),
         SNew(STextBlock)
         .WrapTextAt(300.0f)
         .Text_Lambda([weakThis = MakeWeakObjectPtr(this)]() -> FText
         {
            if (UTATUpgradeGraphNode* self = weakThis.Get())
            {
               return self->GetUpgradeDescription();
            }
            return FText::GetEmpty();
         })
      );

#undef TAT_MAKE_VISIBLE_IF

   return body;
}

#endif // WITH_EDITOR

UTATUpgradeGraph::UTATUpgradeGraph()
{
   NodeTypes = { UTATUpgradeGraphNode::StaticClass() };
   EdgeType = UTATUpgradeGraphEdge::StaticClass();
   bEdgeTransitionEnabled = true;
#if WITH_EDITORONLY_DATA
   bCanRenameNode = true;
   bCanBeCyclical = false;
#endif
}

UTATUpgradeGraphNode* UTATUpgradeGraph::FindUpgradeNode(FGameplayTag upgradeTag, int32 level) const
{
   for (UOSEGenericGraphNode* node : AllNodes)
   {
      if (UTATUpgradeGraphNode* upgradeNode = Cast<UTATUpgradeGraphNode>(node))
      {
         if (upgradeNode->GetUpgradeTag() == upgradeTag && upgradeNode->GetUpgradeLevel() == level)
         {
            return upgradeNode;
         }
      }
   }
   return nullptr;
}

#if WITH_EDITOR

struct FTATUpgradeLevelPair
{
   FGameplayTag Tag;
   int32 Level = 0;
   bool operator==(const FTATUpgradeLevelPair& rhs) const { return Tag == rhs.Tag && Level == rhs.Level; }
   friend uint32 GetTypeHash(const FTATUpgradeLevelPair& pair) { return HashCombine(GetTypeHash(pair.Tag), GetTypeHash(pair.Level)); }
};

EDataValidationResult UTATUpgradeGraph::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);

   TSet<FTATUpgradeLevelPair, DefaultKeyFuncs<FTATUpgradeLevelPair>, TInlineSetAllocator<64>> upgradeLevelPairCombos;
   for (UOSEGenericGraphNode* genericNode : AllNodes)
   {
      if (UTATUpgradeGraphNode* node = Cast<UTATUpgradeGraphNode>(genericNode))
      {
         if (!node->UpgradeType)
         {
            context.AddError(FText::Format(INVTEXT("Upgrade graph node '{0}' does not have an upgrade type"),
               FText::FromString(node->GetName())));
            continue;
         }

         if (node->UseNameOverride && node->NameOverride.IsEmptyOrWhitespace())
         {
            context.AddWarning(FText::Format(INVTEXT("Upgrade graph node '{0}' (type = '{1}', level = {2}) has an empty (or all-whitespace) name property"),
               FText::FromString(node->GetName()), FText::FromString(node->UpgradeType->GetName()), node->GetUpgradeLevel()));
         }

         const FGameplayTag upgradeTag = node->GetUpgradeTag();
         if (upgradeTag.IsValid())
         {
            const FTATUpgradeLevelPair upgradeLevelPair{ upgradeTag, node->GetUpgradeLevel() };
            if (upgradeLevelPairCombos.Contains(upgradeLevelPair))
            {
               context.AddError(FText::Format(INVTEXT("Upgrade graph node '{0}' has a duplicate tag/level combination (tag = {1}, level = {2})"),
                  FText::FromString(node->GetNodeDebugName()), FText::FromString(upgradeLevelPair.Tag.ToString()), upgradeLevelPair.Level));
            }
            else
            {
               upgradeLevelPairCombos.Add(upgradeLevelPair);
            }
         }
         else
         {
            context.AddError(FText::Format(INVTEXT("Upgrade graph node '{0}' references upgrade type '{1}' that has an invalid upgrade tag"),
               FText::FromString(node->GetNodeDebugName()), FText::FromString(node->UpgradeType->GetName())));
         }
      }
   }

   return CombineDataValidationResults(baseResult,
      (context.GetNumWarnings() + context.GetNumErrors() == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}

#endif // WITH_EDITOR
