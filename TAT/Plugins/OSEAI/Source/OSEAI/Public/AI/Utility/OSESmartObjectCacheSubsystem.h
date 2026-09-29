// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Subsystems/WorldSubsystem.h"
#include "SmartObjectTypes.h"

#include "OSESmartObjectCacheSubsystem.generated.h"

class USmartObjectComponent;

// A simple subsystem to cache the lookups of smart object components
//
// The engine stores these by object path, which is slower to look up, but tolerates
// loading an unloading. This is less of a problem how they are using it, but TAT
// is using the smart objects components frequently, for workflow and other reasons.
//
// This keeps a quick map of handle to weak pointer, falling back to the smart object
// subsystem if it ever becomes invalid again.
UCLASS()
class OSEAI_API UOSESmartObjectCacheSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

   USmartObjectComponent* GetSmartObjectComponentForHandle(FSmartObjectHandle handle);

private:

   TMap<FSmartObjectHandle, TWeakObjectPtr<USmartObjectComponent>> _cachedSmartObjects;
	
};
