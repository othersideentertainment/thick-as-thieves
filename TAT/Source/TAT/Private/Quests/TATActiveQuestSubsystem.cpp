// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATActiveQuestSubsystem.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATAuthMapSeedSubsystem.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATQuestTags.h"
#include "Settings/TATMatchSettings.h"
#include "TATGameInstance.h"
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherTypeInfo.h"
#include "Quests/TATQuestObjective.h"
#include "Quests/Modules/TATQuestGraphObjectiveNode.h"
#include "Quests/Modules/TATQuestGraphUtil.h"
#include "Quests/Modules/TATQuestGraphSubgraphNode.h"

// ose
#include "OSECoreCheats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActiveQuestSubsystem)
DEFINE_LOG_CATEGORY_STATIC(LogTATActiveQuestSubsystem, Log, All);

namespace ActiveQuestCVars
{
   static int MissionFallbackMode = 0;
   FAutoConsoleVariableRef CVarFallbackMissionMode(
      TEXT("TAT.Mission.FallbackMode"),
      MissionFallbackMode,
      TEXT("How to choose a mission if none is selected:\n")
      TEXT("0 = Random\n")
      TEXT("1 = First"),
      ECVF_Default);

   static constexpr int MissionFallbackMode_Random = 0;
   static constexpr int MissionFallbackMode_First = 1;
}

namespace ActiveQuestHelpers
{
   TConstArrayView<FGameplayTag> GetMissionsForWorld(const UWorld* world)
   {
      // First try missions from project settings
      if (const FTATMapSettings* mapSettings = UTATProjectSettings::Get().FindMapSettings(world))
      {
         return mapSettings->Missions;
      }

      // Then fall back to developer ones in world settings
      return ATATWorldSettings::Get(world).DevelopmentMissions;
   }

   int32 ChooseRandomMissionIndex(int32 seed, int32 count)
   {
      check(count > 0);
      constexpr int32 missionHash = 0xF0847FE8; // Just a number to mix in with the seed (xxhash of "MatchQuest")
      FRandomStream randomStream(SeedHelpers::CombineSeed(missionHash, seed));
      return randomStream.RandHelper(count);
   }

   static FGameplayTag ChooseRandomMission(int32 seed, TConstArrayView<FGameplayTag> questOptions)
   {
      const int32 index = ChooseRandomMissionIndex(seed, questOptions.Num());
      return questOptions[index];
   }

   static FGameplayTag ResolveMission(const UWorld* world, int32 seed)
   {
      check(world);

      FGameplayTag missionTag;

      // Retrieve mission selected by host
      // NOTE: storing selected-mission in TATMatchSettings is likely to change, as it is not a reliable source of truth 
      // w.r.t editor-prefs quest override applied here
      if (const UTATMatchSettings* matchSettings = UTATMatchSettingsBase::GetTATMatchSettings<UTATMatchSettings>(world))
      {
         // NOTE: we are not clearing the mission! revisit this in the future
         missionTag = matchSettings->Mission;
      }

      // If not set, choose a random mission for the current map
      // doing before the editor override in case it is interesting to override to nothing.
      if (!missionTag.IsValid())
      {
         TConstArrayView<FGameplayTag> questOptions = GetMissionsForWorld(world);
         if (questOptions.Num() > 0)
         {
            switch(ActiveQuestCVars::MissionFallbackMode)
            {
            case ActiveQuestCVars::MissionFallbackMode_Random:
               missionTag = ChooseRandomMission(seed, questOptions);
               break;
            case ActiveQuestCVars::MissionFallbackMode_First:
               missionTag = questOptions[0];
               break;
            }

         }
      }

#if WITH_EDITOR
      // in editor allow override w/ developer settings
      if (GEngine->IsEditor() && !GIsAutomationTesting)
      {
         const UTATEditorSettings& tatEditorSettings = UTATEditorSettings::Get();
         if (tatEditorSettings.ShouldOverrideMission)
         {
            missionTag = tatEditorSettings.MissionOverride;
         }
      }
#endif

      return missionTag;
   }

   struct FResolvedContract
   {
      FGameplayTag ContractTag;
      FPlayerMask OfficialPlayers;
   };
   static FResolvedContract ResolveContracts(const UWorld* world)
   {
      check(world);

#if WITH_EDITOR
      // in editor allow override w/ developer settings
      if (GEngine->IsEditor() && !GIsAutomationTesting)
      {
         const UTATEditorSettings& tatEditorSettings = UTATEditorSettings::Get();
         if (tatEditorSettings.OverrideContracts)
         {
            FPlayerMask officialPlayers;
            for (int i = 0; i < tatEditorSettings.PlayersOfficiallyOnContract.Num(); ++i)
            {
               if (tatEditorSettings.PlayersOfficiallyOnContract[i])
               {
                  // NOTE: This doesn't necessarily line up with PIE numbers like the previous overrides, since order of connections could vary (unless we forced it to)
                  //       But
                  const int32 playerId = i + 1;
                  officialPlayers.Add(playerId);
               }
            }

            // For now, override replaces all contract if enabled
            return FResolvedContract { tatEditorSettings.ContractTag, officialPlayers};
         }
      }
#endif
      

      if (UTATGameInstance* gameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         const FTATPartyContracts& partyContracts = gameInstance->GetContractSelections();
         FGameplayTag contractTag = partyContracts.ContractTag;
         FPlayerMask officialPlayers;
         for (const FUniqueNetIdRepl& playerNetId : partyContracts.PlayersOnContract)
         {
            const int32 playerId = gameInstance->TryPredictPlayerIdForPartyMember(playerNetId);
            if (playerId != INDEX_NONE)
            {
               officialPlayers.Add(playerId);
            }
         }
         
         // clear quest selections so that it doesn't stick around
         gameInstance->ClearContractSelections();
         
         return FResolvedContract { contractTag, officialPlayers };
      }

      return {};
   }

   static int32 ChooseRandomWeatherIndex(int32 seed, int32 count)
   {
      check(count > 0);
      // Just a number to mix in with the seed. Computed with the Python expression: hex(abs(hash("WeatherTypeIndex")) & 0x7fffffff)
      constexpr int32 weatherTypeIndexHash = 0x3f58ee2a;
      FRandomStream randomStream(SeedHelpers::CombineSeed(weatherTypeIndexHash, seed));
      return randomStream.RandHelper(count);
   }

   static TOptional<FGameplayTag> SelectRandomWeatherType(UWorld* world, int32 seed)
   {
      TArray<FGameplayTag, TInlineAllocator<16>> validWeatherTypes;

      const UDataTable* weatherDataTable = UTATWeatherSettings::Get().WeatherTypeDataTable.LoadSynchronous();
      if (weatherDataTable == nullptr)
      {
         return NullOpt;
      }

      check(weatherDataTable->GetRowStruct() == FTATWeatherTypeInfo::StaticStruct());
      constexpr const TCHAR* contextString = TEXT("ActiveQuestHelpers::SelectRandomWeatherType");
      weatherDataTable->ForeachRow<FTATWeatherTypeInfo>(contextString,
         [&validWeatherTypes, world](const FName& key, const FTATWeatherTypeInfo& info)
         {
            if (info.IsWeatherTypeAllowedInLevel(world))
            {
               validWeatherTypes.Add(info.WeatherType);
            }
         });

      if (validWeatherTypes.IsEmpty())
      {
         return NullOpt;
      }

      const int32 weatherTypeIndex = ChooseRandomWeatherIndex(seed, validWeatherTypes.Num() - 1);
      if (ensure(validWeatherTypes.IsValidIndex(weatherTypeIndex)))
      {
         return validWeatherTypes[weatherTypeIndex];
      }

      return NullOpt;
   }

   static void AddWeatherTags(UWorld* world, FGameplayTagContainer& worldTags)
   {
      bool isDefaultWeatherType = false;
      worldTags.AddTag(UTATWeatherSettings::GetCurrentWeatherType(world, isDefaultWeatherType));
      if (const FTATWeatherTypeInfo* weatherTypeInfo = UTATWeatherSettings::Get().GetCurrentWeatherTypeInfo(world))
      {
         if (weatherTypeInfo->WeatherType.IsValid())
         {
            worldTags.AddTag(weatherTypeInfo->WeatherType);
         }
         if (weatherTypeInfo->GameplayMetadata.VisibilityIndoors.IsValid())
         {
            worldTags.AddTag(weatherTypeInfo->GameplayMetadata.VisibilityIndoors);
         }
         if (weatherTypeInfo->GameplayMetadata.VisibilityOutdoors.IsValid())
         {
            worldTags.AddTag(weatherTypeInfo->GameplayMetadata.VisibilityOutdoors);
         }
         if (weatherTypeInfo->GameplayMetadata.SoundCarryIndoors.IsValid())
         {
            worldTags.AddTag(weatherTypeInfo->GameplayMetadata.SoundCarryIndoors);
         }
         if (weatherTypeInfo->GameplayMetadata.SoundCarryOutdoors.IsValid())
         {
            worldTags.AddTag(weatherTypeInfo->GameplayMetadata.SoundCarryOutdoors);
         }
      }
   }

   static void AddDifficultyTag(UWorld* world, FGameplayTagContainer& worldTags)
   {
      const ETATDifficulty difficulty = TATDifficulty::GetDifficultyForMatch(world);
      const FGameplayTag difficultyTag = TATDifficulty::GetDifficultyTag(difficulty);
      if (difficultyTag.IsValid())
      {
         worldTags.AddTag(difficultyTag);
      }
   }

   static bool GenerateModularQuest(const UTATQuestGraph* questGraph, const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& context)
   {
      check(questGraph != nullptr);

   #if WITH_EDITOR
      if (!questGraph->Map.IsNull())
      {
         const FString expectedMapName = GetNameSafe(questGraph->Map.Get());
         const FString actualMapName = GetNameSafe(params.World);
         if (expectedMapName != actualMapName)
         {
            UE_LOG(LogTATActiveQuestSubsystem, Warning, TEXT("Expected quest graph %s to have map %s, got %s"),
               *GetNameSafe(questGraph), *expectedMapName, *actualMapName);
         }
      }
   #endif

      if (!questGraph->EvalQuestGraph(params, context))
      {
         UE_LOG(LogTATActiveQuestSubsystem, Error, TEXT("Failed to generate valid quest plan from graph %s (seed=%d)"),
            *GetNameSafe(questGraph), params.MapSeed);
         return false;
      }

      if (!ensure(context.QuestPlan.Num() > 0))
      {
         return false;
      }

      if (!ensure(context.QuestPlan[0] != nullptr && context.QuestPlan[0]->IsRootNode()))
      {
         return false;
      }

      return true;
   }
   
   const TCHAR* Indent(int32 level)
   {
      // Just return different suffixes of same buffer
      constexpr int kIndentSize = 4;
      constexpr int kMaxIndentCount = 10;
      static constexpr TCHAR kIndentBuffer[] = TEXT("                                        ");
      static_assert(sizeof(kIndentBuffer) == sizeof(TCHAR)*((kMaxIndentCount*kIndentSize)+1));
      return kIndentBuffer + kIndentSize*(kMaxIndentCount - FMath::Clamp(level, 0, kMaxIndentCount));
   }

   void FPlayerMask::Reset()
   {
      Mask = 0;
   }

   void FPlayerMask::Add(int32 playerId)
   {
      if (ensure(playerId >= 0 && playerId < 32))
      {
         Mask |= (1 << playerId);
      }
   }

   bool FPlayerMask::Contains(int32 playerId) const
   {
      if (playerId >= 0 && playerId < 32)
      {
         return (Mask & (1 << playerId)) != 0;
      }

      return false;
   }

   void DebugLogModularQuestEvalLog(TConstArrayView<FTATQuestGraphLogMessage> evalLog)
   {
      UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("Quest graph eval log (%i entries):"), evalLog.Num());
      for (const FTATQuestGraphLogMessage& msg : evalLog)
      {
         if (msg.IsError)
         {
            UE_LOG(LogTATActiveQuestSubsystem, Error, TEXT("    %s"), *msg.ToString());
         }
         else
         {
            UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("    %s"), *msg.ToString());
         }
      }
   }

   void DebugLogModularQuestPlan(const FTATQuestGraphEvalContext& context)
   {
      // Reconstructs the "depth" of each quest graph node so we can display the correct indentation
      struct FGraphStack
      {
         TArray<UTATQuestGraphBase*> Stack;

         int32 Depth() const { return Stack.Num(); }

         void Next(UTATQuestGraphNode* node)
         {
            check(node != nullptr);
            UTATQuestGraphBase* graph = Cast<UTATQuestGraphBase>(node->GetGraph());
            if (!ensure(graph != nullptr))
            {
               return;
            }
            if (Stack.Num() == 0)
            {
               Stack.Add(graph);
            }
            else if (Stack.Last() != graph)
            {
               if (node->IsRootNode())
               {
                  Stack.Add(graph);
               }
               else if (ensure(Stack.Num() > 1 && Stack[Stack.Num() - 2] == graph))
               {
                  Stack.RemoveAt(Stack.Num() - 1);
               }
            }
         }
      };

      UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("==================== Quest Plan (%i) ===================="), context.QuestPlan.Num());
      FGraphStack graphStack;
      for (int32 i = 0; i < context.QuestPlan.Num(); i++)
      {
         if (UTATQuestGraphNode* node = context.QuestPlan[i])
         {
            graphStack.Next(node);
            UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("%s[%i] %s"), Indent(graphStack.Depth() + 1), i, *node->GetNodeDebugName());
         }
         else
         {
            UE_LOG(LogTATActiveQuestSubsystem, Warning, TEXT("%s[%i] NULL"), Indent(1), i);
         }
      }
   }
}

bool UTATActiveQuestSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATActiveQuestSubsystem::_InitQuestGraph(UWorld* world, const int32 seed)
{
   // NB: GetMissionInfo requires a game instance, and thus should not be called if there might not be one
   const FTATMissionInfo* missionInfo = GetMissionInfo();
   if (missionInfo == nullptr || missionInfo->QuestGraph.IsNull())
   {
      return;
   }


   // TODO: Can we meaningfully avoid a sync load here, since so much depends on it?
   const UTATQuestGraph* questGraph = missionInfo->QuestGraph.LoadSynchronous();
      
   // TODO: Figure out what other world tags we want to collect here to pass to the quest graph
   FTATQuestGraphEvalParams questGraphParams{};
   questGraphParams.World = world;
   questGraphParams.MapSeed = seed;

      
   ActiveQuestHelpers::AddWeatherTags(world, questGraphParams.WorldTags);
   ActiveQuestHelpers::AddDifficultyTag(world, questGraphParams.WorldTags);
#if OSE_CHEATS_ENABLED
   // Allow overriding world tags in editor settings
   if (UTATEditorSettings::Get().OverrideQuestGraphWorldTags)
   {
      questGraphParams.WorldTags = UTATEditorSettings::Get().QuestGraphWorldTags;
   }
#endif

   UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("Generating modular quest; seed = %i"), seed);

#if OSE_CHEATS_ENABLED
   const TArray<FOSEGenericGraphSoftNodeHandle>& nodeOverrides = UTATEditorSettings::Get().QuestGraphNodeOverrides;
   if (nodeOverrides.Num() > 0)
   {
      UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("    Quest Graph Node Overrides (from per-user editor settings):"));
      for (int32 i = 0; i < nodeOverrides.Num(); i++)
      {
         const FOSEGenericGraphNodeHandle nodeHandle = nodeOverrides[i].LoadSynchronous();
         questGraphParams.ForceSelectNodes.Add(nodeHandle);
         const UOSEGenericGraphNode* node = nodeHandle.GetNode();
         const FString nodeDesc = (node != nullptr) ? node->GetNodeDebugName() : nodeHandle.ToDebugString();
         UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("        [%i] %s"), i, *nodeDesc);
      }
   }
#endif

   FTATQuestGraphEvalContext questGraphContext;

   TArray<FTATQuestGraphLogMessage> evalLogMessages;
   questGraphContext.EvalLog = &evalLogMessages;

#if OSE_CHEATS_ENABLED
   // Allow specifying the initial quest tags in editor settings
   if (UTATEditorSettings::Get().OverrideQuestGraphQuestTags)
   {
      questGraphContext.QuestTags = UTATEditorSettings::Get().QuestGraphQuestTags;
   }

   for (const TSoftObjectPtr<UTATSceneVariantConfig>& softSceneVariant : UTATEditorSettings::Get().SceneVariantOverrides)
   {
      // lazy editor loading doesn't seem to foil this check, but not entirely sure why
      if (UTATSceneVariantConfig* variant = softSceneVariant.Get())
      {
         questGraphContext.AddSceneVariant(variant);
      }
   }
#endif

   auto logTagContainer = [](const TCHAR* label, const FGameplayTagContainer& tags)
   {
      UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("    %s (%i):"), label, tags.Num());
      for (int32 i = 0; i < tags.Num(); i++)
      {
         UE_LOG(LogTATActiveQuestSubsystem, Log, TEXT("        [%i] %s"), i, *tags.GetByIndex(i).ToString());
      }
   };
   logTagContainer(TEXT("Input WorldTags"), questGraphParams.WorldTags);
   logTagContainer(TEXT("Input QuestTags"), questGraphContext.QuestTags);

   if (!ActiveQuestHelpers::GenerateModularQuest(questGraph, questGraphParams, questGraphContext))
   {
      UE_LOG(LogTATActiveQuestSubsystem, Error, TEXT("Quest generation failed!"));
   }

   ActiveQuestHelpers::DebugLogModularQuestEvalLog(evalLogMessages);
   ActiveQuestHelpers::DebugLogModularQuestPlan(questGraphContext);
   
   _questGraphResult = questGraphContext.IntoResult();
   _questGraph = questGraph;
}

void UTATActiveQuestSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // Skip for map types where it is not relevant
   if (!UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorldChecked(this).AllowActiveQuests)
   {
      return;
   }

   UWorld* world = GetWorld();

   TATDifficulty::InitDifficulty(world);

   UTATAuthMapSeedSubsystem* seedSubsystem = collection.InitializeDependency<UTATAuthMapSeedSubsystem>();
   check(seedSubsystem);
   const int32 seed = seedSubsystem->GetOrCreateSeed();

   // Pick a random weather type for this match.
   // Note that this respects the weather type data table settings - designers can specify which weather types are valid for each map.
   if (UTATMatchSettings* matchSettings = UTATMatchSettings::GetMutableMatchSettings<UTATMatchSettings>(world))
   {
      if (TOptional<FGameplayTag> weatherType = ActiveQuestHelpers::SelectRandomWeatherType(world, seed))
      {
         UE_LOG(LogTATActiveQuestSubsystem, Verbose, TEXT("Selected random weather type: %s"), *weatherType->ToString());
         matchSettings->WeatherType = *weatherType;
      }
   }

   const ActiveQuestHelpers::FResolvedContract resolvedContract = ActiveQuestHelpers::ResolveContracts(world);
   _contract = resolvedContract.ContractTag;
   _playersOfficiallyOnContract = resolvedContract.OfficialPlayers;
   
   _mission = ActiveQuestHelpers::ResolveMission(world, seed);
   
   if (_mission.IsValid())
   {
      UE_LOG(LogTATActiveQuestSubsystem, Verbose, TEXT("Selected mission: %s"), *_mission.ToString());
      _InitQuestGraph(world, seed);
   }
}

const FTATMissionInfo* UTATActiveQuestSubsystem::GetMissionInfo() const
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   return questDataSubsystem.FindMissionInfo(_mission);
}

TOptional<float> UTATActiveQuestSubsystem::GetEndgameDurationOverride() const
{
   return TATQuestGraphUtil::TryGetProperty<float>(_questGraphResult.Properties, ETATQuestGraphPropertyType::EndgameDuration);
}

TOptional<float> UTATActiveQuestSubsystem::GetQuestProperty_Float(ETATQuestGraphPropertyType propType) const
{
   return TATQuestGraphUtil::TryGetProperty<float>(_questGraphResult.Properties, propType);
}

const FTATQuestObjectiveInfo* UTATActiveQuestSubsystem::GetContractObjective() const
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   if (const FTATContractInfo* info = questDataSubsystem.FindContractInfo(_contract))
   {
      return info->GetObjective();
   }

   return nullptr;
}

TConstArrayView<TSoftObjectPtr<UWorld>> UTATActiveQuestSubsystem::GetContractSublevels() const
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   if (const FTATContractInfo* info = questDataSubsystem.FindContractInfo(_contract))
   {
      return info->SublevelsToLoad;
   }

   return {};
}

bool UTATActiveQuestSubsystem::IsPlayerAccompliceForContract(int32 playerId) const
{
   return !_playersOfficiallyOnContract.Contains(playerId);
}

void UTATActiveQuestSubsystem::ForEachObjective(TFunctionRef<void(const FTATQuestObjectiveInfo&)> handler) const
{
   if (const UTATQuestGraphObjectiveNode* objectiveNode = _questGraphResult.ObjectiveNodeHandle.GetNode<UTATQuestGraphObjectiveNode>())
   {
      handler(objectiveNode->GetObjectiveChecked());
   }
   
   if (const FTATQuestObjectiveInfo* contractObjective = GetContractObjective())
   {
      handler(*contractObjective);
   }
}

void UTATActiveQuestSubsystem::ForEachContract(TFunctionRef<void(const FTATContractInfo&)> handler) const
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   if (const FTATContractInfo* info = questDataSubsystem.FindContractInfo(_contract))
   {
      handler(*info);
   }
}

FGameplayTag UTATActiveQuestSubsystem::GetModularQuestClueSourceTag() const
{
   return _mission;
}

const FTATClueFormatParams& UTATActiveQuestSubsystem::GetMissionFormatParams() const
{
   return _questGraphResult.ClueFormatParams;
}

const FTATQuestObjectiveInfo* UTATActiveQuestSubsystem::GetMissionObjective() const
{
   if(const UTATQuestGraphObjectiveNode* node = _questGraphResult.ObjectiveNodeHandle.GetNode<UTATQuestGraphObjectiveNode>())
   {
      return node->Objective.GetPtr<FTATQuestObjectiveInfo>();
   }
   return nullptr;
}

TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> UTATActiveQuestSubsystem::GetMissionSceneVariants() const
{
   return _questGraphResult.SceneVariants;
}

const FGameplayTagContainer& UTATActiveQuestSubsystem::GetMissionQuestTags() const
{
   return _questGraphResult.QuestTags;
}

TConstArrayView<FSoftObjectPath> UTATActiveQuestSubsystem::GetMissionClueSetAssets() const
{
   return _questGraphResult.ClueSets;
}

TConstArrayView<FTATClueSetForSpawn> UTATActiveQuestSubsystem::GetClueSetsForMissionSpawns() const
{
   return _questGraphResult.SpawnSpecificClueSets;
}

void UTATActiveQuestSubsystem::RegisterActorSpawner(UTATQuestActorSpawnerComponent* spawner)
{
   check(spawner);
   _spawners.Add(spawner);
}

bool UTATActiveQuestSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

