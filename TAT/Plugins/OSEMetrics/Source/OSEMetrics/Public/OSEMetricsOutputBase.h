// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class OSEMETRICS_API FOSEMetricsOutputBase
{
   friend class FOSEMetricsSystem;

   TWeakPtr<FOSEMetricsSystem> _weakSystem;

protected:
   TSharedPtr<FOSEMetricsSystem> _GetMetricsSystem() const;

public:
   FOSEMetricsOutputBase();
   virtual ~FOSEMetricsOutputBase();

   virtual bool Enable() { return false; }
   virtual void Disable() {}
   
   virtual void Tick(float deltaSeconds) {}

   virtual void OnMetricGroupAdded(FName groupName) {}
   virtual void OnMetricAdded(FName groupName, FName metricName) {}

   virtual FString ToString() const { return FString(); }
};
