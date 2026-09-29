// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJobs/NetJobTypes.h"

// ue4
#include "CoreMinimal.h"
#include "Misc/Guid.h"

#include "TATAnalyticsMgr.generated.h"

class FJsonObject;
class ATATGameState;
class ATATPlayerState;

// Manages sending analytics
// Currently combines building the events and sending them out, but there is room to separate that
UCLASS(Config=Game)
class TAT_API UTATAnalyticsMgr : public UObject
{
   GENERATED_BODY()

public:
   static UTATAnalyticsMgr& Get(const UObject* contextObject);

   void Init();
   void Shutdown();

   void OnPostLoadMapWithWorld(UWorld* loadedWorld);

   void StartSession();
   void EndSession();

private:
   TSharedRef<FJsonObject> _MakeEvent(const TCHAR* eventName) const;
   void _AddBuildAndDeviceInfo(FJsonObject& payload) const;
   void _AddScalabilityInfo(FJsonObject& payload) const;
   void _AddPlayerId(FJsonObject& payload) const;
   void _SendRequest(const TSharedRef<FJsonObject>& payload) const;

   UFUNCTION()
   void _OnNetJobCompleted(const FNetJobCompleteInfo& info);

private:
   bool _inSession;
   FGuid _sessionGuid;
   FString _environment;
};
