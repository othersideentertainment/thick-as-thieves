// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetricsOutputHTTPServer.h"

// ose
#include "OSEMetrics.h"
#include "OSEMetricsSettings.h"
#include "OSEMetricsSystem.h"

// ue
#include "HttpServerModule.h"

static const FString kJsonMimetype = TEXT("application/json");
static constexpr bool kFailToCreateRouterOnBindFailure = true;


static bool HandleMetricsResult(const FHttpServerRequest& request, const FOSEMetricsResult& metricsResult, const FHttpResultCallback& onComplete)
{
   FString jsonString;

   switch (metricsResult.Status)
   {
   case FOSEMetricsResult::EStatus::Success:
      if (!metricsResult.ToJsonString(jsonString))
      {
         UE_LOG(LogOSEMetrics, Error, TEXT("Error in HTTP GET request for path '%s': failed to serialize metrics"), *request.RelativePath.GetPath());
         onComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError));
         return true;
      }
      onComplete(FHttpServerResponse::Create(jsonString, kJsonMimetype));
      return true;

   case FOSEMetricsResult::EStatus::NotFound:
      UE_LOG(LogOSEMetrics, Error, TEXT("Error in HTTP GET request for path '%s' (NotFound): %s"), *request.RelativePath.GetPath(), *metricsResult.ErrorMessage);
      onComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::NotFound));
      return true;

   case FOSEMetricsResult::EStatus::Error:
      UE_LOG(LogOSEMetrics, Error, TEXT("Error in HTTP GET request for path '%s': %s"), *request.RelativePath.GetPath(), *metricsResult.ErrorMessage);
      onComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError));
      return true;

   default:
      break;
   }

   UE_LOG(LogOSEMetrics, Error, TEXT("Unknown Error in HTTP GET request for path '%s'"), *request.RelativePath.GetPath());
   return false;
}


FOSEMetricsOutputHTTPServer::FOSEMetricsOutputHTTPServer(uint16 port)
   : _port(port)
   , _router(FHttpServerModule::Get().GetHttpRouter(port, kFailToCreateRouterOnBindFailure))
{
   const int32 numPortsToTry = FMath::Max(1, UOSEMetricsSettings::Get().MaxNumPortsToTry) - 1;
   int32 numTries = 1;
   while (!_router && numTries < numPortsToTry && _port < std::numeric_limits<uint16_t>::max() - static_cast<uint16>(numPortsToTry))
   {
      UE_LOG(LogOSEMetrics, Warning, TEXT("Failed to get HTTP router on port %d (bind failure). Trying a different port..."), port);
      ++_port;
      _router = FHttpServerModule::Get().GetHttpRouter(_port, kFailToCreateRouterOnBindFailure);
      ++numTries;
   }

   if (numTries > 1)
   {
      if (_router)
      {
         UE_LOG(LogOSEMetrics, Warning, TEXT("Failed to bind metrics server to port %d, but successfully listened on port %d as a fallback"), port, _port);
      }
      else
      {
         UE_LOG(LogOSEMetrics, Error, TEXT("Failed to bind metrics server to port %d and failed to find a functional fallback"), port);
      }
   }

   if (!_router)
   {
      return;
   }

   _allMetricsRouteHandle = _router->BindRoute(
      FHttpPath(TEXT("/all")),
      EHttpServerRequestVerbs::VERB_GET,
      FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& request, const FHttpResultCallback& onComplete) -> bool
      {
         return this->_onHttpGetAll(request, onComplete);
      }));

   UE_LOG(LogOSEMetrics, Log, TEXT("Started metrics server on port %d"), _port);
}

FOSEMetricsOutputHTTPServer::~FOSEMetricsOutputHTTPServer()
{
   if (_router)
   {
      for (const auto& groupPair : _metricsGroups)
      {
         for (const auto& metricPair : groupPair.Value.MetricsHandles)
         {
            if (metricPair.Value.IsValid())
            {
               _router->UnbindRoute(metricPair.Value);
            }
         }

         if (groupPair.Value.GroupHandle.IsValid())
         {
            _router->UnbindRoute(groupPair.Value.GroupHandle);
         }
      }

      if (_allMetricsRouteHandle.IsValid())
      {
         _router->UnbindRoute(_allMetricsRouteHandle);
      }
   }
}

bool FOSEMetricsOutputHTTPServer::Enable()
{
   if (!_router)
   {
      return false;
   }
   UE_LOG(LogOSEMetrics, Log, TEXT("FOSEMetricsOutputHTTPServer started listening on port %d"), _port);
   FHttpServerModule::Get().StartAllListeners();
   return true;
}

void FOSEMetricsOutputHTTPServer::Disable()
{
   if (!_router)
   {
      return;
   }
   UE_LOG(LogOSEMetrics, Log, TEXT("FOSEMetricsOutputHTTPServer stopped listening on port %d"), _port);
   FHttpServerModule::Get().StopAllListeners();
}

void FOSEMetricsOutputHTTPServer::OnMetricGroupAdded(FName groupName)
{
   if (!_router || _metricsGroups.Contains(groupName))
   {
      return;
   }

   FMetricsGroup& newGroup = _metricsGroups.Add(groupName);

   TWeakPtr<FOSEMetricsOutputHTTPServer> weakSelf = AsWeak();
   newGroup.GroupHandle = _router->BindRoute(
      FHttpPath(FString::Printf(TEXT("/%s"), *groupName.ToString())),
      EHttpServerRequestVerbs::VERB_GET,
      FHttpRequestHandler::CreateLambda([weakSelf, groupName](const FHttpServerRequest& request, const FHttpResultCallback& onComplete) -> bool
      {
         if (TSharedPtr<FOSEMetricsOutputHTTPServer> self = weakSelf.Pin())
         {
            return self->_onHttpGetGroup(groupName, request, onComplete);
         }
         return false;
      })
   );
}

void FOSEMetricsOutputHTTPServer::OnMetricAdded(FName groupName, FName metricName)
{
   if (!_router)
   {
      return;
   }

   FMetricsGroup* group = _metricsGroups.Find(groupName);
   if (group == nullptr)
   {
      return;
   }

   if (group->MetricsHandles.Contains(metricName))
   {
      return;
   }

   TWeakPtr<FOSEMetricsOutputHTTPServer> weakSelf = AsWeak();
   group->MetricsHandles.Add(metricName, _router->BindRoute(
         FHttpPath(FString::Printf(TEXT("/%s/%s"), *groupName.ToString(), *metricName.ToString())),
         EHttpServerRequestVerbs::VERB_GET,
         FHttpRequestHandler::CreateLambda([weakSelf, groupName, metricName](const FHttpServerRequest& request, const FHttpResultCallback& onComplete) -> bool
         {
            if (TSharedPtr<FOSEMetricsOutputHTTPServer> self = weakSelf.Pin())
            {
               return self->_onHttpGetMetric(groupName, metricName, request, onComplete);
            }
            return false;
         })
      ));
}

FString FOSEMetricsOutputHTTPServer::ToString() const
{
   return FString::Printf(TEXT("FOSEMetricsOutputHTTPServer(port: %i)"), (int32)_port);
}

bool FOSEMetricsOutputHTTPServer::_onHttpGetAll(const FHttpServerRequest& request, const FHttpResultCallback& onComplete)
{
   TSharedPtr<FOSEMetricsSystem> metricsSystem = _GetMetricsSystem();
   if (!metricsSystem)
   {
      return false;
   }
   return HandleMetricsResult(request, metricsSystem->QueryAll(request.QueryParams), onComplete);
}

bool FOSEMetricsOutputHTTPServer::_onHttpGetGroup(FName groupName, const FHttpServerRequest& request, const FHttpResultCallback& onComplete)
{
   TSharedPtr<FOSEMetricsSystem> metricsSystem = _GetMetricsSystem();
   if (!metricsSystem)
   {
      return false;
   }
   return HandleMetricsResult(request, metricsSystem->QueryGroup(groupName, request.QueryParams), onComplete);
}

bool FOSEMetricsOutputHTTPServer::_onHttpGetMetric(FName groupName, FName metricName, const FHttpServerRequest& request, const FHttpResultCallback& onComplete)
{
   TSharedPtr<FOSEMetricsSystem> metricsSystem = _GetMetricsSystem();
   if (!metricsSystem)
   {
      return false;
   }
   return HandleMetricsResult(request, metricsSystem->Query(groupName, metricName, request.QueryParams), onComplete);
}
