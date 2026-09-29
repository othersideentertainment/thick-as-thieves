// (c) 2022-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetricsSettings.h"

// ose
#include "OSEMetrics.h"


UOSEMetricsSettings::UOSEMetricsSettings()
{
}

// static
bool UOSEMetricsSettings::GetMetricsServerConfig(FMetricsConfig& outConfig)
{
   const UOSEMetricsSettings& settings = UOSEMetricsSettings::Get();
   
   auto validatePort = [](int32 port) -> TOptional<uint16>
   {
      if (port > 0 && port < std::numeric_limits<uint16>::max())
      {
         return static_cast<uint16>(port);
      }
      return NullOpt;
   };

   outConfig = {};
   outConfig.Enabled = settings.EnableMetricsServer;
   outConfig.FileOutput = settings.EnableFileOutput;
   outConfig.HTTPServerOutput = settings.EnableHTTPServer;
   outConfig.Port = validatePort(settings.DefaultPort);

   // Check for command line overrides
   int32 portFlag = static_cast<uint16>(settings.DefaultPort);
   if (FParse::Value(FCommandLine::Get(), TEXT("-MetricsPort="), portFlag))
   {
      if (auto port = validatePort(portFlag))
      {
         // Force-enable the metrics server if the -MetricsPort flag is used
         outConfig.Enabled = true;
         outConfig.HTTPServerOutput = true;
         outConfig.Port = port;
      }
      else
      {
         UE_LOG(LogOSEMetrics, Error, TEXT("Failed to set metrics port: invalid value %i"), portFlag);
      }
   }

   bool fileOutputFlag = false;
   if (FParse::Bool(FCommandLine::Get(), TEXT("-MetricsFileOutput="), fileOutputFlag))
   {
      if (fileOutputFlag)
      {
         // Force-enable the metrics server if the -MetricsFileOutput flag is used
         outConfig.Enabled = true;
         outConfig.FileOutput = true;
      }
   }

   return outConfig.Enabled;
}

