// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEMetricsUtils.h"

// ue
#include "Subsystems/WorldSubsystem.h"

#include "OSEGameSessionMetricsSubsystem.generated.h"

/// Metrics subsystem to collect stats over time (eg. min, max, avg) for later retrieval by metrics endpoints
UCLASS()
class OSEMETRICS_API UOSEGameSessionMetricsSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   /// Only enable this subsystem on server builds? Set to false to enable everywhere.
   static constexpr bool kOnlyEnableSubsystemOnServers = true;

   /// Initial time slice we compute the rolling average from.
   static constexpr double kDefaultStatsTimeSliceSeconds = 5.0;

   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;

   // from UWorldSubsystem
   virtual void PostInitialize() override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

public:
   void ResetGameDeltaSecondsStats(double rollingTimeSliceSeconds);
   void ResetRealDeltaSecondsStats(double rollingTimeSliceSeconds);

   FORCEINLINE OSEMetricsUtils::TRollingValueStats<float> GetGameDeltaSecondsStats(TOptional<double> currentTime = NullOpt, TOptional<double> timespanSeconds = NullOpt) const
   {
      return _gameDeltaSeconds.GetStats(currentTime, timespanSeconds);
   }

   FORCEINLINE OSEMetricsUtils::TRollingValueStats<float> GetRealDeltaSecondsStats(TOptional<double> currentTime = NullOpt, TOptional<double> timespanSeconds = NullOpt) const
   {
      return _realDeltaSeconds.GetStats(currentTime, timespanSeconds);
   }

private:
   bool _Tick(float deltaSeconds);

   void _EnableTick();
   void _DisableTick();

   FTSTicker::FDelegateHandle _tickHandle;

   /// Game delta seconds (affected by time dilation)
   OSEMetricsUtils::TRollingValue<float> _gameDeltaSeconds;

   /// Realtime delta seconds (NOT affected by time dilation)
   OSEMetricsUtils::TRollingValue<float> _realDeltaSeconds;

};
