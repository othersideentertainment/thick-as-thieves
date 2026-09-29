// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Engine/DeveloperSettings.h"

#include "OSEMetricsSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Metrics Settings"))
class OSEMETRICS_API UOSEMetricsSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UOSEMetricsSettings();

   // static access
   static const UOSEMetricsSettings& Get() { return *GetDefault<UOSEMetricsSettings>(); }
   
   struct FMetricsConfig
   {
      bool Enabled = false;
      bool FileOutput = false;
      bool HTTPServerOutput = false;
      TOptional<uint16> Port;
   };

   /// Checks metrics server config and command line overrides to determine if the metrics server should be enabled,
   /// and if so, what port it should use.
   static bool GetMetricsServerConfig(FMetricsConfig& outConfig);

   /// Metrics server toggle. Can override this with (game-specific) command line flags.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   bool EnableMetricsServer = false;

   /// Output metrics data to a file
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   bool EnableFileOutput = false;

   /// When metrics file output is enabled, how frequently to query game data
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   float FileOutputQueryIntervalSeconds = 1.0f;

   /// When metrics file output is enabled, how frequently to flush it to disk
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   float FileOutputFlushIntervalSeconds = 5.0f;

   /// If true, saves metrics data to the profiling directory. Otherwise saves it to the logs directory.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   bool FileOutputSaveToProfilingDir = true;

   /// Enables an HTTP server that listens on DefaultPort. Allows polling for real-time metrics data.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   bool EnableHTTPServer = false;

   /// Default port to listen on.
   /// If the port is zero, the metrics server will be disabled by default.
   /// You can override the port with (game-specific) command line flags.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   int32 DefaultPort = 9070;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Metrics Server Settings")
   int32 MaxNumPortsToTry = 1;
};
