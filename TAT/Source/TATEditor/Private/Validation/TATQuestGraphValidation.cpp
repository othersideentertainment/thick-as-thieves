// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/TATQuestGraphValidation.h"

// tat
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphMapCheckContext.h"

// ue
#include "Logging/MessageLog.h"

void TATQuestGraphValidation::ValidatedRelatedQuestGraphs(UWorld* world, FMessageLog& msgLog)
{
   TArray<TSoftObjectPtr<UTATQuestGraph>> questGraphs = UTATQuestGraph::FindForMap(world);
   if(questGraphs.IsEmpty())
   {
      return;
   }

   FTATQuestGraphMapCheckContext context = FTATQuestGraphMapCheckContext::Create(world,
      [&msgLog](TSharedRef<FTokenizedMessage> message) { msgLog.AddMessage(MoveTemp(message)); });
   
   for (const TSoftObjectPtr<UTATQuestGraph>& softGraph : questGraphs)
   {
      const UTATQuestGraph* graph = softGraph.LoadSynchronous();
      if (graph == nullptr)
      {
         continue;
      }

      graph->ValidateAgainstWorld(context);
   }
}
