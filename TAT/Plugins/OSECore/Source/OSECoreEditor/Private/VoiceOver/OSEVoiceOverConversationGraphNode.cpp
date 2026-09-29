// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverConversationGraphNode.h"

//ose editor
#include "VoiceOver/OSEVoiceOverConversationGraphSchema.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverConversationGraphNode)

UOSEVoiceOverConversationGraphNode::UOSEVoiceOverConversationGraphNode(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
}

void UOSEVoiceOverConversationGraphNode::AllocateDefaultPins()
{
   CreatePin(EGPD_Output, UOSEVoiceOverConversationGraphSchema::PC_ConversationNode, TEXT("Next"));
   CreatePin(EGPD_Input, UOSEVoiceOverConversationGraphSchema::PC_ConversationNode, NAME_None);
}

FText UOSEVoiceOverConversationGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
   return FText::FromString(TEXT("Conversation Node"));
}

FLinearColor UOSEVoiceOverConversationGraphNode::GetNodeTitleColor() const
{
   return FLinearColor::Red;
}

void UOSEVoiceOverConversationGraphNode::AutowireNewNode(UEdGraphPin* fromPin)
{
   Super::AutowireNewNode(fromPin);

   if (fromPin)
   {
      EEdGraphPinDirection desiredDirection = fromPin->Direction == EGPD_Output ? EGPD_Input : EGPD_Output;
      for (UEdGraphPin* pin : Pins)
      {
         if (pin->Direction == desiredDirection)
         {
            if (GetSchema()->TryCreateConnection(fromPin, pin))
            {
               fromPin->GetOwningNode()->NodeConnectionListChanged();
               break;
            }
         }
      }
   }
}

bool UOSEVoiceOverConversationGraphNode::CanUserDeleteNode() const
{
   return true;
}

