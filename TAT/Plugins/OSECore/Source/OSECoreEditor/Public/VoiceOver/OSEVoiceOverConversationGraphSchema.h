// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ue4
#include "AssetRegistry/AssetData.h"
#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

#include "OSEVoiceOverConversationGraphSchema.generated.h"


/** Action to add a node to the graph */
USTRUCT()
struct OSECOREEDITOR_API FOSEVoiceOverConversationGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
   GENERATED_BODY();

   // Simple type info
   static FName StaticGetTypeId() { static FName Type("FOSEVoiceOverConversationGraphSchemaAction_NewNode"); return Type; }

   FOSEVoiceOverConversationGraphSchemaAction_NewNode()
      : FEdGraphSchemaAction()
   {}

   FOSEVoiceOverConversationGraphSchemaAction_NewNode(FText nodeCategory, FText menuDesc, FText toolTip, const int32 grouping)
      : FEdGraphSchemaAction(MoveTemp(nodeCategory), MoveTemp(menuDesc), MoveTemp(toolTip), grouping)
   {}

   //~ Begin FEdGraphSchemaAction Interface
   virtual FName GetTypeId() const override { return StaticGetTypeId(); }
   virtual UEdGraphNode* PerformAction(class UEdGraph* parentGraph, UEdGraphPin* fromPin, const FVector2D location, bool selectNewNode = true) override;
   //~ End FEdGraphSchemaAction Interface


};

UCLASS(MinimalAPI)
class UOSEVoiceOverConversationGraphSchema : public UEdGraphSchema
{
   GENERATED_BODY()

public:
   /** Check whether connecting these pins would cause a loop */
   bool ConnectionCausesLoop(const UEdGraphPin* inputPin, const UEdGraphPin* outputPin) const;

   //~ Begin EdGraphSchema Interface
   virtual void GetGraphContextActions(FGraphContextMenuBuilder& contextMenuBuilder) const override;
   virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* context) const override;
   virtual FName GetParentContextMenuName() const override { return NAME_None; }
   virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* pinA, const UEdGraphPin* pinB) const override;
   virtual bool TryCreateConnection(UEdGraphPin* pinA, UEdGraphPin* pinB) const override;
   virtual bool ShouldHidePinDefaultValue(UEdGraphPin* pin) const override;
   virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& pinType) const override;
   virtual void BreakNodeLinks(UEdGraphNode& targetNode) const override;
   virtual void BreakPinLinks(UEdGraphPin& targetPin, bool sendsNodeNotifcation) const override;
   virtual void BreakSinglePinLink(UEdGraphPin* sourcePin, UEdGraphPin* targetPin) const override;
   virtual bool SafeDeleteNodeFromGraph(UEdGraph* graph, UEdGraphNode* node) const override;
   //~ End EdGraphSchema Interface

   static FName PC_ConversationNode;
};
