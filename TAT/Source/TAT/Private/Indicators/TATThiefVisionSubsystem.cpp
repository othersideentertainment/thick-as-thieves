// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATThiefVisionSubsystem.h"

// tat
#include "Indicators/TATClientProxyActorInterface.h"
#include "Indicators/TATClientProxyIndicatorConfig.h"
#include "Player/TATPlayerController.h"
#include "Developer/TATProjectSettings.h"
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherUtilities.h"

// ue
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefVisionSubsystem)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefVisionSubsystem, Log, All);

DECLARE_STATS_GROUP(TEXT("Thief Vision Subsystem"), STATGROUP_ThiefVision, STATCAT_Advanced)

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Active Local Client Proxy Actors (Total)"), STAT_ClientProxyTotalActorCount, STATGROUP_ThiefVision);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Active Local Client Proxy Actors (Visible)"), STAT_ClientProxyVisibleActorCount, STATGROUP_ThiefVision);
DECLARE_CYCLE_STAT(TEXT("Client Proxy Actor Spawner"), STAT_ClientProxyActorSpawner, STATGROUP_ThiefVision);
DECLARE_CYCLE_STAT(TEXT("Server Thief Vision Update"), STAT_ThiefVisionUpdate, STATGROUP_ThiefVision);
DECLARE_CYCLE_STAT(TEXT("Server Thief Vision Tick"), STAT_ThiefVisionTick, STATGROUP_ThiefVision);

// Minimum lifespan for a new thief vision indicator to be sent to a player.
static constexpr float kMinReplicatedThiefVisionIndicatorSecondsRemaining = 0.25f;

namespace ThiefVisionHelpers
{
   static FVector GetPlayerLocationChecked(APlayerController* pc)
   {
      check(pc != nullptr);

      APawn* pawn = pc->GetPawnOrSpectator();
      check(pawn != nullptr);

      FVector loc;
      FRotator rot;
      pawn->GetActorEyesViewPoint(loc, rot);
      return loc;
   }

   FORCEINLINE bool HasAuthority(const UWorld* world)
   {
      ensureMsgf(world != nullptr, TEXT("Thief vision subsystem got null world when checking authority"));
      return (world != nullptr && world->GetNetMode() < NM_Client) || IsRunningDedicatedServer();
   }
}

bool FTATThiefVisionIndicatorQuery::MatchesQuery(const FTATClientProxyInfo& clientProxyInfo) const
{
   if (FilterByIndicatorType)
   {
      if (IndicatorTypesMatchExact)
      {
         if (!IndicatorTypes.HasTagExact(clientProxyInfo.IndicatorType))
         {
            return false;
         }
      }
      else
      {
         if (!IndicatorTypes.HasTag(clientProxyInfo.IndicatorType))
         {
            return false;
         }
      }
   }

   if (FilterByInstigator && clientProxyInfo.Instigator != Instigator)
   {
      return false;
   }

   if (FilterByWorldLocation && FVector::DistSquared(WorldLocation, clientProxyInfo.Transform.Location) > FMath::Square(DistanceFromWorldLocation))
   {
      return false;
   }

   if (FilterByCustomData && clientProxyInfo.CustomData != CustomData)
   {
      return false;
   }

   return true;
}

void FTATThiefVisionIndicatorPrecomputedConfig::ApplyConfigLoadSynchronous(const FTATClientProxyIndicatorConfig& cfg, const FGameplayTag& weatherType)
{
   const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();

   IndicatorClass = cfg.IndicatorClass.LoadSynchronous();

   if (cfg.RequireThiefVision && tatSettings.ThiefVisionStatusTag.IsValid())
   {
      RequireGameplayTags.AddTag(tatSettings.ThiefVisionStatusTag);
   }

   if (cfg.RequireGameplayTags.Num() > 0)
   {
      RequireGameplayTags.AppendTags(cfg.RequireGameplayTags);
   }

   AutoRemoveOnInstigatorKnockout = cfg.AutoRemoveOnInstigatorKnockout;

   MinimumDeduplicateDistance = FMath::Max(0.0f, cfg.MinimumDeduplicateDistance);

   AllowRotation = cfg.AllowRotation;
   AllowScale = cfg.AllowScale;

   IndicatorLifeSpan = cfg.IndicatorLifeSpan;
   if (IndicatorLifeSpan <= 0)
   {
      UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Thief vision indicator '%s' does not have a lifespan configured and will never be spawned!"),
         *cfg.IndicatorId.ToString());
   }

   const float* outsideLifeSpan = weatherType.IsValid() ? cfg.IndicatorLifeSpanOutsideByWeatherType.Find(weatherType) : nullptr;
   IndicatorLifeSpanOutside = (outsideLifeSpan != nullptr) ? FMath::Max(0.0f, *outsideLifeSpan) : 0.0f;

   VisibleAtInfiniteRange = cfg.VisibleAtInfiniteRange;
   MaxVisibleRange = VisibleAtInfiniteRange ? 0.0f : FMath::Max(0.0f, cfg.MaxVisibleRange);

   // Populate the max visible range value here so we don't need to look up project settings every time we need it (for cache locality)
   if (!VisibleAtInfiniteRange && MaxVisibleRange <= 0)
   {
      MaxVisibleRange = tatSettings.DefaultClientProxyIndicatorMaxVisibleDistance;
   }

   // The max replicated range is the visible range plus hysteresis and some extra buffer room
   if (MaxVisibleRange > 0)
   {
      HysteresisRange = FMath::Max(0.0f, tatSettings.ClientProxyIndicatorHysteresisDistance);
      ReplicationBufferRange = FMath::Max(0.0f, tatSettings.ClientProxyIndicatorReplicationBufferDistance);
      MaxReplicatedRange = MaxVisibleRange + HysteresisRange;
   }
   else
   {
      MaxReplicatedRange = 0.0f;
   }
}

bool FTATThiefVisionIndicatorPrecomputedConfig::IsVisibleToPawn(APawn* pawn) const
{
   if (RequireGameplayTags.Num() > 0)
   {
      if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(pawn))
      {
         return tagInterface->HasAllMatchingGameplayTags(RequireGameplayTags);
      }
   }
   return true;
}

bool FTATThiefVisionIndicatorPrecomputedConfig::InReplicationRange(const FVector& indicatorLocation, const FVector& playerLocation, float currentlyReplicated) const
{
   if (VisibleAtInfiniteRange)
   {
      return true;
   }
   const float replicationRange = currentlyReplicated ? (MaxReplicatedRange + ReplicationBufferRange) : MaxReplicatedRange;
   return FVector::DistSquared(indicatorLocation, playerLocation) <= FMath::Square(replicationRange);
}

bool FTATThiefVisionIndicatorPrecomputedConfig::InVisibleRange(const FVector& indicatorLocation, const FVector& playerLocation, bool currentlyVisible) const
{
   if (VisibleAtInfiniteRange)
   {
      return true;
   }
   const float maxDistance = currentlyVisible ? (MaxVisibleRange + HysteresisRange) : MaxVisibleRange;
   return FVector::DistSquared(indicatorLocation, playerLocation) <= FMath::Square(maxDistance);
}

bool UTATThiefVisionSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   if (const UWorld* world = outer->GetWorld())
   {
      // Game world only
      if (world->IsGameWorld())
      {
         return true;
      }
   }

   return false;
}

void UTATThiefVisionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
   Super::OnWorldBeginPlay(InWorld);
   _hasAuthority = ThiefVisionHelpers::HasAuthority(&InWorld);

   // Cache the weather type so we can use it for weather-based indicator config
   bool isDefaultWeatherType = false;
   _weatherType = UTATWeatherSettings::GetCurrentWeatherType(this, isDefaultWeatherType);

   const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();

   // TODO: Async load the data table
   if (UDataTable* clientProxyIndicatorConfigs = tatSettings.ThiefVisionIndicatorsDataTable.LoadSynchronous())
   {
      clientProxyIndicatorConfigs->ForeachRow<FTATClientProxyIndicatorConfig>(TEXT("ThiefVisionClientProxyIndicatorConfig"),
         [this](const FName& key, const FTATClientProxyIndicatorConfig& indicator)
         {
            const FGameplayTag indicatorId = indicator.IndicatorId;

            // Don't even bother setting up disabled indicators since we will never show them
            if (!indicator.IsEnabled)
            {
               this->_disabledIndicators.Add(indicatorId);
               return;
            }

            if (this->_indicatorConfig.Contains(indicatorId) || indicatorId == FGameplayTag::EmptyTag)
            {
               UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Skipping duplicate or empty thief vision indicator type '%s'"),
                  *indicatorId.ToString());
               return;
            }

            if (indicator.IndicatorClass.IsNull())
            {
               UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Skipping thief vision indicator type '%s': it has a null indicator class"),
                  *indicatorId.ToString());
               return;
            }

            FTATThiefVisionIndicatorPrecomputedConfig& newCfg = this->_indicatorConfig.Add(indicatorId);
            if (indicator.IndicatorClass.IsValid())
            {
               // The indicator actor class is already loaded, so we can init the config without needing an async-load first
               newCfg.ApplyConfigLoadSynchronous(indicator, _weatherType);
            }
            else
            {
               // The indicator actor class is not loaded, so async-load it and then init the config
               TWeakObjectPtr<UTATThiefVisionSubsystem> weakThis = this;
               // NB. We're _copying_ indicatorConfig into this lambda. It's not ideal, but it's a small struct and this only happens in initialize.
               UAssetManager::GetStreamableManager().RequestAsyncLoad(indicator.IndicatorClass.ToSoftObjectPath(),
                  [weakThis, indicator]()
                  {
                     if (!weakThis.IsValid())
                     {
                        return;
                     }
                     FTATThiefVisionIndicatorPrecomputedConfig* cfg = weakThis->_indicatorConfig.Find(indicator.IndicatorId);
                     check(cfg != nullptr);
                     cfg->ApplyConfigLoadSynchronous(indicator, weakThis->_weatherType);
                  });
            }
         });
   }
   else
   {
      UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Could not load thief vision indicator configs data table from project settings"));
   }
}

void UTATThiefVisionSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATThiefVisionSubsystem::Deinitialize()
{
   Super::Deinitialize();
}

ETickableTickType UTATThiefVisionSubsystem::GetTickableTickType() const
{
   if (Super::GetTickableTickType() == ETickableTickType::Never)
   {
      return ETickableTickType::Never;
   }

   // Only tick on the server.
   // Note that we can't rely on the cached _hasAuthority value here because this can be called before OnWorldBeginPlay
   return ThiefVisionHelpers::HasAuthority(GetWorld()) ? ETickableTickType::Always : ETickableTickType::Never;
}

void UTATThiefVisionSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   SCOPE_CYCLE_COUNTER(STAT_ThiefVisionTick);

   // Remove all expired thief vision glyphs.
   if (_hasAuthority && _authorityIndicators.Num() > 0)
   {
      const float currentTime = GetWorld()->GetTimeSeconds();
      for (auto iter = _authorityIndicators.CreateIterator(); iter; ++iter)
      {
         const FTATClientProxyInfo& info = iter->Value;
         const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
         if (cfg == nullptr || !info.IsActive(cfg->GetIndicatorLifeSpan(info.IsOutside), currentTime))
         {
            iter.RemoveCurrent();
         }
      }
   }
}

bool UTATThiefVisionSubsystem::AuthoritySpawnThiefVisionIndicator(FGameplayTag indicatorType, const FTransform& transform, float deduplicateDistance, AActor* instigator, uint8 customData)
{
   if (GetWorld()->IsNetMode(NM_Client))
   {
      UE_LOG(LogTATThiefVisionSubsystem, Warning, TEXT("AuthoritySpawnThiefVisionIndicator() called on non-authority client!"));
      return false;
   }

   // Silently fail when trying to spawn a disabled indicator
   if (_disabledIndicators.Contains(indicatorType))
   {
      return false;
   }

   if (!indicatorType.IsValid() || !_indicatorConfig.Contains(indicatorType))
   {
      UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("AuthoritySpawnThiefVisionIndicator() called with invalid indicatorType '%s'"),
         *indicatorType.ToString());
      return false;
   }

   // Avoid spawning an indicator too close to another indicator with the same actor class
   deduplicateDistance = FMath::Max(deduplicateDistance, _indicatorConfig[indicatorType].MinimumDeduplicateDistance);
   if (deduplicateDistance > 0)
   {
      if (FTATClientProxyInfo* existingIndicator = _AuthorityFindThiefVisionIndicatorClosestToLocation(indicatorType, transform.GetLocation(), deduplicateDistance))
      {
         // Refresh the existing indicator
         existingIndicator->LastRefreshServerWorldTime = GetWorld()->GetTimeSeconds();
         return true;
      }
   }

   // Add a new indicator
   const int32 uniqueIndicatorId = _AuthorityGenerateIndicatorUniqueId();
   _authorityIndicators.Add(uniqueIndicatorId, _AuthorityConstructNewThiefVisionIndicator(uniqueIndicatorId, indicatorType, transform, instigator, customData));
   return true;
}

int32 UTATThiefVisionSubsystem::AuthorityRemoveThiefVisionIndicatorsMatchingQuery(const FTATThiefVisionIndicatorQuery& query)
{
   int32 numRemoved = 0;
   for (auto it = _authorityIndicators.CreateIterator(); it; ++it)
   {
      const FTATClientProxyInfo& clientProxyInfo = it->Value;
      if (query.MatchesQuery(clientProxyInfo))
      {
         it.RemoveCurrent();
         ++numRemoved;
      }
   }
   return numRemoved;
}

bool UTATThiefVisionSubsystem::AuthorityUpdateThiefVisionIndicatorsForPlayer(APlayerController* pc, FTATClientProxyInfoArray& indicators)
{
   SCOPE_CYCLE_COUNTER(STAT_ThiefVisionUpdate);

   check(_hasAuthority);
   check(pc != nullptr && pc->HasAuthority());

   APawn* playerPawn = pc->GetPawnOrSpectator();
   if (playerPawn == nullptr)
   {
      return false;
   }

   const float currentWorldTime = GetWorld()->GetTimeSeconds();
   const FVector playerLocation = playerPawn->GetActorLocation();
   const bool isListenServer = pc->IsLocalController();

   auto localUpdateOrSpawnProxyActor = [this, pc, isListenServer, currentWorldTime](const FTATClientProxyInfo& serverInfo, const FTATClientProxyInfo* existingClientInfo)
   {
      check(isListenServer);
      if (FClientProxyActorState* proxyState = _clientProxyActors.Find(serverInfo.UniqueId))
      {
         if (existingClientInfo != nullptr && existingClientInfo->ShouldUpdate(serverInfo) && proxyState->Actor.IsValid())
         {
            _ClientUpdateProxyActor(pc, serverInfo, currentWorldTime, *proxyState);
         }
      }
      else
      {
         FClientProxyActorState newProxy;
         if(_ClientSpawnProxyActor(pc, serverInfo, currentWorldTime, newProxy))
         {
            _clientProxyActors.Add(serverInfo.UniqueId, newProxy);
         }
      }
   };

   auto localDestroyProxyActor = [this, isListenServer](int32 indicatorUniqueId)
   {
      check(isListenServer);
      if (FClientProxyActorState* proxyState = _clientProxyActors.Find(indicatorUniqueId))
      {
         _ClientDestroyProxyActor(*proxyState);
         _clientProxyActors.Remove(indicatorUniqueId);
      }
   };

   auto shouldReplicate = [this, playerPawn, playerLocation, currentWorldTime](const FTATClientProxyInfo& info, bool currentlyReplicated)
   {
      const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
      if (cfg == nullptr)
      {
         return false;
      }

      if (!cfg->IsVisibleToPawn(playerPawn))
      {
         return false;
      }

      // Ignore any indicators that are just about to be destroyed
      if (!currentlyReplicated && info.GetRemainingLifeSpan(cfg->GetIndicatorLifeSpan(info.IsOutside), currentWorldTime) < kMinReplicatedThiefVisionIndicatorSecondsRemaining)
      {
         return false;
      }

      // Indicator is out of range for this player
      if (!cfg->InReplicationRange(info.Transform.Location, playerLocation, currentlyReplicated))
      {
         return false;
      }

      return true;
   };

   int32 numAddedIndicators = 0;
   int32 numUpdatedIndicators = 0;
   int32 numRemovedIndicators = 0;

   _authorityProcessedIndicators.Empty(indicators.Items.Num());

   for (auto it = indicators.Items.CreateIterator(); it; ++it)
   {
      FTATClientProxyInfo& info = *it;
      const FTATClientProxyInfo* authInfo = _authorityIndicators.Find(info.UniqueId);

      // Remove indicators that no longer exist or aren't relevant for player
      if (!authInfo || !shouldReplicate(*authInfo, true))
      {
         if (isListenServer)
         {
            localDestroyProxyActor(info.UniqueId);
         }

         it.RemoveCurrentSwap();
         indicators.MarkArrayDirty();

         numRemovedIndicators += 1;
      }
      // Update indicators that have changed
      else if (info.ShouldUpdate(*authInfo))
      {
         ensureMsgf(
            info.IndicatorType == authInfo->IndicatorType,
            TEXT("Thief vision indicator (%d) has mismatched type (%s != %s)"),
            info.UniqueId, *info.IndicatorType.ToString(), *authInfo->IndicatorType.ToString()
         );

         if (isListenServer)
         {
            localUpdateOrSpawnProxyActor(*authInfo, &info);
         }

         info.LastRefreshServerWorldTime = authInfo->LastRefreshServerWorldTime;
         indicators.MarkItemDirty(info);

         numUpdatedIndicators += 1;
      }

      _authorityProcessedIndicators.Add(info.UniqueId);
   }

   for (const auto& authInfoPair : _authorityIndicators)
   {
      // Add indicators that are newly relevant for player
      if (!_authorityProcessedIndicators.Contains(authInfoPair.Key) && shouldReplicate(authInfoPair.Value, false))
      {
         FTATClientProxyInfo& info = indicators.Items.Add_GetRef(authInfoPair.Value);
         indicators.MarkItemDirty(info);

         if (isListenServer)
         {
            localUpdateOrSpawnProxyActor(info, nullptr);
         }

         numAddedIndicators += 1;
      }
   }

   UE_CLOG(numAddedIndicators > 0, LogTATThiefVisionSubsystem, Verbose, TEXT("Added %d indicators (%d total)"), numAddedIndicators, indicators.Items.Num());
   UE_CLOG(numUpdatedIndicators > 0, LogTATThiefVisionSubsystem, Verbose, TEXT("Updated %d indicators (%d total)"), numUpdatedIndicators, indicators.Items.Num());
   UE_CLOG(numRemovedIndicators > 0, LogTATThiefVisionSubsystem, Verbose, TEXT("Removed %d indicators (%d total)"), numRemovedIndicators, indicators.Items.Num());

   return numAddedIndicators != 0 || numUpdatedIndicators != 0 || numRemovedIndicators != 0;
}

int32 UTATThiefVisionSubsystem::_AuthorityGenerateIndicatorUniqueId()
{
   check(GetWorld()->GetNetMode() != NM_Client);
   const int32 newId = _authorityNextIndicatorUniqueId;
   check(newId >= 0 && static_cast<int64>(newId) < static_cast<int64>(std::numeric_limits<int32>::max()));
   ++_authorityNextIndicatorUniqueId;
   return newId;
}

FTATClientProxyInfo UTATThiefVisionSubsystem::_AuthorityConstructNewThiefVisionIndicator(int32 uniqueId, const FGameplayTag& indicatorType, const FTransform& transform, AActor* instigator, uint8 customData)
{
   FTATClientProxyInfo info;
   check(GetWorld()->GetNetMode() != NM_Client);
   const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(indicatorType);
   check(cfg != nullptr);
   info.UniqueId = uniqueId;
   info.IndicatorType = indicatorType;
   info.LastRefreshServerWorldTime = GetWorld()->GetTimeSeconds();
   info.Transform.Set(transform, cfg->AllowRotation, cfg->AllowScale);
   info.Instigator = instigator;
   info.IsOutside = cfg->RequiresOutsideCheck() ? !UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(this, transform.GetLocation() + (FVector::UpVector * 2.0f)) : false;
   info.CustomData = customData;
   return info;
}

void UTATThiefVisionSubsystem::ClientSyncThiefVisionIndicatorVisibility(APlayerController* pc)
{
   check(pc != nullptr);
   check(pc->IsLocalController());

   SET_DWORD_STAT(STAT_ClientProxyTotalActorCount, _clientProxyActors.Num());
   int32 numVisibleProxyActors = 0;

   APawn* playerPawn = pc->GetPawn();
   if (playerPawn == nullptr)
   {
      return;
   }

   const FVector playerLocation = ThiefVisionHelpers::GetPlayerLocationChecked(pc);

   for (auto& pair : _clientProxyActors)
   {
      FClientProxyActorState& proxyState = pair.Value;
      if (const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(proxyState.IndicatorType))
      {
         if (AActor* proxyActor = proxyState.Actor.Get())
         {
            const bool shouldBeVisible = cfg->InVisibleRange(proxyActor->GetActorLocation(), playerLocation, proxyState.Visible) && cfg->IsVisibleToPawn(playerPawn);
            _ClientSetProxyActorVisibility(proxyState, shouldBeVisible);
            ++numVisibleProxyActors;
         }
      }
   }

   SET_DWORD_STAT(STAT_ClientProxyVisibleActorCount, numVisibleProxyActors);
}

void UTATThiefVisionSubsystem::ClientGetHiddenThiefVisionComponents(APlayerController* pc, const FVector& viewLocation, TSet<FPrimitiveComponentId>& outHiddenComponents)
{
   check(pc != nullptr);
   ensure(pc->IsLocalPlayerController());

   const float currentGameTime = GetWorld()->GetTimeSeconds();
   for (const auto& pair : _registeredComponents)
   {
      UPrimitiveComponent* comp = pair.Key.Get();
      if (comp != nullptr && pair.Value.HiddenAfterGameTime > 0 && currentGameTime >= pair.Value.HiddenAfterGameTime)
      {
         outHiddenComponents.Add(comp->GetPrimitiveSceneId());
      }
   }
}

void UTATThiefVisionSubsystem::NotifyThiefVisionStatusChanged(APlayerController* pc, bool newThiefVisionEnabled)
{
   if (pc == nullptr)
   {
      return;
   }

   OnThiefVisionStatusChanged.Broadcast(pc, newThiefVisionEnabled);

   if (pc->IsLocalController())
   {
      // cache the enabled state for the local player
      _localPlayerThiefVisionEnabled = newThiefVisionEnabled;

      OnLocalPlayerThiefVisionStatusChanged.Broadcast(pc, newThiefVisionEnabled);

      // Update visibility state of all registered components
      if (newThiefVisionEnabled)
      {
         for (auto& pair : _registeredComponents)
         {
            pair.Value.HiddenAfterGameTime = 0.0f;
         }
      }
      else
      {
         const float currentGameTime = GetWorld()->GetTimeSeconds();
         for (auto& pair : _registeredComponents)
         {
            pair.Value.HiddenAfterGameTime = currentGameTime + pair.Value.DelayTimeBeforeHide;
         }
      }
   }
}

void UTATThiefVisionSubsystem::FindThiefVisionIndicatorTypes(FGameplayTagContainer& outIndicatorTypes, TFunctionRef<bool(FGameplayTag, const FTATThiefVisionIndicatorPrecomputedConfig&)> callback) const
{
   for (const auto& pair : _indicatorConfig)
   {
      if (callback(pair.Key, pair.Value))
      {
         outIndicatorTypes.AddTag(pair.Key);
      }
   }
}

// static
bool UTATThiefVisionSubsystem::IsThiefVisionEnabled(APawn* pawn)
{
   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(pawn))
   {
      return tagInterface->HasMatchingGameplayTag(UTATProjectSettings::Get().ThiefVisionStatusTag);
   }
   return false;
}

void UTATThiefVisionSubsystem::RegisterThiefVisionLinkedComponent(UPrimitiveComponent* component, float delayTimeBeforeHide)
{
   if (component == nullptr || GetWorld()->GetNetMode() == NM_DedicatedServer)
   {
      return;
   }

   FThiefVisionComponentState& state = _registeredComponents.FindOrAdd(component);
   state.DelayTimeBeforeHide = FMath::Max(0.0f, delayTimeBeforeHide);
   // If thief vision is enabled, assume this actor is being spawned and hide the component immediately with no delay time
   state.HiddenAfterGameTime = _localPlayerThiefVisionEnabled ? 0.0f : GetWorld()->GetTimeSeconds();
}

void UTATThiefVisionSubsystem::UnregisterThiefVisionLinkedComponent(UPrimitiveComponent* component)
{
   if (component == nullptr || GetWorld()->GetNetMode() == NM_DedicatedServer)
   {
      return;
   }

   // Take this opportunity to also remove all components that no longer have valid pointers.
   for (auto iter = _registeredComponents.CreateIterator(); iter; ++iter)
   {
      UPrimitiveComponent* compKey = iter.Key().Get();
      if (compKey == nullptr || compKey == component)
      {
         iter.RemoveCurrent();
      }
   }
}

FTATClientProxyInfo* UTATThiefVisionSubsystem::_AuthorityFindThiefVisionIndicatorClosestToLocation(FGameplayTag indicatorType, const FVector& worldLocation, float maxDistance)
{
   check(_hasAuthority);
   const float maxDistSquared = FMath::Square(maxDistance);
   FTATClientProxyInfo* closestIndicatorInfo = nullptr;
   float closestIndicatorDistSquared = FLT_MAX;
   const bool requireIndicatorType = indicatorType.IsValid();
   const float currentWorldTime = GetWorld()->GetTimeSeconds();
   for (auto& pair : _authorityIndicators)
   {
       FTATClientProxyInfo& info = pair.Value;

      // Ignore any indicators that don't match the required type
      if (requireIndicatorType && info.IndicatorType != indicatorType)
      {
         continue;
      }

      const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
      if (cfg == nullptr)
      {
         continue;
      }

      // Ignore any indicators that are about to be destroyed
      if (info.GetRemainingLifeSpan(cfg->GetIndicatorLifeSpan(info.IsOutside), currentWorldTime) <= kMinReplicatedThiefVisionIndicatorSecondsRemaining)
      {
         continue;
      }

      const float indicatorDistSquared = FVector::DistSquared(worldLocation, info.Transform.Location);

      // Ignore any indicators that are too far away from the search location
      if (maxDistance > 0 && indicatorDistSquared > maxDistSquared)
      {
         continue;
      }

      if (indicatorDistSquared < closestIndicatorDistSquared)
      {
         closestIndicatorInfo = &info;
         closestIndicatorDistSquared = indicatorDistSquared;
      }
   }

   return closestIndicatorInfo;
}

void UTATThiefVisionSubsystem::_ClientOnPreReplicatedRemove(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& removedIndices, int32 finalSize)
{
   for (int32 idx : removedIndices)
   {
      check(indicators.IsValidIndex(idx));
      const FTATClientProxyInfo& info = indicators[idx];

      UE_LOG(LogTATThiefVisionSubsystem, Verbose, TEXT("(fast array) REMOVE: id %d (%s)"), info.UniqueId, *info.IndicatorType.ToString());

      FClientProxyActorState* existingState = _clientProxyActors.Find(info.UniqueId);
      if (existingState != nullptr)
      {
         _ClientDestroyProxyActor(*existingState);
         _clientProxyActors.Remove(info.UniqueId);
      }
      else
      {
         UE_LOG(LogTATThiefVisionSubsystem, Warning, TEXT("Failed to remove thief vision indicator with id %d and type '%s': no such indicator was found"),
            info.UniqueId, *info.IndicatorType.ToString());
      }
   }
}

void UTATThiefVisionSubsystem::_ClientOnPostReplicatedAdd(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& addedIndices, int32 finalSize)
{
   AGameStateBase* gameState = GetWorld()->GetGameState();
   if (gameState == nullptr)
   {
      return;
   }
   const float currentServerWorldTime = gameState->GetServerWorldTimeSeconds();

   for (int32 idx : addedIndices)
   {
      check(indicators.IsValidIndex(idx));
      const FTATClientProxyInfo& info = indicators[idx];

      UE_LOG(LogTATThiefVisionSubsystem, Verbose, TEXT("(fast array) ADD: id %d (%s)"), info.UniqueId, *info.IndicatorType.ToString());

      const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
      if (cfg == nullptr)
      {
         UE_LOG(LogTATThiefVisionSubsystem, Warning, TEXT("Failed to spawn thief vision indicator with id %d: invalid indicator type '%s'"),
            info.UniqueId, *info.IndicatorType.ToString());
         continue;
      }

      FClientProxyActorState& proxyState = _clientProxyActors.FindOrAdd(info.UniqueId);

      // This shouldn't happen, but just to be safe, clean up any existing proxy actor with this id
      if (proxyState.IndicatorType.IsValid() && proxyState.Actor.IsValid())
      {
         UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Expected new thief vision indicator for id=%d, type='%s', but we have an existing one already!"),
            info.UniqueId, *info.IndicatorType.ToString());
         _ClientDestroyProxyActor(proxyState);
         proxyState = FClientProxyActorState();
      }

      _ClientSpawnProxyActor(pc, info, currentServerWorldTime, proxyState);
   }
}

void UTATThiefVisionSubsystem::_ClientOnPostReplicatedChange(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& changedIndices, int32 finalSize)
{
   AGameStateBase* gameState = GetWorld()->GetGameState();
   if (gameState == nullptr)
   {
      return;
   }
   const float currentServerWorldTime = gameState->GetServerWorldTimeSeconds();

   for (int32 idx : changedIndices)
   {
      check(indicators.IsValidIndex(idx));
      const FTATClientProxyInfo& info = indicators[idx];

      UE_LOG(LogTATThiefVisionSubsystem, Verbose, TEXT("(fast array) CHANGE: id %d (%s) %.2f"), info.UniqueId, *info.IndicatorType.ToString(), indicators[idx].LastRefreshServerWorldTime);

      const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
      if (cfg == nullptr)
      {
         UE_LOG(LogTATThiefVisionSubsystem, Warning, TEXT("Failed to update thief vision indicator with id %d: invalid indicator type '%s'"),
            info.UniqueId, *info.IndicatorType.ToString());
         continue;
      }

      FClientProxyActorState* existingProxy = _clientProxyActors.Find(info.UniqueId);
      if (existingProxy != nullptr && existingProxy->Actor.IsValid())
      {
         _ClientUpdateProxyActor(pc, info, currentServerWorldTime, *existingProxy);
      }
      else
      {
         // No existing proxy actor, or its lifetime expired - spawn a new one
         FClientProxyActorState newProxy;
         if (_ClientSpawnProxyActor(pc, info, currentServerWorldTime, newProxy))
         {
            _clientProxyActors.FindOrAdd(info.UniqueId) = newProxy;
         }
      }
   }
}

void UTATThiefVisionSubsystem::_ClientUpdateProxyActor(APlayerController* pc, const FTATClientProxyInfo& info, float currentServerWorldTime, FClientProxyActorState& existingProxyState)
{
   check(existingProxyState.IndicatorType == info.IndicatorType);

   AActor* proxyActor = existingProxyState.Actor.Get();
   check(proxyActor != nullptr);

   const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(info.IndicatorType);
   check(cfg != nullptr);
   check(cfg->IndicatorClass && proxyActor->IsA(cfg->IndicatorClass));

   const float remainingLifeSpan = info.GetRemainingLifeSpan(cfg->GetIndicatorLifeSpan(info.IsOutside), currentServerWorldTime);

   // Update the existing indicator if we can, otherwise spawn a new one to replace it
   if (remainingLifeSpan > 0)
   {
      if (proxyActor->Implements<UTATClientProxyActorInterface>())
      {
         ITATClientProxyActorInterface::Execute_OnClientProxyActorLifeSpanRefreshed(proxyActor, remainingLifeSpan);
      }
   }
   else
   {
      _ClientDestroyProxyActor(existingProxyState);
      _clientProxyActors.Remove(info.UniqueId);
   }
}

namespace UE::Net
{
   // Bit of a hack - this lets us access private fields in AActor, which we need to disable replication before finalizing actor spawn.
   // This works because AActor has "friend class UE::Net::FTearOffSetter", but only actually defines FTearOffSetter in a cpp file (DataChannel.cpp).
   //
   // NB. If this hack ever breaks due to changes in AActor or DataChannel, we can trivially fix this by adding our own friend class declaration in AActor.
   // If you do that, don't forget to rename this class and change the namespace to something more specific (eg. TAT::FReplicatesSetter).
   class FTearOffSetter
   {
      FTearOffSetter() = delete;
   public:
      static void SetReplicates(AActor* actor, bool newReplicates)
      {
         // Just copy the behavior of AActor::SetReplicates (minus the post-init and logging parts)
         check(!actor->bActorInitialized);
         actor->RemoteRole = newReplicates ? ROLE_SimulatedProxy : ROLE_None;
         actor->bReplicates = newReplicates;
      }
   };
}

bool UTATThiefVisionSubsystem::_ClientSpawnProxyActor(APlayerController* pc, const FTATClientProxyInfo& indicatorInfo, float currentServerWorldTime, FClientProxyActorState& outProxyState) const
{
   SCOPE_CYCLE_COUNTER(STAT_ClientProxyActorSpawner);

   check(pc != nullptr);
   check(pc->IsLocalController());
   UWorld* world = GetWorld();
   check(world != nullptr);
   check(world->GetNetMode() != NM_DedicatedServer);

   outProxyState.Actor.Reset();
   outProxyState.Visible = false;
   outProxyState.IndicatorType = indicatorInfo.IndicatorType;

   const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(indicatorInfo.IndicatorType);
   check(cfg != nullptr);

   const float remainingLifeSpan = indicatorInfo.GetRemainingLifeSpan(cfg->GetIndicatorLifeSpan(indicatorInfo.IsOutside), currentServerWorldTime);
   if (remainingLifeSpan <= 0.25f)
   {
      // Don't bother spawning an actor that's only going to last a small fraction of a second.
      return false;
   }

   check(cfg->IndicatorClass != nullptr);

   // Pass the indicator's instigator as the actor's instigator if it happens to be a pawn.
   AActor* newActor = world->SpawnActorDeferred<AActor>(cfg->IndicatorClass, indicatorInfo.Transform.Get(), pc, Cast<APawn>(indicatorInfo.Instigator));
   if (newActor == nullptr)
   {
      UE_LOG(LogTATThiefVisionSubsystem, Error, TEXT("Can't spawn client proxy actor of type '%s': SpawnActor failed"), *indicatorInfo.IndicatorType.ToString());
      return false;
   }

   outProxyState.Actor = newActor;

   // We need to make sure we're not spawning this actor with replication enabled. Spawning a replicated actor on a client is harmless, but spawning it on
   // a listen server will cause it to replicate to other clients and we don't want that.
   // We also want to disable replication _before_ we finish spawning the actor so that we're not doing an initial wasted replication before turning it off.
   // Ideally we would just set newActor->bReplicates to false, but that's a protected field. The next best option is calling newActor->SetReplicates(false),
   // which will do exactly what we want and does work correctly, but it will also log a warning saying that you should actually just set the bReplicates field
   // directly instead of calling SetReplicates, which we can't do... because bReplicates is a protected field. Thanks Unreal.
   if (newActor->GetIsReplicated())
   {
      // Take advantage of AActor having a friend class declaration called `UE::Net::FTearOffSetter` without ever defining that class in any header files
      // to make our own private setter helper class for AActor fields.
      UE::Net::FTearOffSetter::SetReplicates(newActor, false);
   }

   // Fire an event just _before_ BeginPlay so actors can detect if they've been spawned as a client proxy
   if (newActor->Implements<UTATClientProxyActorInterface>())
   {
      FTATClientProxySpawnParams spawnParams{};
      spawnParams.Instigator = indicatorInfo.Instigator;
      spawnParams.CustomData = indicatorInfo.CustomData;
      ITATClientProxyActorInterface::Execute_OnSpawnedAsClientProxy(newActor, remainingLifeSpan, cfg->GetIndicatorLifeSpan(indicatorInfo.IsOutside), spawnParams);
   }

   newActor->FinishSpawning(indicatorInfo.Transform.Get());

   // Post-spawn setup

   // Setup initial visibility
   bool startVisible = true;
   if (APawn* playerPawn = pc->GetPawnOrSpectator())
   {
      constexpr bool currentlyVisible = false;
      if (!cfg->InVisibleRange(indicatorInfo.Transform.Location, ThiefVisionHelpers::GetPlayerLocationChecked(pc), currentlyVisible)
         || !cfg->IsVisibleToPawn(playerPawn))
      {
         startVisible = false;
      }
   }
   constexpr bool forceSet = true;
   _ClientSetProxyActorVisibility(outProxyState, startVisible, forceSet);

   return true;
}

void UTATThiefVisionSubsystem::_ClientDestroyProxyActor(FClientProxyActorState& proxyState) const
{
   AActor* proxyActor = proxyState.Actor.Get();
   if (!IsValid(proxyActor))
   {
      return;
   }

   // If the proxy actor implements the proxy actor interface, we can let it know it's about to go away so it can play transition VFX.
   if (proxyActor->Implements<UTATClientProxyActorInterface>())
   {
      ITATClientProxyActorInterface::Execute_OnDestroyClientProxy(proxyActor);
   }
   else
   {
      proxyActor->Destroy();
   }
}

void UTATThiefVisionSubsystem::_ClientSetProxyActorVisibility(FClientProxyActorState& proxyState, bool newVisible, bool force) const
{
   if (!force && proxyState.Visible == newVisible)
   {
      return;
   }

   proxyState.Visible = newVisible;

   AActor* proxyActor = proxyState.Actor.Get();
   if (proxyActor == nullptr)
   {
      return;
   }

   if (proxyActor->Implements<UTATClientProxyActorInterface>())
   {
      // If the actor implements the client proxy interface, let it handle visibility
      ITATClientProxyActorInterface::Execute_OnClientProxyActorSetVisible(proxyActor, newVisible);
   }
   else
   {
      proxyActor->SetActorHiddenInGame(!newVisible);
   }
}

APlayerController* UTATThiefVisionSubsystem::_GetLocalPlayerController() const
{
   if (GetWorld()->GetNetMode() != NM_DedicatedServer)
   {
      for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
      {
         APlayerController* pc = it->Get();
         if (pc != nullptr && pc->IsLocalPlayerController())
         {
            return pc;
         }
      }
   }
   return nullptr;
}
