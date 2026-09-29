// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherSubsystem.h"

// tat
#include "Developer/TATWeatherSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Environment/TATWeatherUtilities.h"
#include "Environment/TATWeatherTypeInfo.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "TATGameInstance.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherSubsystem, Warning, All);
DECLARE_STATS_GROUP(TEXT("Weather Subsystem"),STATGROUP_WeatherSubsystem, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Time to check"), STAT_WeatherSubsystem_Tick, STATGROUP_WeatherSubsystem)
DECLARE_DWORD_COUNTER_STAT(TEXT("Polled Actors"), STAT_WeatherSubsystem_PolledActors, STATGROUP_WeatherSubsystem);

UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Indoors, "Status.Indoors");

void UTATWeatherSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
   const UTATWeatherSettings& settings = UTATWeatherSettings::Get();

   _pollIntervalRangeSeconds = settings.ServerIndoorCheck_ActorPollingRateRange;
   _targetTickRateSeconds = settings.ServerIndoorCheck_TargetTickRateSeconds;;
   _maxActorsToPollPerTick = settings.ServerIndoorCheck_MaxActorsToPollPerTick;
   _maxFrameTimeUntilCutoff = settings.ServerIndoorCheck_CutoffFrameTimePerTick;

   if(const UWorld* world = GetWorld())
   {
      if(UTATGameInstance* tatGameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         tatGameInstance->OnMatchSettingsUpdated.AddDynamic(this, &ThisClass::_OnMatchSettingsUpdated);
         if(tatGameInstance->HasMatchSettings())
         {
            _OnMatchSettingsUpdated(&tatGameInstance->GetMatchSettings());
         }
      }
   }
}

void UTATWeatherSubsystem::Deinitialize()
{
   if(const UWorld* world = GetWorld())
   {
      if(UTATGameInstance* tatGameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         tatGameInstance->OnMatchSettingsUpdated.RemoveAll(this);
      }
   }
   Super::Deinitialize();
}

bool UTATWeatherSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer) || outer == nullptr || outer->GetWorld() == nullptr)
   {
      return false;
   }

   // Game world only
   const UWorld* world = outer->GetWorld();
   if (!world->IsGameWorld())
   {
      return false;
   }

   // Only allow the weather subsystem in worlds with a configured weather manager
   ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   if (worldSettings == nullptr || worldSettings->WeatherManager == nullptr)
   {
      return false;
   }

   return true;
}

void UTATWeatherSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);
}

bool UTATWeatherSubsystem::IsTickable() const
{
   return Super::IsTickable() && _polledStates.Num() > 0;
}

void UTATWeatherSubsystem::_ChangeTagsOnActor(AActor* actor, const FGameplayTagContainer& container, const bool add)
{
   if(actor == nullptr)
      return;
   const IAbilitySystemInterface* abilitySystemInterface = Cast<IAbilitySystemInterface>(actor);
   if(abilitySystemInterface == nullptr)
      return;
   UAbilitySystemComponent* abilitySystemComponent = abilitySystemInterface->GetAbilitySystemComponent();
   if(abilitySystemComponent == nullptr)
      return;
   return _ChangeTagsOnActor(abilitySystemComponent, container, add);
}
void UTATWeatherSubsystem::_ChangeTagsOnActor(UAbilitySystemComponent* abilitySystemComponent,
                                             const FGameplayTagContainer& container,
                                             const bool add)
{
   if(abilitySystemComponent == nullptr)
      return;
   // These are _only_ used for NPC's right now, so these aren't replicated. HOWEVER, if we do make these tags replicated
   // note that the AOSECharacterBase::HasMatchingGameplayTag function _only_ checks the UAbilitySystemComponent->HasMatchingGameplayTag
   // which doesn't check replicated loose gameplay tags. So we'd need to either update the ability system components implementation to also
   // take into account the replicated loose tags or do it at the AOSECharacterBase level.
   if(add)
   {
      abilitySystemComponent->AddLooseGameplayTags(container);
   }
   else
   {
      abilitySystemComponent->RemoveLooseGameplayTags(container);
   }
}

void UTATWeatherSubsystem::_OnMatchSettingsUpdated(UTATMatchSettingsBase* newMatchSettings)
{
   // Note: We don't want to grab the match settings weather _directly_ because it could be overridden in the
   // UTATWeatherSettings::GetCurrentWeatherType function.
   _HandleWeatherMetaTagsChanged();
}

void UTATWeatherSubsystem::_HandleWeatherMetaTagsChanged()
{
   bool changed { false };   
   auto updateWeatherTags = [&changed](FGameplayTag& cachedTagValue, const FGameplayTag& currentWeatherValue)
   {
      if(cachedTagValue != currentWeatherValue)
      {
         cachedTagValue = currentWeatherValue;
         changed = true;
      }
   };

   // UTATWeatherSettings::Get().GetCurrentWeatherTypeInfo is deceptively complex. So we don't want to call this often.
   if (const FTATWeatherTypeInfo* weatherTypeInfo = UTATWeatherSettings::Get().GetCurrentWeatherTypeInfo(this))
   {
      updateWeatherTags(_currentWeatherType, weatherTypeInfo->WeatherType);
      updateWeatherTags(_visibilityIndoorsTag, weatherTypeInfo->GameplayMetadata.VisibilityIndoors);
      updateWeatherTags(_visibilityOutdoorsTag, weatherTypeInfo->GameplayMetadata.VisibilityOutdoors);
      updateWeatherTags(_soundCarryIndoorsTag, weatherTypeInfo->GameplayMetadata.SoundCarryIndoors);
      updateWeatherTags(_soundCarryOutdoorsTag, weatherTypeInfo->GameplayMetadata.SoundCarryOutdoors);
   }
   
   if(changed)
   {
      FGameplayTagContainer previousTags;
      previousTags.AppendTags(_indoorTags);
      previousTags.AppendTags(_outdoorTags);

      _indoorTags.Reset();
      _outdoorTags.Reset();

      _indoorTags.AddTag(_currentWeatherType);
      _indoorTags.AddTag(_visibilityIndoorsTag);
      _indoorTags.AddTag(_soundCarryIndoorsTag);
      _indoorTags.AddTag(TAG_Status_Indoors);
      
      _outdoorTags.AddTag(_currentWeatherType);
      _outdoorTags.AddTag(_visibilityOutdoorsTag);
      _outdoorTags.AddTag(_soundCarryOutdoorsTag);
      
      for (FTATWeatherActorPollingState& pollingState : _polledStates)
      {
         _ChangeTagsOnActor(pollingState.TrackedActor.Get(), previousTags, false);
      }
   }
}

void UTATWeatherSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
   SCOPE_CYCLE_COUNTER(STAT_WeatherSubsystem_Tick)

   const float now = GetWorld()->GetTimeSeconds();

   // This code is very similar to FOSESchedulerTaskSet_MaxFrameTimeTick, but the OSESchedulerTask's require a ActorComponent
   // to tick over, in this case, we only have a struct, so it could be some future work to have the OSEScheduler system take
   // a more generic target that uses some sort of interface, that'll allow us to use it with actor components and
   // arbitrary structs implementing that interface.
   const uint64 pollStartTime = FPlatformTime::Cycles64();
   int32 numActorsPolled = 0;

   const double startTime = FPlatformTime::Seconds();
   const double endTime = startTime + _maxFrameTimeUntilCutoff;

   int startTickIndex = _indexToTickIfBreaking;
   const int numberOfTasks = _polledStates.Num();

   // Loop back to zero if the starting index is over or equal to the number of tasks
   if(startTickIndex >= numberOfTasks)
   {
      startTickIndex = 0;
   }
   if(startTickIndex == INDEX_NONE)
   {
      startTickIndex = 0;
   }

   UE_LOG(
     LogTATWeatherSubsystem,
     Verbose,
     TEXT("Starting tick from index %i"), startTickIndex);
   
   int index = startTickIndex;
   // Iterate over all polling actors, decide if they need to be updated, and update them if needed
   for (int i = 0; i < numberOfTasks; ++i)
   {
      FTATWeatherActorPollingState& pollingState = _polledStates[index];

      UE_LOG(
         LogTATWeatherSubsystem,
         Verbose,
         TEXT("Ticking index %i"), index);
      
      if(pollingState.TrackedActor == nullptr)
      {
         continue;
      }
      AActor* actor = pollingState.TrackedActor.Get();
      check(actor != nullptr);
      
      index = index +1;
      // if the index goes off the end of the number of tasks, reset back to zero. 
      if (index >= numberOfTasks)
      {
         index = 0;
      }
      
      // Stash off the last ticked index into the set.
      _indexToTickIfBreaking = index;
      if(index == startTickIndex)
      {
         // in-case of a fast frame, exit out if we've looped around to our starting
         // index as theres no point evaluating twice per frame
         UE_LOG(
           LogTATWeatherSubsystem,
           Verbose,
           TEXT("Breaking out of tick due to fast frame"));
         break;
      }
      if (pollingState.LastUpdateTime == 0 || now - pollingState.LastUpdateTime >= pollingState.NextUpdateInterval)
      {
         pollingState.LastUpdateTime = now;
         pollingState.NextUpdateInterval = _GetRandomPollingIntervalSeconds();
      }
      else
      {
         continue;
      }

      const bool prevInsideValue = pollingState.IsInside;      
      constexpr float minTraceDistance = 0.0f;
      pollingState.IsInside = UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(
         this,
         actor->GetActorLocation(),
         minTraceDistance,
         actor
      );

      if(pollingState.IsInside != prevInsideValue)
      {
         _UpdateActorState(actor, pollingState.IsInside);
      }
      ++numActorsPolled;
      
      if(endTime < FPlatformTime::Seconds())
      {
         // the platform time is now past the end time, break out.
         UE_LOG(
           LogTATWeatherSubsystem,
           Verbose,
           TEXT("Breaking out of tick due to frame time limit"));
         break;
      }
   }

   if (numActorsPolled > 0)
   {
      SET_CYCLE_COUNTER(STAT_WeatherSubsystem_PolledActors, numActorsPolled);
      const uint64 pollDuration = FPlatformTime::Cycles64() - pollStartTime;
      UE_LOG(
         LogTATWeatherSubsystem,
         Verbose,
         TEXT("Polled %i actors in %llu cycles (%.4f seconds) out of %i actors registered for polling"),
         numActorsPolled, pollDuration, FPlatformTime::ToSeconds64(pollDuration), _polledStates.Num());
   }
}

void UTATWeatherSubsystem::RegisterActor(AActor* actor, const bool enablePolling)
{
   if (actor == nullptr)
      return;

   // Now that _polledStates is an array, we need to find by predicate, this is _slightly_ slower than a map lookup,
   // but the array should be fairly small and this function should be called only on begin play of the actor being tracked.
   const FTATWeatherActorPollingState* existingState = _polledStates.FindByPredicate(
      [actor](const FTATWeatherActorPollingState& pollingState)
            {
               return pollingState.TrackedActor == actor;      
            });
   if(existingState != nullptr)
      return;

   UE_LOG(LogTATWeatherSubsystem, Verbose, TEXT("Registered actor %s with the weather subsystem (polling = %s)"),
      *actor->GetName(), (enablePolling ? TEXT("true") : TEXT("false")));

   // Setup initial state
   if (enablePolling)
   {
      // For polling actors, pretend they were updated this frame and give them a random interval for their first actual update.
      // This prevents a situation at spawn where every single NPC is setting up initial state in a single frame.
      FTATWeatherActorPollingState& pollingState = _polledStates.AddDefaulted_GetRef();
      pollingState.TrackedActor = actor;
      pollingState.LastUpdateTime = GetWorld()->GetTimeSeconds();
      pollingState.NextUpdateInterval = _GetRandomPollingIntervalSeconds();

      // Set up their state with default indoors values so WeatherType can
      // be initially cached, indoor status will be updated next update
      _UpdateActorState(actor, false);
   }
   else
   {
      constexpr float minTraceDistance = 0.0f;
      const bool isInside = UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(
         this,
         actor->GetActorLocation(),
         minTraceDistance,
         actor
      );
      // For non-polling actors, just set up their state immediately
      _UpdateActorState(actor, isInside);
   }
}

void UTATWeatherSubsystem::UnregisterActor(AActor* actor)
{
   if (actor == nullptr)
   {
      return;
   }

   _polledStates.RemoveAll([actor](const FTATWeatherActorPollingState& pollingState)
   {
      return pollingState.TrackedActor == actor;
   });

   UE_LOG(LogTATWeatherSubsystem, Verbose, TEXT("Unregistered actor %s with the weather subsystem"), *actor->GetName());
}

void UTATWeatherSubsystem::_UpdateActorState(AActor* actor, const bool isInside) const
{
   check(actor != nullptr);
   const IAbilitySystemInterface* abilitySystemInterface = Cast<IAbilitySystemInterface>(actor);
   if(abilitySystemInterface == nullptr)
      return;

   UAbilitySystemComponent* asc = abilitySystemInterface->GetAbilitySystemComponent();
   if(asc == nullptr)
      return;

   if (isInside)
   {
      UE_VLOG(actor, LogTATWeatherSubsystem, Log, TEXT("Actor %s is now INSIDE"), *actor->GetName());
      _ChangeTagsOnActor(asc, _indoorTags, true);
      _ChangeTagsOnActor(asc, _outdoorTags, false);
   }
   else
   {
      UE_VLOG(actor, LogTATWeatherSubsystem, Log, TEXT("Actor %s is now OUTSIDE"), *actor->GetName());
      _ChangeTagsOnActor(asc, _indoorTags, false);
      _ChangeTagsOnActor(asc, _outdoorTags, true);
   }
}
