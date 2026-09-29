// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverConversationNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverConversationNode)


UOSEVoiceOverConversationNode* UOSEVoiceOverConversationNode::SelectRandomValidNode(const TArray<UOSEVoiceOverConversationNode*>& nodes, const TArray<AActor*>& speakers, FRandomStream& randomStream)
{
   TArray<UOSEVoiceOverConversationNode*, TInlineAllocator<8>> potentialNodes; //Use an inline allocator here, more than 8 nodes seems unlikely.

   for (UOSEVoiceOverConversationNode* node : nodes)
   {
      if (!speakers.IsValidIndex(node->SpeakerIndex))
         continue;

      FOSEConditionContext context(speakers[node->SpeakerIndex]);
      if (node->Conditions.Satisfied(context))
      {
         potentialNodes.Add(node);
      }
   }

   if (potentialNodes.Num() == 0)
      return nullptr;

   return potentialNodes[randomStream.RandRange(0, potentialNodes.Num() - 1)];
}


UOSEVoiceOverConversationNode* UOSEVoiceOverConversationNode::FindNextNode(const TArray<AActor*>& speakers, FRandomStream& randomStream)
{
   if (NextNodes.Num() == 0)
      return nullptr;

   return SelectRandomValidNode(NextNodes, speakers, randomStream);
}

