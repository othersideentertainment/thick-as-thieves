// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT




#include "Tutorial/TATTutorialRespawnManager.h"

// tat
#include "Settings/TATMatchSettings.h"
#include "Variation/TATActorSpawner.h"
#include "Variation/TATCharacterSpawner.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnPlan.h"
#include "Variation/TATSpawnTiming.h"

// ose
#include "OSEProjectSettings.h"
#include "Player/OSEPlayerState.h"

// ue
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialRespawnManager)

ATATTutorialRespawnManager::ATATTutorialRespawnManager()
{
   PrimaryActorTick.bCanEverTick = true;
}

void ATATTutorialRespawnManager::BeginPlay()
{
   Super::BeginPlay();
   _OverrideRespawnDuration();
}

void ATATTutorialRespawnManager::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   _RestoreRespawnDuration();
   Super::EndPlay(endPlayReason);
}

void ATATTutorialRespawnManager::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   // Tick until the player state is found
   // This is likely the first frame, but don't want to assume that
   if (AOSEPlayerState* playerState = AOSEPlayerState::GetLocalOSEPlayerState(this))
   {
      playerState->GetAbilitySystemComponent()->RegisterGameplayTagEvent(UOSEProjectSettings::Get().ConditionUnconsciousTag, EGameplayTagEventType::NewOrRemoved)
         .AddUObject(this, &ThisClass::_OnUnconsciousTagChanged);
      SetActorTickEnabled(false);
   }
}

void ATATTutorialRespawnManager::_OnUnconsciousTagChanged(FGameplayTag gameplayTag, int count)
{
   const bool isUnconscious = count > 0;

   if(isUnconscious)
   {
      _SuspendTutorial();
   }
   else
   {
      _ResetWorld();
      _ResumeTutorial();
   }
}

void ATATTutorialRespawnManager::_ResetWorld()
{
   TRACE_CPUPROFILER_EVENT_SCOPE(ATATTutorialRespawnManager::_ResetWorld)
   _DestroyTransientActors();
   FTATSpawnPlan newSpawnPlan;

   // Go through the spawners that spawn actors specifically (rather than all spawners total)
   // (not clue or quest spawners, though that should be NA)
   for (ATATActorSpawner* actorSpawner : TActorRange<ATATActorSpawner>(GetWorld()))
   {
      if (ATATCharacterSpawner* characterSpawner = Cast<ATATCharacterSpawner>(actorSpawner))
      {
         // If it is a character spawner, destroy and re-create the actor if it is still there
         //
         // TODO: We will need to exclude some characters from being re-created (at a minimum the
         //       taken-down guard after taking it down). Although it may be that destroying it is good enough.
         if (AActor* spawnedActor = characterSpawner->GetSpawnedActor(); IsValid(spawnedActor))
         {
            FTATSpawnPlanEntry& entry = newSpawnPlan.Spawns.Emplace_GetRef(actorSpawner->GetSpawnerComponent());
            entry.ClassToSpawn = spawnedActor->GetClass();
            entry.DidSpawn = true;
            spawnedActor->Destroy();
         }
      }
      else
      {
         // For non-character spawners, spawn them again if they are missing
         //
         // We can get what spawners actual spawn from the spawn plan, but for now assume it spawned a thing
         // if not disabled, which may be true in the FTUE level.
         UTATSpawnerComponent* spawnerComponent = actorSpawner->GetSpawnerComponent();
         check(spawnerComponent);
         if(spawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled &&
            spawnerComponent->GetSpawnBucketAsset() &&
            !IsValid(actorSpawner->GetSpawnedActor()))
         {
            check(spawnerComponent->GetSpawnBucketAsset()->Entries.Num() > 0);
            FTATSpawnPlanEntry& entry = newSpawnPlan.Spawns.Emplace_GetRef(actorSpawner->GetSpawnerComponent());
            entry.DidSpawn = true;
            entry.ClassToSpawn = spawnerComponent->GetSpawnBucketAsset()->Entries[0].ClassToSpawn;
         }
      }
   }
   newSpawnPlan.Execute(ETATSpawnTiming::Initial);
}

void ATATTutorialRespawnManager::_DestroyTransientActors()
{
   TRACE_CPUPROFILER_EVENT_SCOPE(ATATTutorialRespawnManager::_DestroyTransientActors)
   for(const TSoftClassPtr<AActor>& softClass : _baseClassesToDestroy)
   {
      UClass* actorClass = softClass.Get();
      // If the class isn't loaded, then it isn't there to be destroyed
      if(actorClass == nullptr)
      {
         continue;
      }

      for (AActor* actorToDestroy : TActorRange<AActor>(GetWorld(), actorClass))
      {
         actorToDestroy->Destroy();
      }
   }
}

void ATATTutorialRespawnManager::_SuspendTutorial()
{
   if(UTATTutorialRunnerComponent* runner = UTATTutorialRunnerComponent::Find(GetWorld()))
   {
      _tutorialRunner = runner;
      _tutorialSnapshot = runner->SnapshotStateToResumeTo();
      runner->StopRunning();
   }
}

void ATATTutorialRespawnManager::_ResumeTutorial()
{
   if(UTATTutorialRunnerComponent* runner = _tutorialRunner.Get())
   {
      runner->StartRunningAtSnapshot(_tutorialSnapshot);
      _tutorialRunner.Reset();
   }
}

void ATATTutorialRespawnManager::_OverrideRespawnDuration()
{
   // Override the respawn duration in match settings while active
   //
   // If game stops using the match setting, this code should hopefully break
   // (although it might not if the setting is left around, and merely no longer used)
   if (UTATMatchSettings* matchSettings = UTATMatchSettingsBase::GetMutableMatchSettings<UTATMatchSettings>(this))
   {
      _previousRespawnDuration = matchSettings->RespawnDuration;
      matchSettings->RespawnDuration = _respawnDurationOverride;
   }
}

void ATATTutorialRespawnManager::_RestoreRespawnDuration()
{
   // Match settings live on the game instance, and are not guaranteed to be reset, except in specific flows in the lobby
   // So restoring the value here
   if (UTATMatchSettings* matchSettings = UTATMatchSettingsBase::GetMutableMatchSettings<UTATMatchSettings>(this))
   {
      matchSettings->RespawnDuration = _previousRespawnDuration;
   }
}


