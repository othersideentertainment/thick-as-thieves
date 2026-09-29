// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATCompoundClueSet.h"

// tat
#include "Developer/TATCycleChecker.h"
#include "Variation/Clues/TATClueInfo.h"
#if WITH_EDITOR
#include "Quests/TATMatchQuestDescription.h"
#include "Quests/TATQuestTags.h"
#endif

// ue
#include "Misc/DataValidation.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCompoundClueSet)

void UTATCompoundClueSet::AddRelevantClueViews(const FTATClueSetContext& context, TArray<FConstStructView>& result) const
{
   for(const FTATClueCluster& cluster : _clueClusters)
   {
      if(!context.ContextTags.HasAll(cluster.RequiredTags))
      {
         continue;
      }

      for(const UTATClueSetBase* clueSet : cluster.ClueSets)
      {
         if(clueSet)
         {
            clueSet->AddRelevantClueViews(context, result);
         }
      }

      for(const FInstancedStruct& clue : cluster.Clues)
      {
         if(clue.IsValid())
         {
            result.Emplace(clue);
         }
      }
   }
}

#if WITH_EDITOR
EDataValidationResult UTATCompoundClueSet::IsDataValid(class FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // check that QuestChoice tags are present in the MQD
   if (MatchQuestDescription)
   {
      FGameplayTagContainer validChoiceTags = MatchQuestDescription->CollectChoiceTags();

      for (int clusterIndex = 0; clusterIndex < _clueClusters.Num(); ++clusterIndex)
      {
         const FTATClueCluster& cluster = _clueClusters[clusterIndex];

         for (const FGameplayTag& requiredTag : cluster.RequiredTags)
         {
            // checking QuestChoice tags specifically, in case there are other tags that come from elsewhere
            if (requiredTag.MatchesTag(TAG_QuestChoice) && !validChoiceTags.HasTag(requiredTag))
            {
               context.AddError(FText::FormatOrdered(INVTEXT("[{0}] Cluster {1} requires QuestChoice {2}, but it is not present in MatchQuestDesc {3}"),
                  FText::FromString(GetName()), clusterIndex, FText::FromString(requiredTag.ToString()), FText::FromString(MatchQuestDescription->GetName())));
            }
         }
      }
   }
   TTATObjectCycleChecker<UTATClueSetBase> cycleChecker([](const UTATClueSetBase* clueSet, TFunctionRef<void (const UTATClueSetBase*)> visitor) { clueSet->VisitClueSetDependencies(visitor);});
   cycleChecker.DetectCycles(this, [&context](const TConstArrayView<const UTATClueSetBase*>& foundCycle)
   {
      TSharedRef<FTokenizedMessage> message = context.AddMessage(EMessageSeverity::Error, INVTEXT("Cycle in clue sets"));
      TATCycleChecker::AddCycleError(*message, foundCycle);
   });

   for (int clusterIndex = 0; clusterIndex < _clueClusters.Num(); ++clusterIndex)
   {
      const FTATClueCluster& cluster = _clueClusters[clusterIndex];

      for (int i = 0; i < cluster.Clues.Num(); ++i)
      {
         const FInstancedStruct& clue = cluster.Clues[i];
         if (!clue.IsValid())
         {
            context.AddError(FText::FormatOrdered(INVTEXT("[{0}] No clue at cluster {1}, clue-index {2}"),
               FText::FromString(GetName()), clusterIndex, i));
            continue;
         }

         clue.Get<FTATClueInfo>().Validate([&context, clusterIndex, i, this](const FText& message)
         {
            context.AddError(FText::FormatOrdered(INVTEXT("[{0}] Error at cluster {1}, clue-index {2}: {3}"),
               FText::FromString(GetName()), clusterIndex, i, message));
         });
      }
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UTATCompoundClueSet::VisitClueSetDependencies(TFunctionRef<void(const UTATClueSetBase*)> visitor) const
{
   for(const FTATClueCluster& cluster : _clueClusters)
   {
      for(const UTATClueSetBase* clueSet : cluster.ClueSets)
      {
         if(IsValid(clueSet))
         {
            visitor(clueSet);
         }
      }
   }
}
#endif
