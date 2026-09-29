// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose editor
#include "VoiceOver/OSEVoiceOverConversationGraph.h"

// ose
#include "VoiceOver/OSEVoiceOverConversation.h"
#include "VoiceOver/OSEVoiceOverConversationNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverConversationGraph)


//Find which graph nodes are roots and set the graph roots to match.

void UOSEVoiceOverConversationGraph::RecomputeRootNodes()
{
   if (Nodes.Num() == 0)
      return;

   UOSEVoiceOverConversationGraphNode* dummyNode = CastChecked<UOSEVoiceOverConversationGraphNode>(Nodes[0]);
   UOSEVoiceOverConversation* conversation = dummyNode->Conversation;
   conversation->ConversationRoots.Empty();

   for (UEdGraphNode* node : Nodes)
   {
      bool hasInput = false;
      for (UEdGraphPin* pin : node->Pins)
      {
         if (pin->Direction == EGPD_Output)
         {
            continue;
         }

         if (pin->LinkedTo.Num() > 0)
         {
            hasInput = true;
            break;
         }
      }

      if (!hasInput)
      {
         UOSEVoiceOverConversationGraphNode* castNode = CastChecked<UOSEVoiceOverConversationGraphNode>(node);
         conversation->ConversationRoots.Add(castNode->ConversationNode);
      }
   }
}



