// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Templates/SharedPointer.h"

#include "OSEIdentityMgr.generated.h"

class UOSEPlatformIdentity;
class UOSEUserPrivilege;
class UOSESaveGameSystem;

//---------------------------------------------------------------------------------------
/// OSE Identity Manager
///
/// A game instance subsystem which instantiates OSE Platform Identity, User Privilege, and
/// Save Game System.
//---------------------------------------------------------------------------------------
UCLASS()
class OSECORE_API UOSEIdentityMgr : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public: 

   // USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

public:
   UFUNCTION(BlueprintCallable, Category = "OSE|Identity")
   UOSEPlatformIdentity* GetPlatformIdentity() const { return _platformIdentity; }

   UFUNCTION(BlueprintCallable, Category = "OSE|Identity")
   UOSEUserPrivilege* GetUserPrivilege() const { return _userPrivilege; }

   UFUNCTION(BlueprintCallable, Category = "OSE|Identity")
   UOSESaveGameSystem* GetSaveGameSystem() const { return _saveGameSystem; }

private:
   UPROPERTY(Transient)
   UOSEPlatformIdentity* _platformIdentity = nullptr;

   UPROPERTY(Transient)
   UOSEUserPrivilege* _userPrivilege = nullptr;

   UPROPERTY(Transient)
   UOSESaveGameSystem* _saveGameSystem = nullptr;
};
