// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGameInstance.h"

// ose
#include "GameFramework/OSEOnlineSessionClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameInstance)

void UOSEGameInstance::PostLoad()
{
   Super::PostLoad();
}

void UOSEGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
   Super::LoadComplete(LoadTime, MapName);
}

TSubclassOf<UOnlineSession> UOSEGameInstance::GetOnlineSessionClass()
{
   return UOSEOnlineSessionClient::StaticClass();
}

