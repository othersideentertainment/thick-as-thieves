// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/MapVariationValidationUtl.h"

// tat
#include "Developer/TATCycleChecker.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/SceneVariants/TATLayerSceneRequirement.h"

// ue4
#include "EngineUtils.h"
#if WITH_EDITOR
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#endif
#include "Engine/LevelStreamingDynamic.h"
#include "Layers/Layer.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Logging/LogScopedVerbosityOverride.h"
#include "Logging/MessageLog.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"

DEFINE_LOG_CATEGORY(LogTATMapVariation);

FName MapVariationValidationHelper::kValidationLogName = TEXT("MapVariation");
bool MapVariationValidationHelper::sInit = false;

namespace MissionValidationUtl
{
   void FindAllMissionSpawnersInWorld(UWorld* world, TArray<UTATSpawnerComponent*>& outSpawners)
   {
      // heuristic to avoid re-entrant calls
      if (IsRunningCommandlet())
      {

         // Normally, the editor will start streaming levels on the first tick. Update the streaming state manually instead
         if (ULevelInstanceSubsystem* levelInstanceSubsystem = GWorld->GetSubsystem<ULevelInstanceSubsystem>())
         {
            levelInstanceSubsystem->OnUpdateStreamingState();
         }

         // Make sure all requested sublevels are finished loading before we run map check
         GWorld->FlushLevelStreaming();


         // force an engine tick to resolve sublevel and world partition changes
         GEngine->TickDeferredCommands();
      }

      for (FActorIterator it(world); it; ++it)
      {
         AActor* actor = (*it);
         ULevel* level = actor->GetLevel();
         UWorld* outerWorld = Cast<UWorld>(level->GetOuter());

         // not sure what this would be, so skip it?
         if (!outerWorld)
         {
            continue;
         }

         TInlineComponentArray<UActorComponent*> spawnerComponents;
         actor->GetComponents(UTATSpawnerComponent::StaticClass(), spawnerComponents);

         // not a spawner, don't care
         if (spawnerComponents.Num() == 0)
            continue;

         // skip any actors that aren't on an always-loaded level or one of our loaded sublevels
         bool isActorInValidSublevel = true;

         // NOTE: In the editor all the actors are on "Persistent" levels so we can't just query their level
         // and cast to dynamic to see if it's blueprint loaded, hence this workaround
         // TODO: if we stick with pure WP, this may not be relevant
         const TArray<ULevelStreaming*>& worldLevels = world->GetStreamingLevels();
         for(ULevelStreaming* worldLevel : worldLevels)
         {
            TSoftObjectPtr<UWorld> worldLevelObjPtr = worldLevel->GetWorldAsset();
            TSoftObjectPtr<UWorld> outerWorldObjPtr = outerWorld;

            // we worked backward to find a reference to the "real" level!  now we can check if it's dynamic / bp loaded
            if (worldLevelObjPtr.GetAssetName() == outerWorldObjPtr.GetAssetName())
            {
               if (ULevelStreamingDynamic* dynamicLevel = Cast<ULevelStreamingDynamic>(worldLevel))
               {
                  // assume a dynamic level is not valid unless it is a level instance, since you cannot directly control its streaming behavior
                  isActorInValidSublevel = worldLevel->IsA<ULevelStreamingLevelInstance>();
               }
               break;
            }
         }

         // TODO: check against data layers

         if (!isActorInValidSublevel)
            continue;

         for (UActorComponent* comp : spawnerComponents)
         {
            outSpawners.Add(CastChecked<UTATSpawnerComponent>(comp));
         }
      }
   }

#if WITH_EDITOR

   TSharedRef<FUObjectToken> CreateComponentToken(const UActorComponent* component)
   {
      return FUObjectToken::Create(component->GetOwner(), FText::FromString(component->GetReadableName()));
   }

   void ValidateSpawnerIsDefinedInConfig(const UTATSpawnerComponent& spawner, const UTATSpawnDataAsset& spawnDataAsset, FMessageLog& msgLog)
   {
      // only validate spawn group/buckets for spawn group, objective, and objective bucket spawners
      const ETATSpawnChanceType spawnType = spawner.GetSpawnType();
      if (spawnType == ETATSpawnChanceType::UseSpawnGroup)
      {
         const FGameplayTag spawnerGroupTag = spawner.GetSpawnGroupTag();
         UTATActorSpawnBucketAsset* spawnerBucketAsset = spawner.GetSpawnBucketAsset();

         const FTATSpawnGroup* spawnGroup = spawnDataAsset.SpawnGroups.FindByPredicate([spawnerGroupTag](const FTATSpawnGroup& group) { return group.SpawnGroupTag == spawnerGroupTag; });

         // spawn group exists in our ruleset?
         if (!spawnGroup)
         {
            msgLog.Error()
               ->AddToken(CreateComponentToken(&spawner))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(": Spawn group %s is not found in "), *spawnerGroupTag.ToString()))))
               ->AddToken(FUObjectToken::Create(&spawnDataAsset));
         }
      }
   }

   void ValidateSpawnerCycles(const TConstArrayView<const UTATSpawnerComponent*> spawners, FMessageLog& msgLog)
   {
      TTATObjectCycleChecker<UTATSpawnerComponent> cycleChecker([](const UTATSpawnerComponent* spawner, TFunctionRef<void(const UTATSpawnerComponent*)> visitor) {
         if (!spawner->IsEligibleForAutomaticOrdering())
         {
            return;
         }

         if (const UTATSpawnerComponent* dependency = spawner->GetParentSpawner())
         {
            visitor(dependency);
         }
      });

      for (const UTATSpawnerComponent* spawner : spawners)
      {
         if (spawner->IsEligibleForAutomaticOrdering())
         {
            cycleChecker.DetectCycles(spawner, [&msgLog](const TConstArrayView<const UTATSpawnerComponent*>& foundCycle) {
               TSharedRef<FTokenizedMessage> message = msgLog.Error(FText::FromString("Cycle in spawner dependencies: "));
               TATCycleChecker::AddCycleError<UTATSpawnerComponent>(*message, foundCycle, CreateComponentToken);
            });
         }
      }
   }

   static void ValidateLayers(const UTATSpawnDataAsset* spawnData, const UWorld* world, FMessageLog& msgLog)
   {
      for (const FTATLayerSceneRequirement& layerRequirement : spawnData->LayerSceneRequirements)
      {
         const FName layerName = layerRequirement.Layer;
         const bool worldHasLayer = world->Layers.ContainsByPredicate([layerName](const ULayer* layer) { return layer->GetLayerName() == layerName; });
         if(!worldHasLayer)
         {
            msgLog.Error()
               ->AddToken(FUObjectToken::Create(spawnData))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(": Layer '%s' in spawn data not found in world "), *layerName.ToString()))))
               ->AddToken(FUObjectToken::Create(world));
         }

         layerRequirement.Requirement.ValidateRequirement(msgLog, [spawnData]() { return MakeArrayView(spawnData->SceneSets); },
            [spawnData]() { return FUObjectToken::Create(spawnData); });
      }
   }
#endif

   void AddLogContextToken(FTokenizedMessage& message, const UObject* refObject)
   {
      if (refObject == nullptr)
      {
         return;
      }
      if (const UActorComponent* component = Cast<UActorComponent>(refObject))
      {
         message.AddToken(FUObjectToken::Create(component->GetOwner(), FText::FromString(component->GetReadableName())));
      }
      else if (const AActor* actor = Cast<AActor>(refObject))
      {
         // TODO: consider injecting this more globally?
         message.AddToken(FUObjectToken::Create(actor, FText::FromString(actor->GetActorNameOrLabel())));
      }
   }
}

///////////////////////////////////////////////////////////////////
///        MapVariationValidationHelper
///////////////////////////////////////////////////////////////////

// static
void MapVariationValidationHelper::InitLog()
{
#if WITH_EDITOR
   if(!sInit)
   {
      sInit = true;

      FMessageLogModule& messageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
      messageLogModule.RegisterLogListing(kValidationLogName, FText::FromString(TEXT("Map Variation")));
   }
#endif // UE_BUILD_SHIPPING
}

// static
void MapVariationValidationHelper::ClearLog()
{
#if WITH_EDITOR
   FMessageLogModule& messageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
   if (messageLogModule.IsRegisteredLogListing(kValidationLogName))
   {
      TSharedRef<IMessageLogListing> logListing = (messageLogModule.GetLogListing(kValidationLogName));
      logListing->ClearMessages();
   }
#endif // UE_BUILD_SHIPPING
}

// static
void MapVariationValidationHelper::ValidateSpawnConfig(const UTATSpawnDataAsset& spawnDataAsset, UWorld* world, FMessageLog& messageLog, ETATSpawnValidationReason reason)
{
#if WITH_EDITOR
   check(world);

   // run validation on some of our mission asset; this happens as part of data validation but seems nice to show here too
   {
      FDataValidationContext validationContext;
      const_cast<UTATSpawnDataAsset&>(spawnDataAsset).IsDataValid(validationContext);
      for (const FDataValidationContext::FIssue& issue : validationContext.GetIssues())
      {
         messageLog.Error()
            ->AddToken(FUObjectToken::Create(&spawnDataAsset))
            ->AddToken(FTextToken::Create(issue.Message));
      }
   }

   TArray<UTATSpawnerComponent*> spawners;
   MissionValidationUtl::FindAllMissionSpawnersInWorld(world, spawners);

   auto* tatWorldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   check(tatWorldSettings);

   // ensure all the spawners have groups/buckets that are defined in this mission
   for(UTATSpawnerComponent* spawner : spawners)
   {
      if (reason != ETATSpawnValidationReason::MapCheck)
      {
         spawner->ValidateSpawnerProperties(messageLog);
      }
      MissionValidationUtl::ValidateSpawnerIsDefinedInConfig(*spawner, spawnDataAsset, messageLog);
   }

   MissionValidationUtl::ValidateSpawnerCycles(spawners, messageLog);
   UTATSpawnDataAsset::ValidateSpawnersSatisfyRules(&spawnDataAsset, spawners, messageLog);
   MissionValidationUtl::ValidateLayers(&spawnDataAsset, world, messageLog);
#endif // WITH_EDITOR
}

void MapVariationValidationHelper::LogInfo(const FString& infoStr, const UObject* refObject /* nullptr */)
{
#if !UE_BUILD_SHIPPING
   // message log for the editor
   FMessageLog msgLog(kValidationLogName);
   TSharedRef<FTokenizedMessage> infoMsg = msgLog.Info();
   MissionValidationUtl::AddLogContextToken(*infoMsg, refObject);
   infoMsg->AddToken(FTextToken::Create(FText::FromString(infoStr)));

   // output log for the CI
   if (refObject)
   {
      UE_LOG(LogTATMapVariation, Log, TEXT("[%s] %s"), *refObject->GetName(), *infoStr);
   }
   else
   {
      UE_LOG(LogTATMapVariation, Log, TEXT("%s"), *infoStr);
   }
#endif // UE_BUILD_SHIPPING
}

void MapVariationValidationHelper::LogError(const FString& errorStr, const UObject* refObject /* nullptr */)
{
#if !UE_BUILD_SHIPPING
   // message log for the editor
   FMessageLog msgLog(kValidationLogName);
   TSharedRef<FTokenizedMessage> errorMsg = msgLog.Error();
   MissionValidationUtl::AddLogContextToken(*errorMsg, refObject);
   errorMsg->AddToken(FTextToken::Create(FText::FromString(errorStr)));

   // output log for the CI
   if (refObject)
   {
      UE_LOG(LogTATMapVariation, Error, TEXT("[%s] %s"), *refObject->GetName(), *errorStr);
   }
   else
   {
      UE_LOG(LogTATMapVariation, Error, TEXT("%s"), *errorStr);
   }
#endif // UE_BUILD_SHIPPING
}

#if WITH_EDITOR
void MapVariationValidationHelper::FindAllSpawnersInWorld(UWorld* world, TArray<UTATSpawnerComponent*>& outSpawners)
{
   MissionValidationUtl::FindAllMissionSpawnersInWorld(world, outSpawners);
}

TConstArrayView<TObjectPtr<UTATSceneSetAsset>> MapVariationValidationHelper::FindRelevantSceneSets(const AActor* actor)
{
   check(actor);

   // Use editor-only field in level instance world info for more specific validation (or when editing solo)
   if (const ULevel* level = actor->GetLevel())
   {
      constexpr bool checked = false;
      if (const auto* levelWorldSettings = Cast<ATATWorldSettings>(level->GetWorldSettings(checked)))
      {
         if (levelWorldSettings->ValidationSceneSet)
         {
            return MakeArrayView(&levelWorldSettings->ValidationSceneSet, 1);
         }
      }
   }

   const UWorld* world = actor->GetWorld();
   check(world);

   const auto* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   check(worldSettings);

   if (const UTATSpawnDataAsset* spawnData = worldSettings->SpawnData)
   {
      return MakeArrayView(spawnData->SceneSets);
   }

   return TConstArrayView<TObjectPtr<UTATSceneSetAsset>>();
}

#endif
