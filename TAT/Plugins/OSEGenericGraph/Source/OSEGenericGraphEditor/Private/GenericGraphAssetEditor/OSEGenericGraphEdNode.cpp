// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "EdNode_GenericGraph"

UOSEGenericGraphEdNode::UOSEGenericGraphEdNode()
{
   bCanRenameNode = true;
}

UOSEGenericGraphEdNode::~UOSEGenericGraphEdNode()
{

}

void UOSEGenericGraphEdNode::AllocateDefaultPins()
{
   CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
   CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

UOSEGenericGraphEdGraph* UOSEGenericGraphEdNode::GetGenericGraphEdGraph()
{
   return Cast<UOSEGenericGraphEdGraph>(GetGraph());
}

FText UOSEGenericGraphEdNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
   if (GenericGraphNode == nullptr)
   {
      return Super::GetNodeTitle(TitleType);
   }

   if (TitleType == ENodeTitleType::EditableTitle && GenericGraphNode->IsTitleEditable())
   {
      return GenericGraphNode->GetEditableTitle();
   }

   if (TitleType == ENodeTitleType::FullTitle)
   {
      const FText Subtitle = GenericGraphNode->GetNodeDisplaySubtitle();
      if (!Subtitle.IsEmpty())
      {
         return FText::Join(INVTEXT("\n"), GenericGraphNode->GetNodeDisplayTitle(), Subtitle);
      }
   }

   if (TitleType == ENodeTitleType::ListView)
   {
      return GenericGraphNode->GetNodeListViewTitle();
   }

   return GenericGraphNode->GetNodeDisplayTitle();
}

void UOSEGenericGraphEdNode::PrepareForCopying()
{
   GenericGraphNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UOSEGenericGraphEdNode::AutowireNewNode(UEdGraphPin* FromPin)
{
   Super::AutowireNewNode(FromPin);

   if (FromPin != nullptr)
   {
      if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
      {
         FromPin->GetOwningNode()->NodeConnectionListChanged();
      }
   }
}

void UOSEGenericGraphEdNode::SetGenericGraphNode(UOSEGenericGraphNode* InNode)
{
   GenericGraphNode = InNode;
}

FLinearColor UOSEGenericGraphEdNode::GetBackgroundColor() const
{
   return GenericGraphNode == nullptr ? FLinearColor::Black : GenericGraphNode->GetBackgroundColor();
}

const FSlateBrush* UOSEGenericGraphEdNode::GetNodeIcon() const
{
   return (GenericGraphNode != nullptr) ? GenericGraphNode->GetNodeIcon() : FAppStyle::GetBrush(TEXT("GraphEditor.StateMachine_16x"));
}

UEdGraphPin* UOSEGenericGraphEdNode::GetInputPin() const
{
   return Pins[0];
}

UEdGraphPin* UOSEGenericGraphEdNode::GetOutputPin() const
{
   return Pins[1];
}

void UOSEGenericGraphEdNode::PostEditUndo()
{
   UEdGraphNode::PostEditUndo();
}

#undef LOCTEXT_NAMESPACE
