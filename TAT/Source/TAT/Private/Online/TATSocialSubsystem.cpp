// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATSocialSubsystem.h"

#include <Logging/StructuredLog.h>

#include "GameFramework/OSEOnlineSessionClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSocialSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATSocial, Log, All);


void UTATSocialSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATSocialSubsystem::Deinitialize()
{
   Super::Deinitialize();
}

bool UTATSocialSubsystem::IsAvailable() const
{
   return false;
}

bool UTATSocialSubsystem::SendFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::CancelFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

TArray<FTATSocialFriendInvite> UTATSocialSubsystem::GetSentFriendInviteList()
{
   return {};
}

bool UTATSocialSubsystem::AcceptFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::DeclineFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

TArray<FTATSocialFriendInvite> UTATSocialSubsystem::GetReceivedFriendInviteList()
{
   return {};
}

TArray<FTATSocialFriend> UTATSocialSubsystem::GetFriendList()
{
   return {};
}

bool UTATSocialSubsystem::RemoveFriend(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::SendPartyInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::CancelPartyInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

TArray<FTATSocialPartyInvite> UTATSocialSubsystem::GetSentPartyInviteList()
{
   return {};
}

bool UTATSocialSubsystem::AcceptPartyInvite(const FString& inviteId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::DeclinePartyInvite(const FString& inviteId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

TArray<FTATSocialPartyInvite> UTATSocialSubsystem::GetReceivedPartyInviteList()
{
   return {};
}

TOptional<FTATSocialParty> UTATSocialSubsystem::GetParty()
{
   return {};
}

bool UTATSocialSubsystem::GetParty(FTATSocialParty& outParty)
{
   return false;
}

TArray<FTATSocialPartyMember> UTATSocialSubsystem::GetPartyMemberList()
{
   return {};
}

bool UTATSocialSubsystem::PromotePartyMember(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::KickPartyMember(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete)
{
   return false;
}

bool UTATSocialSubsystem::SetPartyMemberReady(bool bIsReady, const FTATSocialOpDelegate& onComplete)
{
   return false;
}
