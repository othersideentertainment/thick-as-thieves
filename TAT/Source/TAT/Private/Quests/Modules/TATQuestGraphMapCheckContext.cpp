// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#if WITH_EDITOR

#include "Quests/Modules/TATQuestGraphMapCheckContext.h"

// tat
#include "Lockpicking/TATCombinationScrape.h"
#include "Quests/Modules/TATQuestGraphNode.h"

// ue
#include "Misc/UObjectToken.h"

bool FTATQuestGraphMapCheckContext::TryVisit(const UObject* graph)
{
   if(!_visitedGraphs.Contains(graph))
   {
      _visitedGraphs.Add(graph);
      return true;
   }

   return false;
}

void FTATQuestGraphMapCheckContext::Report(const UTATQuestGraphNode* node, const FText& messageText) const
{
   check(node);

   TSharedRef<FTokenizedMessage> message = FTokenizedMessage::Create(EMessageSeverity::Error);
   message->AddToken(FUObjectToken::Create(node->GetOuter()));
   message->AddText(messageText);
   _reporter(message);
}

FTATQuestGraphMapCheckContext::FTATQuestGraphMapCheckContext(const UWorld* world, TUniqueFunction<void(TSharedRef<FTokenizedMessage>)> reporter)
   : World(world)
   , _lockCombinationNames(CombinationScrape::GetLockCombinationNamesInWorld(world))
   , _reporter(MoveTemp(reporter))
{
}
#endif

