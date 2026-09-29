// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverConversationGraphSchema.h"

//ose editor
#include "VoiceOver/OSEVoiceOverConversationGraph.h"
#include "VoiceOver/OSEVoiceOverConversationGraphNode.h"

//ose
#include "VoiceOver/OSEVoiceOverConversation.h"
#include "VoiceOver/OSEVoiceOverConversationNode.h"

//ue4
#include "GraphEditorActions.h"
#include "Settings/EditorStyleSettings.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "Toolkits/ToolkitManager.h"
#include "ToolMenus.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverConversationGraphSchema)


#define LOCTEXT_NAMESPACE "OSEVoiceOverConversationGraphSchema"


FName UOSEVoiceOverConversationGraphSchema::PC_ConversationNode(TEXT("Conversation"));

UEdGraphNode* FOSEVoiceOverConversationGraphSchemaAction_NewNode::PerformAction(UEdGraph* parentGraph, UEdGraphPin* fromPin, const FVector2D location, bool selectNewNode/* = true*/)
{
   UOSEVoiceOverConversation* conversation = CastChecked<UOSEVoiceOverConversation>(parentGraph->GetOuter());

   if (!IsValid(conversation))
      return nullptr;

   TSharedPtr<IToolkit> assetEditor = FToolkitManager::Get().FindEditorForAsset(conversation);
   if (!assetEditor.IsValid())
      return nullptr;

   //Set the outers to the conversation and the UEdGraph respectively so things serialilze correctly.
   UOSEVoiceOverConversationNode* conversationNode = NewObject<UOSEVoiceOverConversationNode>(conversation);
   UOSEVoiceOverConversationGraphNode* graphNode = NewObject<UOSEVoiceOverConversationGraphNode>(parentGraph);

   const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
   parentGraph->Modify();
   if (fromPin)
   {
      fromPin->Modify();
   }
   
   conversationNode->Conversation = conversation;

   graphNode->ConversationNode = conversationNode;
   graphNode->Conversation = conversation;

   graphNode->SetFlags(RF_Transactional);

   parentGraph->AddNode(graphNode, true);

   graphNode->CreateNewGuid();
   graphNode->PostPlacedNewNode();

   // For input pins, new node will generally overlap node being dragged off
   // Work out if we want to visually push away from connected node
   int32 xLocation = location.X;
   if (fromPin && fromPin->Direction == EGPD_Input)
   {
      UEdGraphNode* pinNode = fromPin->GetOwningNode();
      const float xDelta = FMath::Abs(pinNode->NodePosX - location.X);
      static constexpr float pushoffDistance = 60;
      if (xDelta < pushoffDistance)
      {
         // Set location to edge of current node minus the max move distance
         // to force node to push off from connect node enough to give selection handle
         xLocation = pinNode->NodePosX - pushoffDistance;
      }
   }

   graphNode->NodePosX = xLocation;
   graphNode->NodePosY = location.Y;
   graphNode->SnapToGrid(GetDefault<UEditorStyleSettings>()->GridSnapSize);

   // setup pins after placing node in correct spot, since pin sorting will happen as soon as link connection change occurs
   graphNode->AllocateDefaultPins();
   graphNode->AutowireNewNode(fromPin);

   return nullptr;
}


// Setup Right Click Menus.
void UOSEVoiceOverConversationGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
   const FText Name = LOCTEXT("NewConversationNode", "New Conversation Node");
   const FText ToolTip = LOCTEXT("NewConversationNodeTooltip", "Create a new conversation node.");

   TSharedPtr<FOSEVoiceOverConversationGraphSchemaAction_NewNode> NewAction(new FOSEVoiceOverConversationGraphSchemaAction_NewNode(FText::GetEmpty(), Name, ToolTip, 0));

   ContextMenuBuilder.AddAction(NewAction);
}

void UOSEVoiceOverConversationGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
   if (Context->Node)
   {
      const UOSEVoiceOverConversationGraphNode* graphNode = Cast<const UOSEVoiceOverConversationGraphNode>(Context->Node);
      {
         FToolMenuSection& Section = Menu->AddSection("OSEVoiceOverConversationGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "VoiceOver Actions"));
         Section.AddMenuEntry(FGenericCommands::Get().Delete);
         Section.AddMenuEntry(FGenericCommands::Get().Cut);
         Section.AddMenuEntry(FGenericCommands::Get().Copy);
         Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
         Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
         
      }
   }
   // No Super call so Node comments option is not shown
}

bool UOSEVoiceOverConversationGraphSchema::ConnectionCausesLoop(const UEdGraphPin* inputPin, const UEdGraphPin* outputPin) const
{
   TArray<UEdGraphNode*> nodeStack;

   nodeStack.Push(inputPin->GetOwningNode());

   while (nodeStack.Num() > 0)
   {
      UEdGraphNode* node = nodeStack.Pop();

      if (node == outputPin->GetOwningNode())
         return true;

      for (UEdGraphPin* pin : node->Pins)
      {
         if (pin->Direction == EGPD_Input)
         {
            //We only follow output pins.
            continue;
         }

         for (UEdGraphPin* link : pin->LinkedTo)
         {
            nodeStack.Push(link->GetOwningNode());
         }
      }
   }

   return false;
}

const FPinConnectionResponse UOSEVoiceOverConversationGraphSchema::CanCreateConnection(const UEdGraphPin* pinA, const UEdGraphPin* pinB) const
{
   // Make sure the pins are not on the same node
   if (pinA->GetOwningNode() == pinB->GetOwningNode())
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both are on the same node"));
   }

   // Compare the directions
   UEdGraphPin* inputPin = nullptr;
   UEdGraphPin* outputPin = nullptr;

   //We have to const_cast the pins here because CategorizePinsByDirection isn't declared with const correctness. It Doesn't modify pinA or pinB so this is safe.
   if (!CategorizePinsByDirection(const_cast<UEdGraphPin*> (pinA), const_cast<UEdGraphPin*>(pinB), inputPin, outputPin))
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionIncompatible", "Directions are not compatible"));
   }

   UOSEVoiceOverConversationGraphNode* outNode = CastChecked<UOSEVoiceOverConversationGraphNode>(outputPin->GetOwningNode());
   UOSEVoiceOverConversationGraphNode* inNode = CastChecked<UOSEVoiceOverConversationGraphNode>(inputPin->GetOwningNode());


   if (ConnectionCausesLoop(inputPin, outputPin))
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionLoop", "Connection would cause loop"));
   }

   return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}


bool UOSEVoiceOverConversationGraphSchema::TryCreateConnection(UEdGraphPin* pinA, UEdGraphPin* pinB) const
{
   check(pinA);
   check(pinB);

   bool bModified = UEdGraphSchema::TryCreateConnection(pinA, pinB);

   if (bModified)
   {
      // Compare the directions
      UEdGraphPin* inputPin = nullptr;
      UEdGraphPin* outputPin = nullptr;

      if (!CategorizePinsByDirection(pinA, pinB, inputPin, outputPin))
      {
         return false;
      }

      UOSEVoiceOverConversationGraphNode* outNode = CastChecked<UOSEVoiceOverConversationGraphNode>(outputPin->GetOwningNode());
      UOSEVoiceOverConversationGraphNode* inNode = CastChecked<UOSEVoiceOverConversationGraphNode>(inputPin->GetOwningNode());

      outNode->ConversationNode->NextNodes.Add(inNode->ConversationNode);
      UOSEVoiceOverConversationGraph* graph = CastChecked<UOSEVoiceOverConversationGraph>(inNode->GetGraph());
      graph->RecomputeRootNodes();
   }

   return bModified;
}

bool UOSEVoiceOverConversationGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
   return true;
}

FLinearColor UOSEVoiceOverConversationGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
   return FLinearColor(1.0f, 0, 0);
}

void UOSEVoiceOverConversationGraphSchema::BreakNodeLinks(UEdGraphNode& targetNode) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

   for (UEdGraphPin* pin : targetNode.Pins)
   {
      for (UEdGraphPin* otherPin : pin->LinkedTo)
      {
         UEdGraphPin* inputPin = nullptr;
         UEdGraphPin* outputPin = nullptr;

         if (!CategorizePinsByDirection(pin, otherPin, inputPin, outputPin))
         {
            return;
         }

         UOSEVoiceOverConversationGraphNode* outNode = CastChecked<UOSEVoiceOverConversationGraphNode>(outputPin->GetOwningNode());
         UOSEVoiceOverConversationGraphNode* inNode = CastChecked<UOSEVoiceOverConversationGraphNode>(inputPin->GetOwningNode());

         outNode->ConversationNode->NextNodes.Remove(inNode->ConversationNode);
      }
   }

   Super::BreakNodeLinks(targetNode);

   UOSEVoiceOverConversationGraph* graph = CastChecked<UOSEVoiceOverConversationGraph>(targetNode.GetGraph());
   graph->RecomputeRootNodes();
}


void UOSEVoiceOverConversationGraphSchema::BreakPinLinks(UEdGraphPin& targetPin, bool bSendsNodeNotifcation) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

   UOSEVoiceOverConversationGraphNode* targetNode = CastChecked<UOSEVoiceOverConversationGraphNode>(targetPin.GetOwningNode());

   for (UEdGraphPin* link : targetPin.LinkedTo)
   {
      UOSEVoiceOverConversationGraphNode* otherNode = CastChecked<UOSEVoiceOverConversationGraphNode>(link->GetOwningNode());
      
      if (targetPin.Direction == EGPD_Input)
      {
         otherNode->ConversationNode->NextNodes.Remove(targetNode->ConversationNode);
      }
      else
      {
         targetNode->ConversationNode->NextNodes.Remove(otherNode->ConversationNode);
      }
   }

   Super::BreakPinLinks(targetPin, bSendsNodeNotifcation);

   UOSEVoiceOverConversationGraph* graph = CastChecked<UOSEVoiceOverConversationGraph>(targetNode->GetGraph());
   graph->RecomputeRootNodes();
}

void UOSEVoiceOverConversationGraphSchema::BreakSinglePinLink(UEdGraphPin* sourcePin, UEdGraphPin* targetPin) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));
   Super::BreakSinglePinLink(sourcePin, targetPin);

   UEdGraphPin* inputPin = nullptr;
   UEdGraphPin* outputPin = nullptr;

   if (!CategorizePinsByDirection(sourcePin, targetPin, inputPin, outputPin))
   {
      return;
   }

   UOSEVoiceOverConversationGraphNode* outNode = CastChecked<UOSEVoiceOverConversationGraphNode>(outputPin->GetOwningNode());
   UOSEVoiceOverConversationGraphNode* inNode = CastChecked<UOSEVoiceOverConversationGraphNode>(inputPin->GetOwningNode());

   outNode->ConversationNode->NextNodes.Remove(inNode->ConversationNode);

   UOSEVoiceOverConversationGraph* graph = CastChecked<UOSEVoiceOverConversationGraph>(inNode->GetGraph());
   graph->RecomputeRootNodes();
}

bool UOSEVoiceOverConversationGraphSchema::SafeDeleteNodeFromGraph(UEdGraph* graph, UEdGraphNode* node) const
{
   if (graph == nullptr || node == nullptr || node->GetGraph() != graph)
   {
      return false;
   }

   UOSEVoiceOverConversationGraphNode* graphNode = Cast<UOSEVoiceOverConversationGraphNode>(node);

   //No need to call the super version here, it always returns false.
   
   for (UEdGraphPin* pin : node->Pins)
   {
      for (UEdGraphPin* otherPin : pin->LinkedTo)
      {
         UEdGraphPin* inputPin = nullptr;
         UEdGraphPin* outputPin = nullptr;

         if (!CategorizePinsByDirection(pin, otherPin, inputPin, outputPin))
         {
            return false;
         }

         UOSEVoiceOverConversationGraphNode* outNode = CastChecked<UOSEVoiceOverConversationGraphNode>(outputPin->GetOwningNode());
         UOSEVoiceOverConversationGraphNode* inNode = CastChecked<UOSEVoiceOverConversationGraphNode>(inputPin->GetOwningNode());

         outNode->ConversationNode->NextNodes.Remove(inNode->ConversationNode);

         if (inNode != graphNode)
         {
            //Is the input node not the deleted node and not have any other connections?
            if (inputPin->LinkedTo.Num() == 1)
            {
               //It's a root node after delete so add it.
               check(!graphNode->Conversation->ConversationRoots.Contains(inNode->ConversationNode));
               graphNode->Conversation->ConversationRoots.Add(inNode->ConversationNode);
            }
         }
      }
   }

   node->DestroyNode();

   return true;
}

#undef LOCTEXT_NAMESPACE

