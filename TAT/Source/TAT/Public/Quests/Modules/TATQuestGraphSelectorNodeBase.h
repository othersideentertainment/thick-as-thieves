// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include "TATQuestGraphSelectorNodeBase.generated.h"


namespace TATQuestGraphUtil { struct FNodeBodyBuilder; }


UCLASS()
class TAT_API UTATQuestGraphSelectorNodeBase : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   /// The number of items that will be selected.
   /// If Min is not the same as Max, a random count in that range will be used.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selector", Meta = (UIMin = 0, ClampMin = 0))
   FInt32Interval SelectCount = { 1, 1 };

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UObject
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;

   // From UOSEGenericGraphNode
   virtual TSharedPtr<SWidget> ConstructNodeBodyWidget() override;
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return EOSEGenericGraphNodeStyle::BorderLight; }
   virtual FLinearColor GetBackgroundColor() const override { return TATQuestGraphUtil::kSelectorColor; }
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;

protected:
   virtual void _ConstructNodeBodyContentSlots(TATQuestGraphUtil::FNodeBodyBuilder& builder);

   virtual FText _GetSelectorTypeDisplayName() const { return FText::GetEmpty(); }
   virtual FText _GetSelectorTypeDisplayNamePlural() const { return FText::FormatOrdered(INVTEXT("{0}s"), _GetSelectorTypeDisplayName()); }
   virtual FName _GetSelectorListPropertyName() const { return NAME_None; }
   virtual int32 _GetSelectorNumChoices() const { return 0; }
   virtual FText _GetSelectorKeyColumnText(int32 idx) const { return FText::GetEmpty(); }
   virtual FText _GetSelectorValueColumnText(int32 idx) const { return FText::GetEmpty(); }
   virtual TSharedPtr<SWidget> _ConstructSelectorItemWidget(int32 idx) const;

private:
   void _RefreshSelectorList();

   TSharedPtr<SVerticalBox> _listWidget;
#endif // WITH_EDITOR
};
