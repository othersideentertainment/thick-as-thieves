// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/MapVariationValidationEditorTool.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnPlan.h"
#include "Variation/TATSpawnStats.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnerHelper.h"
#include "Variation/TATSpawnerRegistrySubsystem.h"
#include "Variation/SceneVariants/TATActiveLayerSet.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"
#include "Quests/Modules/TATQuestGraphTypes.h"
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphNode.h"
#include "Quests/Modules/TATQuestGraphStatsSubsystem.h"
#include "Quests/Spawn/TATQuestSpawnTypes.h"
#include "Quests/Spawn/TATQuestSpawnUtils.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATQuestInfo.h"

// ue5
#include "GameFramework/TATDifficulty.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MapVariationValidationEditorTool)

#define LOCTEXT_NAMESPACE "MapVariationValidationEditorTool"

namespace SpawnToolHelpers {
   static TArray<TObjectPtr<const UTATQuestGraph>> LoadQuestGraphs(TConstArrayView<FGameplayTag> questTags)
   {
      if (questTags.IsEmpty())
      {
         return {};
      }

      FScopedSlowTask slowTask(static_cast<float>(questTags.Num() + 1), LOCTEXT("LoadQuestGraphs", "Loading Quest Graphs"));
      slowTask.MakeDialogDelayed(0.5f);
      
      slowTask.EnterProgressFrame();
      const UDataTable* missionDataTable = UTATProjectSettings::Get().GetDataTableForMissions().LoadSynchronous();
      if (!ensure(missionDataTable))
      {
         return {};
      }

      auto loadQuestGraph = [missionDataTable](FGameplayTag questTag) -> const UTATQuestGraph*
      {
         const FTATMissionInfo* info = missionDataTable->FindRow<FTATMissionInfo>(questTag.GetTagName(), TEXT("LoadQuestGraphs"));
         if (info == nullptr)
         {
            return nullptr;
         }

         return info->QuestGraph.LoadSynchronous();
      };
      
      TArray<TObjectPtr<const UTATQuestGraph>> result;
      for (const FGameplayTag& tag : questTags)
      {
         slowTask.EnterProgressFrame();
         // want nulls if they exist (probably)
         result.Add(loadQuestGraph(tag));
      }

      return result;
   }
   
   UTATSpawnDataAsset* LookupSpawnData(FMessageLog& msgLog)
   {
      FWorldContext& worldContext = GEditor->GetEditorWorldContext();
      if (UWorld* world = worldContext.World())
      {
         ATATWorldSettings& worldSettings = ATATWorldSettings::Get(world);
         if (worldSettings.SpawnData)
         {
            return worldSettings.SpawnData;
         }
         else
         {
            msgLog.Error()
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Could not find spawn config to validate against!")))));
         }
      }
      else
      {
         msgLog.Error()
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Could not find world to validate against!")))));
      }

      return nullptr;
   }

   const UTATQuestGraph* ChooseQuestGraph(int seed, TConstArrayView<TObjectPtr<const UTATQuestGraph>> questGraphs)
   {
      if (questGraphs.IsEmpty())
      {
         return nullptr;
      }

      const int32 index = ActiveQuestHelpers::ChooseRandomMissionIndex(seed, questGraphs.Num());
      return questGraphs[index];
   }

   struct FTATSpawnerSimulator
   {
      bool Init(UTATMapVariationValidationEditorTool* tool, FMessageLog& msgLog)
      {
         SpawnData = LookupSpawnData(msgLog);
         if (SpawnData == nullptr)
         {
            return false;
         }

         World = GEditor->GetEditorWorldContext().World();
         if (World == nullptr)
         {
            return false;
         }

         if(tool->QuestGraphOverride)
         {
            QuestGraphs.Add(tool->QuestGraphOverride);
         }
         else
         {
            QuestGraphs = LoadQuestGraphs(ActiveQuestHelpers::GetMissionsForWorld(World));
         }

         MapVariationValidationHelper::FindAllSpawnersInWorld(World, Spawners);

         for (FGameplayTag locationTag : tool->QuestSpawnLocations)
         {
            QuestSpawnRequests.Add({TSoftClassPtr<AActor>(), FGameplayTag(), locationTag});
         }

         if (QuestSpawnRequests.Num())
         {
            TATQuestSpawnUtils::ScrapeSpawnersInLevel(World, QuestSpawners);
         }

         LimitedTraits.Append(tool->LimitedTraits);
         for (const FTATQuestActorSpawnRequest& spawnRequest : QuestSpawnRequests)
         {
            FTATSceneTraitWithLimit::Add(LimitedTraits, spawnRequest.LocationTag);
         }

         VariantOverrides = MakeArrayView(tool->VariantOverrides);
         ExtraAllowedTraits = tool->ExtraTraits;
         InitialAllowedTraitsCount = ExtraAllowedTraits.Num();

         if (tool->DifficultyType == ETATSimDifficulty::Specific)
         {
            Difficulty = tool->Difficulty;
         }

         return true;
      }

      ETATDifficulty ResolveDifficulty(const FRandomStream& randomStream) const
      {
         if (Difficulty.IsSet())
         {
            return Difficulty.GetValue();
         }

         return static_cast<ETATDifficulty>(randomStream.RandRange(static_cast<int>(ETATDifficulty::Easy),
            static_cast<int>(ETATDifficulty::Hard)));
      }

      void GeneratePlan(const FRandomStream& randomStream)
      {
         Plan.Reset();

         PhaseSummaries.Reset();
         ActiveVariants.Reset();
         ChosenQuestSpawners.Reset();
         QuestPlan.Reset();
         QuestGraphContext.Reset();
         ActiveLayers.Reset();
         FilteredSpawners.Reset();
         ExtraAllowedTraits.SetNum(InitialAllowedTraitsCount, EAllowShrinking::No);
         QuestWorldTags.Reset();
         DidQuestGraphFail = false;

         {
            ETATDifficulty difficulty = ResolveDifficulty(randomStream);
            FGameplayTag difficultyTag = TATDifficulty::GetDifficultyTag(difficulty);
            if (difficultyTag.IsValid())
            {
               ExtraAllowedTraits.Add(difficultyTag);
               QuestWorldTags.AddTagFast(difficultyTag);
            }
         }

         TConstArrayView<UTATSpawnerComponent*> spawnersToUse = Spawners;
         auto removeSpawnersIf = [&spawnersToUse, this](auto&& predicate)
         {
            if(spawnersToUse.GetData() == Spawners.GetData())
            {
               FilteredSpawners = Spawners;
            }

            FilteredSpawners.RemoveAll(predicate);
            spawnersToUse = FilteredSpawners;
         };
         
         {
            FTATSceneVariantSelectionParams params;
            params.Seed = randomStream.GetInitialSeed();
            params.VariantOverrides = VariantOverrides;
            
            if (const UTATQuestGraph* questGraph = ChooseQuestGraph(params.Seed, QuestGraphs))
            {
               const FTATQuestGraphEvalParams graphParams = {
                  .World = World,
                  .WorldTags = QuestWorldTags,
                  .MapSeed = params.Seed,
               };
               QuestGraphContext.AddSceneVariants(VariantOverrides);

               DidQuestGraphFail = !questGraph->EvalQuestGraph(graphParams, QuestGraphContext);
               params.VariantOverrides = QuestGraphContext.GetSceneVariants();
            }
            
            params.OutPhaseSummary = &PhaseSummaries;
            params.ExtraAllowedTraits = ExtraAllowedTraits;
            params.LimitedTraits = LimitedTraits;
            SpawnData->SelectVariants(params, [this](const UTATSceneAsset* scene, const UTATSceneVariantConfig* variant, int32 variantIndex)
               {
                  ActiveVariants.AddVariant(scene, variant);
               });
         }

         {
            ActiveLayers.InitFrom(SpawnData->LayerSceneRequirements, ActiveVariants);
            if(ActiveLayers.IsPopulated())
            {
               removeSpawnersIf([this] (const UTATSpawnerComponent* spawner)
               {
                  const AActor* owner = spawner->GetOwner();
                  return !ActiveLayers.IsCompatible(owner);
               });
            }
         }

         {
            FTATQuestSpawnPlanParams params;
            params.SpawnRequests = QuestSpawnRequests;
            params.QuestSpawners = QuestSpawners;
            params.Variants = &ActiveVariants;
            params.Seed = randomStream.GetInitialSeed();
            TATQuestSpawnUtils::GeneratePlan(QuestPlan, params);
            TATQuestSpawnUtils::EmitSpawnerActors(ChosenQuestSpawners, QuestPlan);
         }

         {
            FTATSpawnGenerationParams params;
            params.Seed = randomStream.GetInitialSeed();
            params.SpawnGroups = SpawnData->SpawnGroups;
            params.Spawners = spawnersToUse;
            params.SceneVariants = &ActiveVariants;
            params.ExternalSpawners = ChosenQuestSpawners;
            SpawnHelpers::GeneratePlan(Plan, params);
         }
      }

      UWorld* World = nullptr;
      TArray<UTATSpawnerComponent*> Spawners;
      TArray<UTATSpawnerComponent*> FilteredSpawners;
      UTATSpawnDataAsset* SpawnData = nullptr;
      TArray<FTATSceneTraitWithLimit> LimitedTraits;
      TArray<FTATQuestActorSpawnRequest> QuestSpawnRequests;
      TArray<TObjectPtr<UTATQuestActorSpawnerComponent>> QuestSpawners;
      TArray<FGameplayTag> ExtraAllowedTraits;
      int InitialAllowedTraitsCount = 0;
      TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> VariantOverrides;
      TArray<FTATScenePhaseSummary> PhaseSummaries;

      FTATSpawnPlan Plan;
      FTATSceneVariantCollection ActiveVariants;
      FTATActiveLayerSet ActiveLayers;
      FTATQuestSpawnPlan QuestPlan;
      TArray<const AActor*> ChosenQuestSpawners;
      TOptional<ETATDifficulty> Difficulty;
      
      TArray<TObjectPtr<const UTATQuestGraph>> QuestGraphs;
      FTATQuestGraphEvalContext QuestGraphContext;
      FGameplayTagContainer QuestWorldTags;
      bool DidQuestGraphFail = false;
   };
}


UTATMapVariationValidationEditorTool::UTATMapVariationValidationEditorTool()
   : Super()
{
   ToolHelpText = FText::FromString(TEXT("Simulate spawning on the currently opened editor map"));
   ToolName = FText::FromString(TEXT("Spawn Simulation Tool"));
}

void UTATMapVariationValidationEditorTool::InitEditorTool()
{
}

void UTATMapVariationValidationEditorTool::ValidateSpawnData()
{
   MapVariationValidationHelper::ClearLog();

   FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);


   if (UTATSpawnDataAsset* spawnData = SpawnToolHelpers::LookupSpawnData(msgLog))
   {
      // run!
      UWorld* world = GEditor->GetEditorWorldContext().World();

      msgLog.Info(FText::FromString(TEXT("Validating spawn data")))
         ->AddToken(FUObjectToken::Create(spawnData));
      MapVariationValidationHelper::ValidateSpawnConfig(*spawnData, world, msgLog);
   }

   // open the log
   msgLog.Open();
}

void UTATMapVariationValidationEditorTool::SimulateSpawning()
{
   MapVariationValidationHelper::InitLog();
   MapVariationValidationHelper::ClearLog();

   FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
   SpawnToolHelpers::FTATSpawnerSimulator simulator;
   if (!simulator.Init(this, msgLog))
   {
      msgLog.Open();
      return;
   }


   FTATSpawnerCountStats countStats;
   FTATSpawnGroupCountStats groupStats;
   FTATVariantFreqencyStats variantFrequencies;
   FTATScenePhaseStats phaseStats;
   FTATQuestCountStats questCountStats;
   FTATTagFrequencyStats missionTagStats;
   FTATQuestGraphNodeStats questNodeStats;
   int questGraphFailures = 0;

   for (int i = 0; i < SimulationCount; ++i)
   {
      FRandomStream randomStream;
      randomStream.GenerateNewSeed();
      if (OverrideSeed >= 0)
      {
         randomStream.Initialize(OverrideSeed);
      }

      simulator.GeneratePlan(randomStream);
      countStats.Append(simulator.Plan);
      groupStats.Append(simulator.Plan);
      variantFrequencies.Append(simulator.ActiveVariants.GetVariants());
      phaseStats.Append(simulator.PhaseSummaries);
      questCountStats.Append(simulator.QuestPlan);
      questNodeStats.Append(simulator.QuestGraphContext.QuestPlan);
      if(ShowSelectedMissionTagStats)
      {
         missionTagStats.Append(simulator.QuestGraphContext.QuestTags);
      }
      if (simulator.DidQuestGraphFail)
      {
         questGraphFailures++;
      }
   }

   if (questGraphFailures > 0)
   {
      const int32 percent = FMath::DivideAndRoundNearest(questGraphFailures * 100, SimulationCount);
      msgLog.Error(FText::Format(INVTEXT("QuestGraph evaluation failed {0}% of the time ({1}/{2})"),
         percent, questGraphFailures, SimulationCount));
   }
   if (ShowVariantPhases)
   {
      phaseStats.WriteToLog(msgLog);
   }
   if (ShowVariantFrequencies)
   {
      simulator.SpawnData->ForEachVariant([&](const UTATSceneVariantConfig* variant) {
         variantFrequencies.WriteVariantToLog(variant, msgLog);
      });
   }
   groupStats.WriteToLog(msgLog);
   if (ShowQuestSpawnerFrequencies)
   {
      // TODO: include non-chosen spawners? (not sure how desirable)
      questCountStats.WriteToLog(msgLog);
   }
   if (ShowSelectedMissionTagStats)
   {
      missionTagStats.WriteToLog(msgLog);
   }
   countStats.WriteToLog(msgLog);
   countStats.WriteToCsv(simulator.World->GetName()); //< TODO: should be conditional?

   if (UTATSpawnerRegistrySubsystem* spawnerRegistry = simulator.World->GetSubsystem<UTATSpawnerRegistrySubsystem>())
   {
      spawnerRegistry->AddSpawnRates(countStats.SpawnCounts, SimulationCount);
      spawnerRegistry->AddExtraSpawnRates(questCountStats.SpawnCounts, SimulationCount); // < also include non-chosen spawners?
   }

   if (UTATQuestGraphStatsSubsystem* graphStatsSubsystem = GEngine->GetEngineSubsystem<UTATQuestGraphStatsSubsystem>())
   {
      graphStatsSubsystem->SetSpawnRates(questNodeStats.Counts, SimulationCount);
   }

   msgLog.Open();
}

void UTATMapVariationValidationEditorTool::GenerateRawSequences()
{
   MapVariationValidationHelper::InitLog();
   MapVariationValidationHelper::ClearLog();

   FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
   SpawnToolHelpers::FTATSpawnerSimulator simulator;
   if (!simulator.Init(this, msgLog))
   {
      msgLog.Open();
      return;
   }

   FTATRawSpawnStats stats;
   for (int i = 0; i < SimulationCount; ++i)
   {
      FRandomStream randomStream;
      randomStream.GenerateNewSeed();

      simulator.GeneratePlan(randomStream);
      stats.Append(simulator.Plan);
   }

   stats.WriteToCsv(simulator.World->GetName());
   msgLog.Info(FText::FromString(TEXT("Wrote raw sequences to csv")));
   msgLog.Open();
}

void UTATMapVariationValidationEditorTool::SimulateSpawningLog()
{
   MapVariationValidationHelper::InitLog();
   MapVariationValidationHelper::ClearLog();

   FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
   SpawnToolHelpers::FTATSpawnerSimulator simulator;
   if (!simulator.Init(this, msgLog))
   {
      msgLog.Open();
      return;
   }

   // TODO: actually aggregate statistics into something useful
   for (int i = 0; i < SimulationCount; ++i)
   {
      FRandomStream randomStream;
      randomStream.GenerateNewSeed();
      msgLog.Info(FText::FromString(FString::Format(TEXT("Simulation iteration {0} (seed {1})"), { i, randomStream.GetCurrentSeed() })));

      simulator.GeneratePlan(randomStream);

      simulator.ActiveVariants.WriteToMessageLog(msgLog);
      simulator.Plan.LogGroupSummaries(msgLog);
   }

   msgLog.Open();
}

void UTATMapVariationValidationEditorTool::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   SaveConfig();
}

#undef LOCTEXT_NAMESPACE
