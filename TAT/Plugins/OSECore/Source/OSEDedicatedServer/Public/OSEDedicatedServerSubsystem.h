// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h"

// ue4
#include "Subsystems/GameInstanceSubsystem.h"

#include "OSEDedicatedServerSubsystem.generated.h"

class UOSEDedicatedServerManagerBase;

UCLASS()
class OSEDEDICATEDSERVER_API UOSEDedicatedServerSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   // Subsystem implementation
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   UOSEDedicatedServerManagerBase* GetDedicatedServerMgr() const { return _serverMgr; }
   
private:
   UPROPERTY(Transient)
   UOSEDedicatedServerManagerBase* _serverMgr = nullptr;
};
