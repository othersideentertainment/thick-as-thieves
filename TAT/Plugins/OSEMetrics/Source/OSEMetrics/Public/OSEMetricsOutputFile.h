// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "OSEMetricsOutputBase.h"

// std
#include <atomic>
#include <mutex>

struct FOSEMetricsResult;
class FOSEMetricsSystem;

class OSEMETRICS_API FOSEMetricsOutputFile
   : public FOSEMetricsOutputBase
   , public TSharedFromThis<FOSEMetricsOutputFile, ESPMode::ThreadSafe>
{
   // config
   float _queryIntervalSeconds = 1.0f;
   float _flushIntervalSeconds = 5.0f;
   bool _useProfilingDir = false;
   FString _outputFilePath;

   // main thread data
   float timeSinceLastQuery = 0.0f;

   // file thread data
   std::atomic<bool> _threadRunning = false;
   TUniquePtr<FThread> _thread;
   mutable std::mutex _pendingOutputDataMutex;
   TArray<TSharedPtr<FJsonObject>> _pendingOutputData;

public:
   explicit FOSEMetricsOutputFile(float queryIntervalSeconds = 1.0f, float flushIntervalSeconds = 5.0f, bool useProfilingDir = true);
   virtual ~FOSEMetricsOutputFile();

   virtual bool Enable() override;
   virtual void Disable() override;

   virtual void Tick(float deltaSeconds) override;

   virtual void OnMetricGroupAdded(FName groupName) override;
   virtual void OnMetricAdded(FName groupName, FName metricName) override;

   virtual FString ToString() const override;

   void WriteMetrics(FOSEMetricsResult&& metricsResult);

private:
   void _StopThread();
   void _ThreadMain(const FString& filePath, float flushInterval);
};
