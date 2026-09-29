// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"
#include "OSEGenericGraphEdge.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphEdNodeEdge"

UOSEGenericGraphEdNodeEdge::UOSEGenericGraphEdNodeEdge()
{
   bCanRenameNode = true;
}

void UOSEGenericGraphEdNodeEdge::SetEdge(UOSEGenericGraphEdge* Edge)
{
   GenericGraphEdge = Edge;
}

void UOSEGenericGraphEdNodeEdge::AllocateDefaultPins()
{
   UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
   Inputs->bHidden = true;
   UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
   Outputs->bHidden = true;
}

FText UOSEGenericGraphEdNodeEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
   if (GenericGraphEdge == nullptr)
   {
      return FText::GetEmpty();
   }

   if (TitleType == ENodeTitleType::EditableTitle && GenericGraphEdge->IsTitleEditable())
   {
      return GenericGraphEdge->GetEditableTitle();
   }

   const FText DisplayTitle = GenericGraphEdge->GetEdgeDisplayTitle();

   if (TitleType == ENodeTitleType::ListView)
   {
      return !DisplayTitle.IsEmptyOrWhitespace() ? DisplayTitle : FText::FromString(TEXT("Edge"));
   }

   return DisplayTitle;
}

void UOSEGenericGraphEdNodeEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
   if (Pin->LinkedTo.Num() == 0)
   {
      // Commit suicide; transitions must always have an input and output connection
      Modify();

      // Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
      if (UEdGraph* ParentGraph = GetGraph())
      {
         ParentGraph->Modify();
      }

      DestroyNode();
   }
}

void UOSEGenericGraphEdNodeEdge::PrepareForCopying()
{
   GenericGraphEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UOSEGenericGraphEdNodeEdge::CreateConnections(UOSEGenericGraphEdNode* Start, UOSEGenericGraphEdNode* End)
{
   Pins[0]->Modify();
   Pins[0]->LinkedTo.Empty();

   Start->GetOutputPin()->Modify();
   Pins[0]->MakeLinkTo(Start->GetOutputPin());

   // This to next
   Pins[1]->Modify();
   Pins[1]->LinkedTo.Empty();

   End->GetInputPin()->Modify();
   Pins[1]->MakeLinkTo(End->GetInputPin());
}

UOSEGenericGraphEdNode* UOSEGenericGraphEdNodeEdge::GetStartNode()
{
   if (Pins[0]->LinkedTo.Num() > 0)
   {
      return Cast<UOSEGenericGraphEdNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
   }
   else
   {
      return nullptr;
   }
}

UOSEGenericGraphEdNode* UOSEGenericGraphEdNodeEdge::GetEndNode()
{
   if (Pins[1]->LinkedTo.Num() > 0)
   {
      return Cast<UOSEGenericGraphEdNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
   }
   else
   {
      return nullptr;
   }
}

#undef LOCTEXT_NAMESPACE

