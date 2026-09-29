// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGameSessionMetricsSubsystem.h"

#include "OSEMetrics.h"

static constexpr int32 kOSEFrameTimeMetricsMaxFramesPerSecond = 144;

void UOSEGameSessionMetricsSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      return;
   }

   ResetGameDeltaSecondsStats(kDefaultStatsTimeSliceSeconds);
   ResetRealDeltaSecondsStats(kDefaultStatsTimeSliceSeconds);

   _EnableTick();
}

void UOSEGameSessionMetricsSubsystem::Deinitialize()
{
   _DisableTick();
   Super::Deinitialize();
}

bool UOSEGameSessionMetricsSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   if constexpr (kOnlyEnableSubsystemOnServers)
   {
      return CastChecked<UWorld>(outer)->GetNetMode() < NM_Client;
   }
   else
   {
      return true;
   }
}

void UOSEGameSessionMetricsSubsystem::PostInitialize()
{
   Super::PostInitialize();
}

void UOSEGameSessionMetricsSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);
}

bool UOSEGameSessionMetricsSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE || worldType == EWorldType::GameRPC;
}

void UOSEGameSessionMetricsSubsystem::ResetGameDeltaSecondsStats(double rollingTimeSliceSeconds)
{
   _gameDeltaSeconds.Reset(rollingTimeSliceSeconds, kOSEFrameTimeMetricsMaxFramesPerSecond);
}

void UOSEGameSessionMetricsSubsystem::ResetRealDeltaSecondsStats(double rollingTimeSliceSeconds)
{
   _realDeltaSeconds.Reset(rollingTimeSliceSeconds, kOSEFrameTimeMetricsMaxFramesPerSecond);
}

bool UOSEGameSessionMetricsSubsystem::_Tick(float deltaSeconds)
{
   // Note that the deltaSeconds argument we get from TFSTicker is the time since the last game frame,
   // *not* since the last tick the delegate received.

   if (UWorld* world = GetWorld())
   {
      const double currentTime = FPlatformTime::Seconds();
      _gameDeltaSeconds.Add(world->DeltaTimeSeconds, currentTime);
      _realDeltaSeconds.Add(world->DeltaRealTimeSeconds, currentTime);
   }

   // TFSTicker delegate functions should always return true to indicate that we want to continue ticking
   return true;
}

void UOSEGameSessionMetricsSubsystem::_EnableTick()
{
   if (_tickHandle.IsValid())
   {
      return;
   }
   _tickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UOSEGameSessionMetricsSubsystem::_Tick));
}

void UOSEGameSessionMetricsSubsystem::_DisableTick()
{
   if (_tickHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_tickHandle);
      _tickHandle.Reset();
   }
}
