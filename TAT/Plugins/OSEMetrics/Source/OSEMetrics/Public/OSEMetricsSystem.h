// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEMetricsUtils.h"

// ue
#include "CoreMinimal.h"
#include "Json.h"

class FOSEMetricsOutputBase;

struct FOSEMetricsResult
{
   enum class EStatus : uint8
   {
      Success = 0,
      Error,
      NotFound,
   };

   EStatus Status = EStatus::Success;
   TSharedPtr<FJsonObject> Data;
   FString ErrorMessage;

   static FOSEMetricsResult Make() { return FOSEMetricsResult{ EStatus::Success, MakeShared<FJsonObject>(), FString() }; }
   static FOSEMetricsResult MakeError(FString&& errorMessage) { return FOSEMetricsResult{ EStatus::Error, nullptr, MoveTemp(errorMessage) }; }
   static FOSEMetricsResult MakeNotFound(FName name) { return FOSEMetricsResult{ EStatus::NotFound, nullptr, name.ToString() }; }
   static FOSEMetricsResult MakeNotFound(FName groupName, FName metricName) { return FOSEMetricsResult{ EStatus::NotFound, nullptr,
      FString::Printf(TEXT("%s/%s"), *groupName.ToString(), *metricName.ToString())}; }

   bool IsValid() const { return Status == EStatus::Success && Data != nullptr; }
   explicit operator bool() const { return IsValid(); }

   bool ToJsonString(FString& outJsonString, bool prettyPrint = true) const;
};

class OSEMETRICS_API FOSEMetricsSystem : public TSharedFromThis<FOSEMetricsSystem>
{
   struct FMetricsGroup
   {
      TMap<FName, FOSEMetricsValueCallback> Metrics;
   };
   TMap<FName, FMetricsGroup> _metricsGroups;
   
   struct FOutput
   {
      bool Enabled = false;
      TSharedPtr<FOSEMetricsOutputBase> Output;
   };
   TArray<FOutput> _outputs;

   double _tickDelegateLastCallTime = 0.0;
   FTSTicker::FDelegateHandle _tickDelegateHandle;

public:
   FOSEMetricsSystem();
   ~FOSEMetricsSystem();

   void AddOutput(const TSharedPtr<FOSEMetricsOutputBase>& newOutput);

   void Enable();
   void Disable();

   void Tick(float deltaSeconds);

   bool AddMetricsGroup(FName groupName);
   bool AddMetric(FName groupName, FName metricName, FOSEMetricsValueCallback&& valueCallback);

   FOSEMetricsResult QueryAll(const TMap<FString, FString>& queryParams = {}) const;
   FOSEMetricsResult QueryGroup(FName groupName, const TMap<FString, FString>& queryParams = {}) const;
   FOSEMetricsResult Query(FName groupName, FName metricName, const TMap<FString, FString>& queryParams = {}) const;

private:
   bool _TickerFn(float gameWorldDeltaSeconds);
};
