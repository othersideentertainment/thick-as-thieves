// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Social/TATSocialFriendsList.h"

// tat
#include "Player/TATPlayerState.h"
#include "UI/TATUIFunctionLibrary.h"

// ose
#include "OnlineSubsystemOSE/OnlineSubsystemOSE.h"

// ue
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSocialFriendsList)

DEFINE_LOG_CATEGORY_STATIC(LogTATSocialFriendsList, Log, All);

static IOnlineFriendsPtr GetFriendsInterface(const UObject* worldContextObject)
{
   const FOnlineSubsystemOSE* onlineSubsystemOSE = FOnlineSubsystemOSE::Get(worldContextObject->GetWorld());
   if (!onlineSubsystemOSE)
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Online Subsystem is not initialized!"));
      return nullptr;
   }

   IOnlineFriendsPtr friendsInterface = onlineSubsystemOSE->GetFriendsInterface();
   if (!friendsInterface)
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Friends Interface is not initialized!"));
      return nullptr;
   }

   return friendsInterface;
}

static FName GetProviderAccountType(const UObject* worldContextObject)
{
   const FOnlineSubsystemOSE* onlineSubsystemOSE = FOnlineSubsystemOSE::Get(worldContextObject->GetWorld());
   if (!onlineSubsystemOSE)
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Online Subsystem is not initialized!"));
      return FName();
   }

   return onlineSubsystemOSE->GetSubsystemName();
}

static IOnlinePresencePtr GetOnlinePresenceInterface(const UObject* worldContextObject)
{
   const FOnlineSubsystemOSE* onlineSubsystemOSE = FOnlineSubsystemOSE::Get(worldContextObject->GetWorld());
   if (!onlineSubsystemOSE)
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Online Subsystem is not initialized!"));
      return nullptr;
   }

   IOnlinePresencePtr presenceInterface = onlineSubsystemOSE->GetPresenceInterface();
   if (!presenceInterface.IsValid())
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Online Presence interface is invalid!"));
      return nullptr;
   }

   return presenceInterface;
}

void UTATSocialFriendsList::NativeOnActivated()
{
   Super::NativeOnActivated();

   const ULocalPlayer* localPlayer = GetOwningLocalPlayer();
   if (!localPlayer)
   {
      UE_LOG(LogTATSocialFriendsList, Error, TEXT("Unable to initialize! {Cannot find local player}"));
      return;
   }

   _localUserNum = localPlayer->GetControllerId();

   _ReadFriendsList();
}

void UTATSocialFriendsList::NativeOnDeactivated()
{
   Super::NativeOnDeactivated();
}

void UTATSocialFriendsList::_ReadFriendsList()
{
   if (IOnlineFriendsPtr friendsInterface = GetFriendsInterface(this))
   {
      friendsInterface->ReadFriendsList(_localUserNum, EFriendsLists::ToString(EFriendsLists::Default), FOnReadFriendsListComplete::CreateUObject(this, &UTATSocialFriendsList::_OnReadFriendsListComplete));
   }
}

void UTATSocialFriendsList::_OnReadFriendsListComplete(int32 readFriendsListLocalUserNum, bool readFriendsListWasSuccessful,
                                                       const FString& readFriendsListListName, const FString& readFriendsListErrorStr)
{
   if (IOnlineFriendsPtr friendsInterface = GetFriendsInterface(this))
   {

      if (readFriendsListWasSuccessful)
      {
         _platformFriendsList.Empty();
         friendsInterface->GetFriendsList(readFriendsListLocalUserNum, readFriendsListListName, _platformFriendsList);
         UE_LOG(LogTATSocialFriendsList, Verbose, TEXT("Outputting friends list:"));
         for (TSharedRef<FOnlineFriend> onlineFriendPtr : _platformFriendsList)
         {
            FString providerUserId = onlineFriendPtr->GetUserId()->ToString();
            UE_LOG(LogTATSocialFriendsList, Verbose, TEXT("   - %s | UserId: %s"), *onlineFriendPtr->GetDisplayName(), *providerUserId);
         }
      }
      else
      {
         UE_LOG(LogTATSocialFriendsList, Error, TEXT("Unable to read friends list! {%s}"), *readFriendsListErrorStr);
      }
   }
}
