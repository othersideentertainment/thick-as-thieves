// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// ose
#include "Identity/OSEUserPrivilege.h"

#include "OSEUserPrivilegeWindows.generated.h"

UCLASS()
class UOSEUserPrivilegeWindows : public UOSEUserPrivilege
{
   GENERATED_BODY()

public:
   virtual void InitializePlatform() override;
   virtual void DeinitializePlatform() override;

};
