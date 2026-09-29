// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetricsSystem.h"

// ose
#include "OSEMetrics.h"
#include "OSEMetricsOutputBase.h"

static inline TSharedPtr<FJsonValueObject> MakeJsonValueObjectSafe(TSharedPtr<FJsonObject> jsonObject)
{
   return jsonObject.IsValid()
      ? MakeShared<FJsonValueObject>(jsonObject)
      : MakeShared<FJsonValueObject>(MakeShared<FJsonObject>());
}

bool FOSEMetricsResult::ToJsonString(FString& outJsonString, bool prettyPrint) const
{
   if (!Data)
   {
      outJsonString = TEXT("{}");
      return false;
   }
   const bool success = prettyPrint
      ? FJsonSerializer::Serialize(Data.ToSharedRef(), TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&outJsonString))
      : FJsonSerializer::Serialize(Data.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&outJsonString));
   if (!success)
   {
      outJsonString = TEXT("{}");
      return false;
   }
   return true;
}

FOSEMetricsSystem::FOSEMetricsSystem()
{
}

FOSEMetricsSystem::~FOSEMetricsSystem()
{
   Disable();
   _outputs.Reset();
}

void FOSEMetricsSystem::AddOutput(const TSharedPtr<FOSEMetricsOutputBase>& newOutput)
{
   if (!newOutput)
   {
      return;
   }
   
   if (!ensure(!newOutput->_weakSystem.IsValid()))
   {
      return;
   }

   // Give the output a reference to ourselves so it can query metrics data
   newOutput->_weakSystem = AsWeak();

   _outputs.Add(FOutput{
      .Enabled = false,
      .Output = newOutput,
   });

   // Call OnAdded methods for existing metrics so the output can configure itself
   for (const auto& groupPair : _metricsGroups)
   {
      newOutput->OnMetricGroupAdded(groupPair.Key);
      for (const auto& metricPair : groupPair.Value.Metrics)
      {
         newOutput->OnMetricAdded(groupPair.Key, metricPair.Key);
      }
   }
}

void FOSEMetricsSystem::Enable()
{
   _tickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FOSEMetricsSystem::_TickerFn));

   for (FOutput& output : _outputs)
   {
      if (output.Enabled || !output.Output)
      {
         continue;
      }
      if (output.Output->Enable())
      {
         output.Enabled = true;
      }
      else
      {
         UE_LOG(LogOSEMetrics, Error, TEXT("Failed to enable metrics output %s"), *output.Output->ToString());
      }
   }
}

void FOSEMetricsSystem::Disable()
{
   for (FOutput& output : _outputs)
   {
      if (!output.Enabled)
      {
         continue;
      }
      if (output.Output)
      {
         output.Output->Disable();
      }
      output.Enabled = false;
   }

   if (_tickDelegateHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_tickDelegateHandle);
      _tickDelegateHandle.Reset();
   }
}

void FOSEMetricsSystem::Tick(float deltaSeconds)
{
   for (const FOutput& output : _outputs)
   {
      if (output.Enabled && output.Output)
      {
         output.Output->Tick(deltaSeconds);
      }
   }
}

bool FOSEMetricsSystem::AddMetricsGroup(FName groupName)
{
   if (_metricsGroups.Contains(groupName))
   {
      return false;
   }
   _metricsGroups.Add(groupName);

   // inform outputs about the new group
   for (const FOutput& output : _outputs)
   {
      if (output.Output)
      {
         output.Output->OnMetricGroupAdded(groupName);
      }
   }

   return true;
}

bool FOSEMetricsSystem::AddMetric(FName groupName, FName metricName, FOSEMetricsValueCallback&& valueCallback)
{
   FMetricsGroup* group = _metricsGroups.Find(groupName);
   if (group == nullptr || group->Metrics.Contains(metricName))
   {
      return false;
   }
   group->Metrics.Add(metricName, MoveTemp(valueCallback));

   // inform outputs about the new metric
   for (const FOutput& output : _outputs)
   {
      if (output.Output)
      {
         output.Output->OnMetricAdded(groupName, metricName);
      }
   }

   return true;
}

FOSEMetricsResult FOSEMetricsSystem::QueryAll(const TMap<FString, FString>& queryParams) const
{
   FOSEMetricsResult result = FOSEMetricsResult::Make();
   for (const auto& groupPair : _metricsGroups)
   {
      TSharedPtr<FJsonObject> groupData = MakeShared<FJsonObject>();
      for (const auto& metricPair : groupPair.Value.Metrics)
      {
         check(metricPair.Value != nullptr);
         groupData->Values.Add(metricPair.Key.ToString(), MakeJsonValueObjectSafe(metricPair.Value(queryParams)));
      }
      result.Data->Values.Add(groupPair.Key.ToString(), MakeShared<FJsonValueObject>(groupData));
   }
   return result;
}

FOSEMetricsResult FOSEMetricsSystem::QueryGroup(FName groupName, const TMap<FString, FString>& queryParams) const
{
   // Make sure the group is valid
   const FMetricsGroup* group = _metricsGroups.Find(groupName);
   if (group == nullptr)
   {
      return FOSEMetricsResult::MakeNotFound(groupName);
   }
   FOSEMetricsResult result = FOSEMetricsResult::Make();
   for (const auto& pair : group->Metrics)
   {
      result.Data->Values.Add(pair.Key.ToString(), MakeJsonValueObjectSafe(pair.Value(queryParams)));
   }
   return result;
}

FOSEMetricsResult FOSEMetricsSystem::Query(FName groupName, FName metricName, const TMap<FString, FString>& queryParams) const
{
   // Make sure the group is valid
   const FMetricsGroup* group = _metricsGroups.Find(groupName);
   if (group == nullptr)
   {
      return FOSEMetricsResult::MakeNotFound(groupName);
   }
   const FOSEMetricsValueCallback* valueCallback = group->Metrics.Find(metricName);
   if (valueCallback == nullptr)
   {
      return FOSEMetricsResult::MakeNotFound(groupName, metricName);
   }
   FOSEMetricsResult result = FOSEMetricsResult::Make();
   result.Data->Values.Add(metricName.ToString(), MakeJsonValueObjectSafe((*valueCallback)(queryParams)));
   return result;
}

bool FOSEMetricsSystem::_TickerFn(float gameWorldDeltaSeconds)
{
   // The deltaTime argument we get is related to the game world, not the time since this tick function was last run, so we need to compute our own deltaTime.
   const double now = FPlatformTime::Seconds();
   const float timeSinceLastTick = (_tickDelegateLastCallTime <= 0) ? (1.0 / 60.0) : (now - _tickDelegateLastCallTime);
   _tickDelegateLastCallTime = now;

   Tick(timeSinceLastTick);

   // This tick function is called via FTSTicker, which assumes that a return value of true means that it should continue ticking.
   constexpr bool continueTicking = true;
   return continueTicking;
}
