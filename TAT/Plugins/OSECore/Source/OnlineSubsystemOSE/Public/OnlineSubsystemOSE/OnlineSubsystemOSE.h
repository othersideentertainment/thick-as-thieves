// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"

#include "OnlineSubsystemOSE.generated.h"

UENUM(BlueprintType)
enum class EOnlineSubsystemOSEPlatform : uint8
{
   Null,
   Steam,
   XSX,
   PS5,
   MAX,
};

class ONLINESUBSYSTEMOSE_API FOnlineSubsystemOSE : public FOnlineSubsystemImpl
{
public:
   FOnlineSubsystemOSE(FName instanceName);
   virtual ~FOnlineSubsystemOSE() { }

   // static
   static FOnlineSubsystemOSE* Get(const UObject* contextObj);
   static const FName sName;

   // FOnlineSubsystemOSE
   IOnlineSubsystem* GetUnderlyingPlatformSubsystem() const;
   EOnlineSubsystemOSEPlatform GetUnderlyingPlatformSubsystemType() const;

   // from IOnlineSubsystem
   virtual IOnlineSessionPtr GetSessionInterface() const override;
   virtual IOnlineFriendsPtr GetFriendsInterface() const override;
   virtual IOnlinePartyPtr GetPartyInterface() const override;
   virtual IOnlineGroupsPtr GetGroupsInterface() const override;
   virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
   virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
   virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
   virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
   virtual IOnlineVoicePtr GetVoiceInterface() const override;
   virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
   virtual IOnlineTimePtr GetTimeInterface() const override;
   virtual IOnlineIdentityPtr GetIdentityInterface() const override;
   virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
   //virtual IOnlineStorePtr GetStoreInterface() const override; // DEPRECATED
   virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
   virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
   virtual IOnlineEventsPtr GetEventsInterface() const override;
   virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
   virtual IOnlineSharingPtr GetSharingInterface() const override;
   virtual IOnlineUserPtr GetUserInterface() const override;
   virtual IOnlineMessagePtr GetMessageInterface() const override;
   virtual IOnlinePresencePtr GetPresenceInterface() const override;
   virtual IOnlineChatPtr GetChatInterface() const override;
   virtual IOnlineStatsPtr GetStatsInterface() const override;
   virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
   virtual IOnlineTournamentPtr GetTournamentInterface() const override;
   virtual IOnlineGameActivityPtr GetGameActivityInterface() const override;
   virtual IOnlineGameItemStatsPtr GetGameItemStatsInterface() const override;
   virtual IOnlineGameMatchesPtr GetGameMatchesInterface() const override;

   virtual bool Init() override;
   virtual bool Shutdown() override;
   virtual FString GetAppId() const override;
   virtual FText GetOnlineServiceName() const override;
   virtual FName GetSubsystemName() const override;

   // from FTickerObjectBase
   virtual bool Tick(float deltaTime) override;

private:
   IOnlineSubsystem* _GetNullSubsystem() const;

private:
   // cached online subsystem
   IOnlineSubsystem* _cachedPlatformOnlineSubsystem = nullptr;
};

DECLARE_LOG_CATEGORY_EXTERN(LogOnlineSubsystemOSE, Log, All);
