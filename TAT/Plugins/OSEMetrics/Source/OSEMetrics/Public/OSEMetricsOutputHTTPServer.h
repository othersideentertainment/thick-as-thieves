// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "OSEMetricsOutputBase.h"

// ue
#include "IHttpRouter.h"


class OSEMETRICS_API FOSEMetricsOutputHTTPServer
   : public FOSEMetricsOutputBase
   , public TSharedFromThis<FOSEMetricsOutputHTTPServer>
{
   struct FMetricsGroup
   {
      TMap<FName, FHttpRouteHandle> MetricsHandles;
      FHttpRouteHandle GroupHandle;
   };

   uint16 _port;
   TSharedPtr<IHttpRouter> _router;
   FHttpRouteHandle _allMetricsRouteHandle;
   TMap<FName, FMetricsGroup> _metricsGroups;

public:
   explicit FOSEMetricsOutputHTTPServer(uint16 port);
   virtual ~FOSEMetricsOutputHTTPServer();

   virtual bool Enable() override;
   virtual void Disable() override;

   virtual void OnMetricGroupAdded(FName groupName) override;
   virtual void OnMetricAdded(FName groupName, FName metricName) override;

   virtual FString ToString() const override;

private:
   // NOTE - Returning true implies that the delegate will eventually invoke OnComplete
   // NOTE - Returning false implies that the delegate will never invoke OnComplete
   // request: The incoming http request to be handled
   // onComplete: The callback to invoke to write an http response
   // Handlers should return true if the request has been handled, false otherwise
   bool _onHttpGetAll(const FHttpServerRequest& request, const FHttpResultCallback& onComplete);
   bool _onHttpGetGroup(FName groupName, const FHttpServerRequest& request, const FHttpResultCallback& onComplete);
   bool _onHttpGetMetric(FName groupName, FName metricName, const FHttpServerRequest& request, const FHttpResultCallback& onComplete);
};
