// (c) 2020-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Analytics/TATAnalyticsMgr.h"

// tat
#include "TATGameInstance.h"
#include "Common/TATVersionEdition.h"
#include "GameFramework/TATWorldSettings.h"
#include "NetJobs/TATNetJobAnalytics.h"
#include "SaveGame/TATSaveGame.h"

// ose
#include "Graphics/Performance/OSEPerformanceTestComponent.h"
#include "NetJobs/NetJobMgr.h"
#include "NetJobs/NetJobTypes.h"

// ue4
#include "Dom/JsonObject.h"
#include "GameFramework/GameUserSettings.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/DateTime.h"
#include "Misc/CoreDelegates.h"
#include "HttpModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnalyticsMgr)


DEFINE_LOG_CATEGORY_STATIC(LogTATAnalyticsMgr, Verbose, All);

namespace AnalyticsMgrHelpers
{
   static int64 MillisecondsSinceEpoch(const FDateTime& date)
   {
      return (date.GetTicks() - FDateTime(1970, 1, 1).GetTicks()) / ETimespan::TicksPerMillisecond;
   }
}

UTATAnalyticsMgr& UTATAnalyticsMgr::Get(const UObject* contextObject)
{
   return UTATGameInstance::Get(contextObject).GetAnalyticsMgr();
}

void UTATAnalyticsMgr::Init()
{
#if WITH_EDITOR
   _environment = TEXT("editor");
#else
   // until environment is a thing that exists and has meaning
   _environment = TEXT("local");
#endif
}

void UTATAnalyticsMgr::Shutdown()
{
   // The Http module appears to defer closing while the request is pending
   // So end-on-shutdown appears to be valid for non-crash PCs, which is all we care about for MS3
   // (is shutdown order consistent?)
   if (_inSession)
   {
      EndSession();
   }
}

void UTATAnalyticsMgr::OnPostLoadMapWithWorld(UWorld* loadedWorld)
{
   if (!IsValid(loadedWorld))
      return;
   UGameInstance* gameInstance = loadedWorld->GetGameInstance();
   check(gameInstance);

   // only on client for now
   if(gameInstance->IsDedicatedServerInstance())
      return;

   // and don't start a session when running automated tests
   if (UOSEPerformanceTestComponent::IsRunningPerformanceTest())
      return;

   // For now, silly session definition = not on a menu
   const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(loadedWorld);
   bool shouldBeInSession = worldSettings.MapType != ETATMapType::Menu;

   if (shouldBeInSession && !_inSession)
   {
      StartSession();
   }
   else if (!shouldBeInSession && _inSession)
   {
      EndSession();
   }
}

void UTATAnalyticsMgr::StartSession()
{
   _inSession = true;
   _sessionGuid = FGuid::NewGuid();

   TSharedRef<FJsonObject> event = _MakeEvent(TEXT("sessionStart"));
   _AddBuildAndDeviceInfo(*event);
   _AddScalabilityInfo(*event);
   _SendRequest(event);
}

void UTATAnalyticsMgr::EndSession()
{
   TSharedRef<FJsonObject> event = _MakeEvent(TEXT("sessionEnd"));
   _AddBuildAndDeviceInfo(*event);
   _AddScalabilityInfo(*event);
   _SendRequest(event);
   _inSession = false;
}

TSharedRef<FJsonObject> UTATAnalyticsMgr::_MakeEvent(const TCHAR* eventName) const
{
   using namespace AnalyticsMgrHelpers;
   TSharedRef<FJsonObject> event = MakeShared<FJsonObject>();
   event->SetStringField(TEXT("id"), FGuid::NewGuid().ToString());
   event->SetNumberField(TEXT("eventTime"), MillisecondsSinceEpoch(FDateTime::UtcNow()));
   event->SetStringField(TEXT("messageName"), eventName);
   event->SetStringField(TEXT("environment"), _environment);
   if (_inSession)
   {
      event->SetStringField(TEXT("sessionID"), _sessionGuid.ToString());
   }
   _AddPlayerId(*event);
   return event;
}

void UTATAnalyticsMgr::_AddBuildAndDeviceInfo(FJsonObject& payload) const
{
   // If these are stable and not-super-cheap, could pre-compute in Map, and bulk add them to payload
   payload.SetStringField(TEXT("clientVersion"), UTATVersion::GetBuildVersionString());
   payload.SetStringField(TEXT("platform"), FPlatformProperties::IniPlatformName());
   payload.SetStringField(TEXT("device"), FPlatformMisc::GetDeviceMakeAndModel());
   payload.SetStringField(TEXT("cpu"), FPlatformMisc::GetCPUBrand());
   payload.SetStringField(TEXT("gpu"), FPlatformMisc::GetPrimaryGPUBrand());
   payload.SetNumberField(TEXT("cpuCoreCount"), FPlatformMisc::NumberOfCores());
   payload.SetNumberField(TEXT("ramPhysicalGB"), FPlatformMemory::GetPhysicalGBRam());

   const FInternationalization& i18n = FInternationalization::Get();
   payload.SetStringField(TEXT("systemLanguage"), i18n.GetDefaultLanguage()->GetName());
   payload.SetStringField(TEXT("titleLanguage"), i18n.GetCurrentLanguage()->GetName());
}

void UTATAnalyticsMgr::_AddScalabilityInfo(FJsonObject& payload) const
{
   struct FScalabilitySettingAndFieldName
   {
      const TCHAR* ScalabilitySetting = TEXT("");
      const TCHAR* FieldName = TEXT("");
   };

   FScalabilitySettingAndFieldName scalabilitySettingsAndFieldNames[] =
   {
      { TEXT("sg.ResolutionQuality"),   TEXT("resolutionQuality") },
      { TEXT("sg.ViewDistanceQuality"), TEXT("viewDistanceQuality") },
      { TEXT("sg.ShadowQuality"),       TEXT("shadowQuality") },
      { TEXT("sg.PostProcessQuality"),  TEXT("postProcessQuality") },
      { TEXT("sg.TextureQuality"),      TEXT("textureQuality") },
      { TEXT("sg.EffectsQuality"),      TEXT("effectsQuality") },
      { TEXT("sg.ShadingQuality"),      TEXT("shadingQuality") },
   };

   for (FScalabilitySettingAndFieldName scalabilitySettingsAndFieldName : scalabilitySettingsAndFieldNames)
   {
      const IConsoleVariable* scalabilitySetting = IConsoleManager::Get().FindConsoleVariable(scalabilitySettingsAndFieldName.ScalabilitySetting);
      check(scalabilitySetting);
      if (scalabilitySetting->IsVariableFloat())
      {
         payload.SetNumberField(scalabilitySettingsAndFieldName.FieldName, scalabilitySetting->GetFloat());
      }
      else
      {
         payload.SetNumberField(scalabilitySettingsAndFieldName.FieldName, scalabilitySetting->GetInt());
      }
   }

   UGameUserSettings* userSettings =  GEngine->GetGameUserSettings();
   float lastCPUBenchmarkResults = userSettings->GetLastCPUBenchmarkResult();
   float lastGPUBenchmarkResults = userSettings->GetLastGPUBenchmarkResult();

   payload.SetNumberField(TEXT("lastCPUBenchmark"), lastCPUBenchmarkResults);
   payload.SetNumberField(TEXT("lastGPUBenchmark"), lastGPUBenchmarkResults);
}

void UTATAnalyticsMgr::_AddPlayerId(FJsonObject& payload) const
{
   // TODO: cache this once it has a lifetime you can listen for (e.g. is not fake)
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (saveGame)
   {
      payload.SetStringField(TEXT("playerID"), saveGame->GetPlayerAnalyticsId().ToString());
   }
}

void UTATAnalyticsMgr::_SendRequest(const TSharedRef<FJsonObject>& payload) const
{
   UE_LOG(LogTATAnalyticsMgr, VeryVerbose, TEXT("AnalyticsPayload: %s"), *JsonStringHelpers::CreateStringFromJsonObject(payload));

   // Just skip the actual sending in PIE
#if WITH_EDITOR
   if (GetWorld()->IsPlayInEditor())
   {
      return;
   }
#endif
}

void UTATAnalyticsMgr::_OnNetJobCompleted(const FNetJobCompleteInfo& info)
{
   TATNetJobAnalytics* job = info.GetJobAs<TATNetJobAnalytics>();
   if (info.Success)
   {
      UE_LOG(LogTATAnalyticsMgr, Verbose, TEXT("Analytics response: %s"), *job->CreateJsonResponseString());
   }
   else
   {
      UE_LOG(LogTATAnalyticsMgr, Warning, TEXT("Analytics request failed"));
   }
}

