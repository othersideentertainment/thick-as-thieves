// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Modules/TATQuestGraphRandomHelpers.h"

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

FTATXoshiroRandomStream TATQuestGraphRandomHelpers::CreateStreamForNode(int32 mapSeed, const UTATQuestGraphNode* node)
{
   check(node);
   // Not being clever for now, just combine the high and low bits
   // NOTE: The only difference between the two functions is which part of the NodeId guid it uses (C vs D)
   const int64 seed = FTATXoshiroRandomStream::MakeSeed(mapSeed, static_cast<int32>(node->NodeId.D));
   return FTATXoshiroRandomStream(seed);
}

FTATXoshiroRandomStream TATQuestGraphRandomHelpers::CreateStreamForPath(int32 mapSeed, const UTATQuestGraphNode* node)
{
   check(node);
   // Not being clever for now, just combine the high and low bits
   // NOTE: The only difference between the two functions is which part of the NodeId guid it uses (C vs D)
   const int64 seed = FTATXoshiroRandomStream::MakeSeed(mapSeed, static_cast<int32>(node->NodeId.C));
   return FTATXoshiroRandomStream(seed);
}
