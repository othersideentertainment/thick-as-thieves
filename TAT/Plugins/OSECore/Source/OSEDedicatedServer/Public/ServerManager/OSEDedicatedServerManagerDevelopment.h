// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h"

#include "OSEDedicatedServerManagerDevelopment.generated.h"

UCLASS()
class OSEDEDICATEDSERVER_API UOSEDedicatedServerManagerDevelopment : public UOSEDedicatedServerManagerBase
{
   GENERATED_BODY()

public:
   virtual void Init(UGameInstance* gameInstance) override;
   virtual void Shutdown() override;
};
