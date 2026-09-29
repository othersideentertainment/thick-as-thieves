// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATActivatableWidget.h"

// ue
#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"


#include "TATSocialFriendsList.generated.h"


/**
 * Class for reading the friends list from the online subsystem and merging it with the pragma friends list.
 */
UCLASS()
class TAT_API UTATSocialFriendsList : public UTATActivatableWidget
{
   GENERATED_BODY()

public:
   virtual void NativeOnActivated() override;
   virtual void NativeOnDeactivated() override;

private:
   UFUNCTION()
   void _ReadFriendsList();
   void _OnReadFriendsListComplete(int32 readFriendsListLocalUserNum, bool readFriendsListWasSuccessful, const FString& readFriendsListListName, const FString& readFriendsListErrorStr);

   int32 _localUserNum = 0;
   TArray<TSharedRef<FOnlineFriend>> _platformFriendsList;
};
