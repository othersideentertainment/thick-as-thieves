// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OSEGameInstance.generated.h"


UCLASS()
class OSECORE_API UOSEGameInstance : public UGameInstance
{
   GENERATED_BODY()

public:
   virtual void PostLoad() override;
   virtual void LoadComplete(const float LoadTime, const FString& MapName) override;
   virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
};
