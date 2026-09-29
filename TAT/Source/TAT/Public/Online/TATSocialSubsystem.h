// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <Subsystems/LocalPlayerSubsystem.h>

#include "TATSocialSubsystem.generated.h"

UENUM(BlueprintType)
enum class ETATSocialPresence : uint8
{
   Unknown,
   Online,
   Away,
};

USTRUCT(BlueprintType)
struct FTATSocialFriendInvite
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FUniqueNetIdRepl PlayerNetId;

   UPROPERTY(BlueprintReadOnly)
   FString DisplayName;
};

USTRUCT(BlueprintType)
struct FTATSocialFriend
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FUniqueNetIdRepl PlayerNetId;

   UPROPERTY(BlueprintReadOnly)
   FString DisplayName;

   UPROPERTY(BlueprintReadOnly)
   ETATSocialPresence Presence = ETATSocialPresence::Unknown;
};

USTRUCT(BlueprintType)
struct FTATSocialPartyInvite
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FString InviteId;

   UPROPERTY(BlueprintReadOnly)
   FUniqueNetIdRepl SourcePlayerNetId;

   UPROPERTY(BlueprintReadOnly)
   FUniqueNetIdRepl TargetPlayerNetId;
};

USTRUCT(BlueprintType)
struct FTATSocialPartyMember
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FUniqueNetIdRepl PlayerNetId;

   UPROPERTY(BlueprintReadOnly)
   FString DisplayName;

   UPROPERTY(BlueprintReadOnly)
   bool bIsLeader = false;

   UPROPERTY(BlueprintReadOnly)
   bool bIsReady = false;
};

USTRUCT(BlueprintType)
struct FTATSocialParty
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   FString PartyId;

   UPROPERTY(BlueprintReadOnly)
   FString InviteCode;

   UPROPERTY(BlueprintReadOnly)
   TArray<FTATSocialPartyMember> Members;

   UPROPERTY(BlueprintReadOnly)
   TArray<FTATSocialPartyInvite> Invites;
};

// clang-format off
DECLARE_DYNAMIC_DELEGATE_TwoParams(FTATSocialOpDelegate, bool, bSuccess, const FText&, ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialFriendDelegate, const FTATSocialFriend&, Friend);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialFriendListDelegate, const TArray<FTATSocialFriend>&, FriendList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialFriendInviteDelegate, const FTATSocialFriendInvite&, Invite);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialFriendInviteListDelegate, const TArray<FTATSocialFriendInvite>&, InviteList);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTATSocialPartyEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialPartyDelegate, const FTATSocialParty&, Party);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialPartyMemberDelegate, const FTATSocialPartyMember&, Member);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialPartyMemberListDelegate, const TArray<FTATSocialPartyMember>&, MemberList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialPartyInviteDelegate, const FTATSocialPartyInvite&, Invite);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATSocialPartyInviteListDelegate, const TArray<FTATSocialPartyInvite>&, InviteList);
// clang-format on

UCLASS()
class TAT_API UTATSocialSubsystem : public ULocalPlayerSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   bool IsAvailable() const;

   // Friend

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   bool SendFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   bool CancelFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   TArray<FTATSocialFriendInvite> GetSentFriendInviteList();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   bool AcceptFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   bool DeclineFriendInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   TArray<FTATSocialFriendInvite> GetReceivedFriendInviteList();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   TArray<FTATSocialFriend> GetFriendList();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Friend")
   bool RemoveFriend(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   // Party

   // UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   // void CreateParty(const FTATSocialOpDelegate& onComplete);
   //
   // UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   // void JoinParty(const FUniqueNetIdRepl& partyId, const FTATSocialOpDelegate& onComplete);
   //
   // UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   // void JoinPartyWithInviteCode(const FString& inviteCode, const FTATSocialOpDelegate& onComplete);
   //
   // UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   // void LeaveParty(const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool SendPartyInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool CancelPartyInvite(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   TArray<FTATSocialPartyInvite> GetSentPartyInviteList();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool AcceptPartyInvite(const FString& inviteId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool DeclinePartyInvite(const FString& inviteId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   TArray<FTATSocialPartyInvite> GetReceivedPartyInviteList();

   TOptional<FTATSocialParty> GetParty();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party", meta = (ExpandBoolAsExecs = "ReturnValue"))
   bool GetParty(FTATSocialParty& outParty);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   TArray<FTATSocialPartyMember> GetPartyMemberList();

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool PromotePartyMember(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool KickPartyMember(const FUniqueNetIdRepl& playerNetId, const FTATSocialOpDelegate& onComplete);

   UFUNCTION(BlueprintCallable, Category = "TAT | Social | Party")
   bool SetPartyMemberReady(bool bIsReady, const FTATSocialOpDelegate& onComplete);

   // Friend events

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendDelegate OnFriendUpdated;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendDelegate OnFriendRemoved;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendListDelegate OnFriendListChanged;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteDelegate OnSentFriendInviteAccepted;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteDelegate OnSentFriendInviteDeclined;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteListDelegate OnSentFriendInviteListChanged;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteDelegate OnReceivedFriendInvite;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteDelegate OnReceivedFriendInviteCancelled;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Friend")
   FTATSocialFriendInviteListDelegate OnReceivedFriendInviteListChanged;

   // Party events

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyDelegate OnPartyJoined;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyDelegate OnPartyUpdated;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyEventDelegate OnPartyKicked;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyEventDelegate OnPartyLeft;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyMemberDelegate OnPartyMemberJoined;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyMemberDelegate OnPartyMemberUpdated;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyMemberDelegate OnPartyMemberLeft;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyMemberListDelegate OnPartyMemberListChanged;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteDelegate OnSentPartyInviteAccepted;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteDelegate OnSentPartyInviteDeclined;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteListDelegate OnSentPartyInviteListChanged;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteDelegate OnReceivedPartyInvite;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteDelegate OnReceivedPartyInviteCancelled;

   UPROPERTY(BlueprintAssignable, Category = "TAT | Social | Party")
   FTATSocialPartyInviteListDelegate OnReceivedPartyInviteListChanged;

protected:
};
