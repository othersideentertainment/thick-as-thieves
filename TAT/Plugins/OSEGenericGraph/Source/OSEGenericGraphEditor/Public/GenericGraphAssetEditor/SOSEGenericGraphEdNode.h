// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

enum class EOSEGenericGraphNodeStyle : uint8;

class UOSEGenericGraphEdNode;

class OSEGENERICGRAPHEDITOR_API SOSEGenericGraphEdNode : public SGraphNode
{
public:
   SLATE_BEGIN_ARGS(SOSEGenericGraphEdNode) {}
   SLATE_END_ARGS()

   void Construct(const FArguments& InArgs, UOSEGenericGraphEdNode* InNode);

   virtual void UpdateGraphNode() override;
   virtual void CreatePinWidgets() override;
   virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
   virtual bool IsNameReadOnly() const override;

   FText GetNodeEditableTitleText() const;
   void OnNodeEditableTitleTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

   virtual FSlateColor GetBorderBackgroundColor() const;
   virtual FSlateColor GetBackgroundColor() const;

   virtual EVisibility GetDragOverMarkerVisibility() const;

protected:
   TSharedPtr<SNodeTitle> NodeTitle;
   TSharedPtr<SBox> NodeCustomBodyParent;
   TSharedPtr<SWidget> NodeCustomBodyWidget;

   FText GetToolTipText() const;

   UOSEGenericGraphEdNode* GetGenericGraphNodeChecked() const;

   static const FSlateBrush* StyleToNodeBodyBrush(EOSEGenericGraphNodeStyle NodeStyle);
   static const FSlateBrush* StyleToNodeColorSpillBrush(EOSEGenericGraphNodeStyle NodeStyle);
   static const FSlateBrush* StyleToNodeGlossBrush(EOSEGenericGraphNodeStyle NodeStyle);
};

