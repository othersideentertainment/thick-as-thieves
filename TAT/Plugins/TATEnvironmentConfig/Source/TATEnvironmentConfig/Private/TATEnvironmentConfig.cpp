// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATEnvironmentConfig.h"

#define LOCTEXT_NAMESPACE "FTATEnvironmentConfigModule"

DEFINE_LOG_CATEGORY_STATIC(LogTATEnvironmentConfig, Log, All);

static FString sEnvironmentName;

static FString ResolveEnvironment()
{
   // see https://otherside.atlassian.net/wiki/spaces/TVT/pages/4933877761/Environment-Specific+ini
   {
      FString environment;
      if (FParse::Value(FCommandLine::Get(), TEXT("environment="), environment))
      {
         return environment;
      }
   }

   {
      FConfigFile environmentConfig;
      {
         const FString basePath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Environment.ini"));
         environmentConfig.Read(basePath);
      }
      {
         const FString overridePath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("EnvironmentOverride.ini"));
         environmentConfig.Combine(overridePath);
      }
      FString environment;
      if (environmentConfig.GetString(TEXT("EnvironmentConfig"), TEXT("Environment"), environment) && !environment.IsEmpty())
      {
         return environment;
      }
   }

#ifdef TAT_DEFAULT_ENVIRONMENT
   return TEXT(TAT_DEFAULT_ENVIRONMENT);
#else
   return GConfig->GetStr(TEXT("EnvironmentConfig"), TEXT("DefaultEnvironment"), GGameIni);
#endif
}

void FTATEnvironmentConfigModule::StartupModule()
{
   const FString environment = ResolveEnvironment();
   sEnvironmentName = environment;
   UE_LOG(LogTATEnvironmentConfig, Log, TEXT("Selected environment: '%s'"), *environment);

   if (environment.IsEmpty())
   {
      return;
   }

   const FName iniNames[] = {FName("Game"), FName("Engine")};
   for (const FName& iniName : iniNames)
   {
      FConfigBranch* configBranch = GConfig->FindBranch(iniName, FString());
      if (!ensure(configBranch))
      {
         continue;
      }

      const FString overrideConfigPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Environment"), environment,
         FString::Printf(TEXT("%s%s.ini"), *environment, *iniName.ToString()));
      if (configBranch->AddDynamicLayerToHierarchy(overrideConfigPath))
      {
         UE_LOG(LogTATEnvironmentConfig, Log, TEXT("Added config %s"), *overrideConfigPath);
      }
      else
      {
         UE_LOG(LogTATEnvironmentConfig, Warning, TEXT("Skipping config %s"), *overrideConfigPath);
      }
   }
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTATEnvironmentConfigModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

const FString& TATEnvironmentConfig::GetEnvironmentName()
{
   return sEnvironmentName;
}

bool TATEnvironmentConfig::ShouldShowEnvironmentName()
{
   return !sEnvironmentName.IsEmpty()
      && GConfig->GetBoolOrDefault(TEXT("EnvironmentConfig"), TEXT("ShowEnvironment"), true, GGameIni);
}

static FAutoConsoleCommand PrintEnvironmentCommand(
   TEXT("TAT.PrintEnvironment"),
   TEXT("Prints the name of the config environment"),
   FConsoleCommandWithOutputDeviceDelegate::CreateLambda([](FOutputDevice& outputDevice)
      {
         outputDevice.Logf(TEXT("Environment: %s"), *sEnvironmentName);
      }),
ECVF_Default);

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTATEnvironmentConfigModule, TATEnvironmentConfig)
