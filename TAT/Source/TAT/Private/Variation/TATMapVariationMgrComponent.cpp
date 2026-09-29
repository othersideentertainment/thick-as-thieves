// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATMapVariationMgrComponent.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATAuthMapSeedSubsystem.h"
#include "GameFramework/TATWorldSettings.h"
#include "Online/TATGameState.h"
#include "Variation/Clues/TATBriefingClue.h"
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATClueSpawnTypes.h"
#include "Variation/Clues/TATClueSpawnerSubsystem.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATClueTextUtils.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATRegularSpawnerClueSource.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnPlan.h"
#include "Variation/TATSpawnTiming.h"
#include "Variation/TATSpawnerHelper.h"
#include "Variation/TATSpawnerRegistrySubsystem.h"
#include "Variation/SceneVariants/TATSceneVariantClueSource.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"
#include "Quests/Spawn/TATQuestSpawnTypes.h"
#include "Quests/Spawn/TATQuestSpawnUtils.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATQuestObjective.h"

// ose
#include "OSECoreCheats.h"

// ue5
#include "Engine/AssetManager.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "Net/UnrealNetwork.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMapVariationMgrComponent)

namespace VariationCvars
{
   static TAutoConsoleVariable<float> CVarSpawnTimeSliceMaxMs(
      TEXT("tat.Spawn.TimeSliceMaxMs"),
      10.0f,
      TEXT("The maximum time per tick it can spend executing spawns (after the initial spawns)"),
      ECVF_Default);
}

UTATMapVariationMgrComponent::UTATMapVariationMgrComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = true;
   // Setting to PostUpdateWork so that it happens after UpdateLevelStreaming in order to catch the existence of non-WP level instance streaming
   // This coud be done by explicitly calling UpdateLevelStreaming once, but that might not work for nested level instances
   PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

   SetIsReplicatedByDefault(true);
}

bool UTATMapVariationMgrComponent::IsMapReady() const
{
   switch(_authorityLoadingState)
   {
   case ETATMapVariationLoadingState::CompleteWithVariation:
      // TODO: do we need a proxy for the replication of spawned actors?
      return _AreDataLayersLoaded() && _AreSublevelsLoaded();
   case ETATMapVariationLoadingState::CompleteNoVariation:
      return true;
   default:
      break;
   }

   // other states are not ready yet
   return false;
}

void UTATMapVariationMgrComponent::AuthorityChooseSceneVariants()
{
   check(GetOwner()->HasAuthority());

   MapVariationValidationHelper::InitLog();
   MapVariationValidationHelper::ClearLog();

   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   UTATActiveQuestSubsystem* activeQuestSubsystem = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>();

   if (UTATSpawnDataAsset* spawnData = worldSettings.SpawnData)
   {
      const int32 seed =  _AuthorityGetSeed();

      _missionChoiceTags.Reset();

      TArray<TObjectPtr<UTATSceneVariantConfig>> variantOverrides;
      if (activeQuestSubsystem->GetMissionSceneVariants().Num() > 0)
      {
         variantOverrides = activeQuestSubsystem->GetMissionSceneVariants();
      }
#if OSE_CHEATS_ENABLED
      else
      {
         // modular quests are assumed to incorporate overrides already
         // TODO: only do this in a single codepath?
         const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
         for (const TSoftObjectPtr<UTATSceneVariantConfig>& softSceneVariant : editorSettings.SceneVariantOverrides)
         {
            // lazy editor loading doesn't seem to foil this check, but not entirely sure why
            if (UTATSceneVariantConfig* variant = softSceneVariant.Get())
            {
               variantOverrides.Add(variant);
            }
         }
      }
#endif

      // TODO(Spawning): Are choice tags still useful?
      _missionChoiceTags.AppendTags(activeQuestSubsystem->GetMissionQuestTags());

      TArray<FTATQuestActorSpawnRequest> spawnRequests;
      activeQuestSubsystem->ForEachObjective([&spawnRequests, world = GetWorld()](const FTATQuestObjectiveInfo& objective)
      {
         objective.AddToActorSpawns(world, spawnRequests);
      });

      // Add traits for active quest locations
      // TODO(Spawning): Are limited scene traits still useful?
      TArray<FTATSceneTraitWithLimit> limitedSceneTraits;
      for (const FTATQuestActorSpawnRequest& spawnRequest : spawnRequests)
      {
         FTATSceneTraitWithLimit::Add(limitedSceneTraits, spawnRequest.LocationTag);
      }

      TArray<FGameplayTag> extraAllowedTraits;
      {
         // Add difficulty to allowed traits, so regular variant selection can use it
         const ETATDifficulty difficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
         const FGameplayTag difficultyTag = TATDifficulty::GetDifficultyTag(difficulty);
         if (difficultyTag.IsValid())
         {
            extraAllowedTraits.Add(difficultyTag);
         }
      }

      FTATSceneVariantSelectionParams params;
      params.Seed = seed;
      params.AllowWarningLogs = true;
      params.LimitedTraits = limitedSceneTraits;
      params.VariantOverrides = variantOverrides;
      params.ExtraAllowedTraits = extraAllowedTraits;

      spawnData->SelectVariants(params, [this](const UTATSceneAsset* scene, const UTATSceneVariantConfig* variant, int32 variantIndex)
      {
         _activeVariants.AddVariant(scene, variant);
         // Set sentinel if none is selected
         _replicatedVariantIndices.AddVariant(variant ? variantIndex : 0xFF);
      });

#if !NO_LOGGING
      FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
      _activeVariants.WriteToMessageLog(msgLog);
#endif
   }

   TATSceneVariantClueSource::AddClueSourcesFromVariants(_activeVariants.GetVariants(), _pendingClueSources);

   // Initialized even without any data
   _replicatedVariantIndices.MarkInitialized();
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _replicatedVariantIndices, this);

   _variantsInitialized = true;
   _InitActiveLayers();
}

ETATMapVariationLoadingState UTATMapVariationMgrComponent::GetCurrentMapVariationLoadingState() const
{
   check(GetOwner()->HasAuthority());
   return _authorityLoadingState;
}

void UTATMapVariationMgrComponent::AuthorityAddPendingClues(const FTATPendingClueSource& clues)
{
   check(GetOwner()->HasAuthority());
   // If level is NA, just skip
   if(_authorityLoadingState == ETATMapVariationLoadingState::CompleteNoVariation)
   {
      return;
   }
   check(_authorityLoadingState < ETATMapVariationLoadingState::WaitingForClues);
   _pendingClueSources.Add(clues);
}

void UTATMapVariationMgrComponent::AuthorityCallOrWaitForComplete(FSimpleDelegate&& delegate)
{
   check(GetOwner()->HasAuthority());
   if (_authorityLoadingState == ETATMapVariationLoadingState::CompleteNoVariation || _authorityLoadingState == ETATMapVariationLoadingState::CompleteWithVariation)
   {
      delegate.ExecuteIfBound();
   }
   else
   {
      _onVariationComplete.Add(delegate);
   }
}

int32 UTATMapVariationMgrComponent::GetBriefingClueCount() const
{
   return _briefingClues.Num();
}

FText UTATMapVariationMgrComponent::GetBriefingClueAt(int32 index) const
{
   if (_briefingClues.IsValidIndex(index))
   {
      float matchDuration = 0.0f;
      float endgameDuration = 0.0f;
      if (ATATGameState* gs = ATATGameState::GetTATGameState(this))
      {
         // Divide by 60.0f to convert from seconds -> minutes
         matchDuration = gs->GetTotalMatchDuration() / 60.0f;
         const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
         endgameDuration = UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty) / 60.0f;
      }
      FNumberFormattingOptions formatOptions;
      formatOptions.MaximumFractionalDigits = 0;
      FText formattedBriefingClues = FText::Format(_briefingClues[index].ClueText, FFormatNamedArguments{
         {"MatchDurationInMinutes", FText::AsNumber(matchDuration, &formatOptions)},
            {"EndgameDurationInMinutes", FText::AsNumber(endgameDuration, &formatOptions)} });
      return formattedBriefingClues;
   }
   else
   {
      return FText::GetEmpty();
   }
}

void UTATMapVariationMgrComponent::ForEachInitialClueThunk(TFunctionRef<void(const FTATClueFactThunk&)> handler) const
{
   for (const FTATBriefingClueData& briefingData : _briefingClues)
   {
      if (briefingData.Facts)
      {
         handler(*briefingData.Facts);
      }
   }
}

void UTATMapVariationMgrComponent::AuthorityExecuteSpawnTiming(ETATSpawnTiming timing)
{
   check(GetOwner()->HasAuthority());
   if ((_executedSpawnTimingsMask & SpawnTimingHelpers::TimingToMask(timing)) != 0)
   {
      return;
   }

   _executedSpawnTimingsMask |= SpawnTimingHelpers::TimingToMask(timing);
   _timeslicedSpawns.Emplace(timing);
}

void UTATMapVariationMgrComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      if (UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorldChecked(this).RunMapVariation)
      {
         _AuthorityBeginVariation();
      }
      else
      {
         // no variation in other map types
         _AuthoritySetLoadingState(ETATMapVariationLoadingState::CompleteNoVariation);
      }
   }
}

void UTATMapVariationMgrComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
   
   Super::EndPlay(endPlayReason);
}

void UTATMapVariationMgrComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _authorityLoadingState, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _briefingClues, params);

   params.Condition = COND_InitialOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _replicatedVariantIndices, params);
}

void UTATMapVariationMgrComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (GetOwner()->HasAuthority())
   {
      // authority tick
      _AuthorityTickWaitForLoad();
      _AuthorityTickTimeslicedSpawns();
   }
}

void UTATMapVariationMgrComponent::_AuthorityBeginVariation()
{
   check(GetOwner()->HasAuthority());
   
   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   if (UTATSpawnDataAsset* missionSpawnData = worldSettings.SpawnData)
   {
      _authoritySpawnData = missionSpawnData;
      
      _AuthorityRequestSublevelLoad();

      // ... now wait to do our randomization until all the layers are loaded since they may contain spawners
      _AuthoritySetLoadingState(ETATMapVariationLoadingState::WaitingForLoad);
   }
   else
   {
      UE_LOG(LogTATMapVariation, Log, TEXT("Loading into a mission map with no spawn data"));

      // consider this complete for no-mission maps
      _AuthoritySetLoadingState(ETATMapVariationLoadingState::CompleteNoVariation);
   }
}

void UTATMapVariationMgrComponent::_AuthorityTickWaitForLoad()
{
   check(GetOwner()->HasAuthority());

   if (_authorityLoadingState == ETATMapVariationLoadingState::WaitingForLoad)
   {
      if (_AreDataLayersLoaded() && _AreLevelInstancesLoaded() && _AreSublevelsLoaded())
      {
         TArray<const AActor*> externalSpawners;
         _AuthorityRunQuestSpawners(externalSpawners);
         _AuthorityRunSpawners(externalSpawners);
         _AuthorityLoadClues();
      }
   }
}

void UTATMapVariationMgrComponent::_AuthorityRunQuestSpawners(TArray<const AActor*>& chosenSpawners)
{
   check(GetOwner()->HasAuthority());

   const UTATActiveQuestSubsystem* activeQuests = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>();
   if (activeQuests == nullptr)
   {
      return;
   }

   TArray<FTATQuestActorSpawnRequest> spawnRequests;
   activeQuests->ForEachObjective([&spawnRequests, world = GetWorld()](const FTATQuestObjectiveInfo& objective)
      {
         objective.AddToActorSpawns(world, spawnRequests);
      });

   FTATQuestSpawnPlanParams params;
   params.SpawnRequests = spawnRequests;
   params.QuestSpawners = activeQuests->GetSpawners();
   params.Variants = &_activeVariants;
   params.Seed = _AuthorityGetSeed();

   FTATQuestSpawnPlan plan;
   TATQuestSpawnUtils::GeneratePlan(plan, params);
   TATQuestSpawnUtils::ExecutePlan(plan);
   TATQuestSpawnUtils::EmitSpawnerActors(chosenSpawners, plan);
   _AddClueRequestsForQuests(plan);
}

void UTATMapVariationMgrComponent::_AddClueRequestsForQuests(const TArray<FTATQuestActorSpawn>& plan)
{
   // Quest spawn requests elide the originating quests (since multiple quests can spawn the same thing),
   // so have to reconstruct the quests that spawned things in order to make clues for this.
   // CLUE-WIP: If clues are going to be fed into the simulator, this is more interesting than probably should be here.(extract?)
   // CLUE-WIP: Since there is a use-case, should originating quests be threaded into this spawn data? (would likely have to be an array)
   const int32 seed = _AuthorityGetSeed();
   const UTATActiveQuestSubsystem* activeQuests = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>();
   check(activeQuests);
   
   {
      FTATSharedClueFormatParams formatParams;
      if (!activeQuests->GetMissionFormatParams().IsEmpty())
      {
         formatParams = MakeSharedClueFormatParams(CopyTemp(activeQuests->GetMissionFormatParams()));
      }

      for(const FSoftObjectPath& softClueSet : activeQuests->GetMissionClueSetAssets())
      {
         _pendingClueSources.Add(FTATPendingClueSource{
            .ClueSet = TSoftObjectPtr<UTATClueSetBase>(softClueSet),
            .Location = UTATDummyClueLocation::Get(), //< TODO: figure out if still useful
            .SourceTag = activeQuests->GetModularQuestClueSourceTag(),
            .ExtraFormatParams = formatParams,
            .ContextTags = _missionChoiceTags,
         });
      }
   }

   {
      // Keep track of which spawns were used, so a given spawn is only taken by a single spawn-specific clue set
      TArray<int32> usedSpawnIndices;
      auto takeSpawn = [&plan, &usedSpawnIndices](const FGameplayTag& actorTag) -> const FTATQuestActorSpawn*
      {
         const int num = plan.Num();
         for (int i = 0; i < num; ++i)
         {
            const FTATQuestActorSpawn& spawn = plan[i];
            if (spawn.ActorTag == actorTag && !usedSpawnIndices.Contains(i))
            {
               usedSpawnIndices.Add(i);
               return &spawn;
            }
         }
         return nullptr;
      };

      int lastSourceIndex = 0; //< use separate source indices so they can be treated as distinct even if using the same clues
      for(const FTATClueSetForSpawn& request : activeQuests->GetClueSetsForMissionSpawns())
      {
         const FTATQuestActorSpawn* spawn = takeSpawn(request.ActorTag);
         if (spawn == nullptr)
         {
            UE_LOG(LogTATMapVariation, Warning, TEXT("Could not find quest spawn for actor tag %s"), *request.ActorTag.ToString());
            continue;
         }

         FTATClueFormatParams formatParams = activeQuests->GetMissionFormatParams();
         TATClueTextUtils::AddItemNameToParams(formatParams, request.ActorTag, this);
         
         _pendingClueSources.Add(FTATPendingClueSource{
            .ClueSet = TSoftObjectPtr<UTATClueSetBase>(request.ClueSet),
            .Location = spawn->Spawner.Get(),
            .SourceTag = activeQuests->GetModularQuestClueSourceTag(),
            .SourceIndex = ++lastSourceIndex,
            .ExtraFormatParams = MakeSharedClueFormatParams(MoveTemp(formatParams)),
            .ContextTags = _missionChoiceTags,
         });

         _clueFactNamespaceByQuestSpawner.Add(spawn->Spawner.Get(),
            FTATClueFactNamespace { .SourceTag = activeQuests->GetModularQuestClueSourceTag(), .SourceIndex = lastSourceIndex });
      }
   }
   
   TArray<FGameplayTag, TInlineAllocator<4>> seenContracts;

   auto findContractSpawn = [&plan] (const FTATContractInfo& contract) -> const FTATQuestActorSpawn*
   {
      // CONSIDER: is this making too many assumptions in reconstructing the related contract? (not now, but maybe in future)
      // TODO: Wean ourselves off uses of the singular form of GetRelatedLootTag (and possibly this lookup altogether)
      FGameplayTag lootTag = contract.GetObjectiveChecked().GetRelatedLootTag();
      if(!lootTag.IsValid())
      {
         return nullptr;
      }

      return plan.FindByKey(lootTag);
   };
   
   activeQuests->ForEachContract([this, &seenContracts, findContractSpawn, seed](const FTATContractInfo& contract)
   {
      if(_pendingClueSources.ContainsByPredicate(
         [questTag = contract.QuestTag] (const FTATPendingClueSource& source) { return source.SourceTag == questTag; }))
      {
         return;
      }

      if (seenContracts.Contains(contract.QuestTag))
      {
         return;
      }
      seenContracts.Add(contract.QuestTag);

      TSoftObjectPtr<UTATClueSetBase> clueSet;
      TWeakInterfacePtr<ITATClueLocationInterface> location = UTATDummyClueLocation::Get();
      
      if(const FTATQuestActorSpawn* questSpawn = findContractSpawn(contract))
      {
         clueSet = TSoftObjectPtr<UTATClueSetBase>(UTATClueSet::FindRandomMatchingSet(contract.QuestTag, questSpawn->Spawner->GetClueLocationTag(), seed));
         location = questSpawn->Spawner.Get();
      }

      if(clueSet.IsNull())
      {
         clueSet = contract.Clues;
      }
      
      if(clueSet.IsNull())
      {
         return;
      }

      _pendingClueSources.Add(FTATPendingClueSource {
         .ClueSet = clueSet,
         .Location = location,
         .SourceTag = contract.QuestTag
      });
   });
}

void UTATMapVariationMgrComponent::_AuthorityRunSpawners(TConstArrayView<const AActor*> externalSpawners)
{
   QUICK_SCOPE_CYCLE_COUNTER(UTATMapVariationMgrComponent_AuthorityRunSpawners);
   // do our randomization now that all data layers are loaded
   check(_authoritySpawnData);

   const UTATSpawnerRegistrySubsystem* spawnerRegistry = GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>();
   check(spawnerRegistry);

   FTATSpawnGenerationParams params;
   params.SpawnGroups = _authoritySpawnData->SpawnGroups;
   params.SceneVariants = &_activeVariants;
   params.Spawners = spawnerRegistry->GetSpawners();
   params.Seed = _AuthorityGetSeed();
   params.ExternalSpawners = externalSpawners;

   SpawnHelpers::GeneratePlan(_spawnPlan, params);

#if !NO_LOGGING
   FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
   _spawnPlan.LogGroupSummaries(msgLog);
#endif

   _spawnPlan.Execute(ETATSpawnTiming::Initial);
   _executedSpawnTimingsMask = SpawnTimingHelpers::TimingToMask(ETATSpawnTiming::Initial);
   
   TATRegularSpawnerClueSource::AddClueSourcesFromSpawnPlan(_spawnPlan, _pendingClueSources, _AuthorityGetSeed());

   // pre-load spawn assets so later spawn executions don't have to sync load
   _spawnPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(_spawnPlan.CollectPreloadAssets().Array());
}

void UTATMapVariationMgrComponent::_AuthorityLoadClues()
{
   check(GetOwner()->HasAuthority());

   if(_pendingClueSources.IsEmpty())
   {
      // all done spawning on the server!
      _AuthoritySetLoadingState(ETATMapVariationLoadingState::CompleteWithVariation);
      return;
   }
   
   _AuthoritySetLoadingState(ETATMapVariationLoadingState::WaitingForClues);
   
   TArray<FSoftObjectPath> pathsToLoad;
   pathsToLoad.Reserve(_pendingClueSources.Num());
   for(const FTATPendingClueSource& clueSource : _pendingClueSources)
   {
      pathsToLoad.Add(clueSource.ClueSet.ToSoftObjectPath());
   }
   
   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakSelf = MakeWeakObjectPtr(this)]
   {
      if(UTATMapVariationMgrComponent* self = weakSelf.Get())
      {
         self->_AuthoritySpawnClues();
      }
   });
}

void UTATMapVariationMgrComponent::_AuthoritySpawnClues()
{
   TArray<FTATClueRequest> clueRequests;
   clueRequests.Reserve(_pendingClueSources.Num());
   Algo::Transform(_pendingClueSources, clueRequests, [] (const FTATPendingClueSource& pending) { return pending.IntoRequest(); });

   // Take briefing clues first
   UWorld* world = GetWorld();
   for(FTATClueRequest& clueRequest : clueRequests)
   {
      FTATBriefingClueInfo::TakeFromRequest(clueRequest, _briefingClues, world);
   }
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _briefingClues, this);

   UTATClueSpawnerSubsystem* clueSubsystem = GetWorld()->GetSubsystem<UTATClueSpawnerSubsystem>();
   check(clueSubsystem);
   
   FTATClueSpawnParams params;
   params.SpawnRequests = clueRequests;
   params.Spawners = clueSubsystem->GetClueSpawners();
   params.Variants = &_activeVariants;
   params.Seed = _AuthorityGetSeed();

   // NOTE: plan is only valid for stack frame
   FTATClueSpawnPlan plan = TATClueSpawnUtils::GeneratePlan(params);
   plan.Execute();

#if !NO_LOGGING
   if(plan.FailedClues.Num())
   {
      FMessageLog msgLog(MapVariationValidationHelper::kValidationLogName);
      plan.WriteFailuresToLog(msgLog);
   }
#endif
   
   _AuthoritySetLoadingState(ETATMapVariationLoadingState::CompleteWithVariation);
}

bool UTATMapVariationMgrComponent::_AreDataLayersLoaded() const
{
   if (UWorldPartitionSubsystem* worldPartitionSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>())
   {
      // TODO: check specific data layers?
      return worldPartitionSubsystem->IsStreamingCompleted();
   }

   return true;
}

bool UTATMapVariationMgrComponent::_AreSublevelsLoaded() const
{
   // NOTE: This is just checking that dynamic sublevels that want to be loaded are loaded
   //       If it turns out that are loads that we don't wan to wait for, we can keep track
   //       of the specific sublevels that we requested.
   for (ULevelStreaming* level : GetWorld()->GetStreamingLevels())
   {
      if (ULevelStreamingDynamic* dynamicLevel = Cast<ULevelStreamingDynamic>(level))
      {
         if (dynamicLevel->ShouldBeLoaded() && dynamicLevel->ShouldBeVisible() &&
            !(dynamicLevel->IsLevelLoaded() && dynamicLevel->IsLevelVisible()))
         {
            UE_LOG(LogTATMapVariation, VeryVerbose, TEXT("Waiting for level: %s"), *dynamicLevel->GetWorldAssetPackageName());
            return false;
         }
      }
   }

   return true;
}

void UTATMapVariationMgrComponent::_AuthorityRequestSublevelLoad()
{
   check(GetOwner()->HasAuthority());
   
   UTATActiveQuestSubsystem* activeQuestSubsystem = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>();
   if (activeQuestSubsystem == nullptr)
   {
      return;
   }

   TConstArrayView<TSoftObjectPtr<UWorld>> sublevelsToLoad = activeQuestSubsystem->GetContractSublevels();
   if (sublevelsToLoad.IsEmpty())
   {
      return;
   }

   auto shouldLoadSublevel = [&sublevelsToLoad](const ULevelStreamingDynamic* dynamicLevel) -> bool
   {
      const FName levelFName = dynamicLevel->GetWorldAsset().ToSoftObjectPath().GetAssetFName();
      return sublevelsToLoad.ContainsByPredicate([levelFName](const TSoftObjectPtr<UWorld>& levelToLoad)
      {
         return levelToLoad.ToSoftObjectPath().GetAssetFName() == levelFName;
      });
   };
   
   for (ULevelStreaming* level : GetWorld()->GetStreamingLevels())
   {
      // This will be replicated down to clients so they load it as well.
      if (ULevelStreamingDynamic* dynamicLevel = Cast<ULevelStreamingDynamic>(level))
      {
         if (shouldLoadSublevel(dynamicLevel))
         {
            UE_LOG(LogTATMapVariation, Log, TEXT("Loading sublevel: %s"), *dynamicLevel->GetWorldAssetPackageName());
            level->SetShouldBeLoaded(true);
            level->SetShouldBeVisible(true);
         }
      }
   }
}

bool UTATMapVariationMgrComponent::_AreLevelInstancesLoaded() const
{
   // Assume that level instances might have spawners that could be relevant
   for (ULevelStreaming* level : GetWorld()->GetStreamingLevels())
   {
      if (ULevelStreamingLevelInstance* levelInstanceLevel = Cast<ULevelStreamingLevelInstance>(level))
      {
         if (!levelInstanceLevel->IsLevelLoaded() || !levelInstanceLevel->IsLevelVisible())
         {
            return false;
         }
      }
   }

   return true;
}

void UTATMapVariationMgrComponent::_AuthoritySetLoadingState(ETATMapVariationLoadingState newState)
{
   check(GetOwner()->HasAuthority());

   _authorityLoadingState = newState;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _authorityLoadingState, this);

   OnMapVariationMgrStateChanged.Broadcast(_authorityLoadingState);
   
   UE_LOG(LogTATMapVariation, Log, TEXT("MapVariationState -> %s"),
      *WriteToString<64>(StaticEnum<ETATMapVariationLoadingState>()->GetNameByValue(static_cast<int64>(newState))));
   if (newState == ETATMapVariationLoadingState::CompleteNoVariation || newState == ETATMapVariationLoadingState::CompleteWithVariation)
   {
      _onVariationComplete.Broadcast();
      _onVariationComplete.Clear();
   }
}

int32 UTATMapVariationMgrComponent::_AuthorityGetSeed() const
{
   // Could also get from game-state as well now
   UTATAuthMapSeedSubsystem* seedSubsystem = GetWorld()->GetSubsystem<UTATAuthMapSeedSubsystem>();
   check(seedSubsystem);
   return seedSubsystem->GetOrCreateSeed();
}

void UTATMapVariationMgrComponent::_AuthorityTickTimeslicedSpawns()
{
   if (_timeslicedSpawns.IsEmpty())
   {
      return;
   }

   TRACE_CPUPROFILER_EVENT_SCOPE(UTATMapVariationMgrComponent::_AuthorityTickTimeslicedSpawns);
   const double endTime = FPlatformTime::Seconds() + (VariationCvars::CVarSpawnTimeSliceMaxMs.GetValueOnGameThread() * 0.001);
   for (FTATSpawnCursor& cursor : _timeslicedSpawns)
   {
      // Not really trying to be fair if there are multiple spawn at the moment
      while (cursor.IsValid(_spawnPlan) && FPlatformTime::Seconds() < endTime)
      {
         cursor.Step(_spawnPlan);
      }
   }

   _timeslicedSpawns.RemoveAll([this](const FTATSpawnCursor& cursor) { return !cursor.IsValid(_spawnPlan); });
}

void UTATMapVariationMgrComponent::_InitActiveLayers()
{
   check(_variantsInitialized);

   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   UTATSpawnDataAsset* spawnData = worldSettings.SpawnData;
   if (spawnData == nullptr)
   {
      return;
   }
   
   _activeLayers.InitFrom(spawnData->LayerSceneRequirements, _activeVariants);
   if (_activeLayers.IsPopulated())
   {
      const int destroyedCount = _activeLayers.DestroyIncompatibleActors(GetWorld());
      UE_LOG(LogTATMapVariation, Verbose, TEXT("Destroyed %d layer-incompatible already-loaded actors"), destroyedCount);
      FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::_OnLevelAddedToWorld);
   }
}

void UTATMapVariationMgrComponent::_OnLevelAddedToWorld(ULevel* level, UWorld* world)
{
   check(level);
   if (world != GetWorld())
   {
      return;
   }

   // NOTE: It would be nice to destroy actors before more of their initialization has run (like NetLoadForClient), but:
   //       1. I don't see a hook for that does not have an engine mod
   //       2. Doing it earlier potentially makes the specific behavior dependent on whether the level was loaded when variants
   //          were evaluated, which could make bugs hard to track down.
   if (ensure(_activeLayers.IsPopulated()))
   {
      const int destroyedCount = _activeLayers.DestroyIncompatibleActors(level);
      UE_LOG(LogTATMapVariation, Verbose, TEXT("Destroyed %d layer-incompatible actors from level %s"), destroyedCount, *level->GetName());
   }
}

void UTATMapVariationMgrComponent::_OnRep_VariantIndices()
{
   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   if (UTATSpawnDataAsset* spawnData = worldSettings.SpawnData)
   {
      spawnData->EmitVariantsForIndices(_replicatedVariantIndices.GetVariantSelections(), [this](const UTATSceneAsset * scene, const UTATSceneVariantConfig * variant)
      {
         _activeVariants.AddVariant(scene, variant);
      });

      UE_LOG(LogTATMapVariation, Log, TEXT("Received %d scene variants on client"), _activeVariants.Num());
      _activeVariants.WriteToLog();
   }
   _variantsInitialized = true;
   _InitActiveLayers();

   OnVariantsReplicated.ExecuteIfBound();
}
