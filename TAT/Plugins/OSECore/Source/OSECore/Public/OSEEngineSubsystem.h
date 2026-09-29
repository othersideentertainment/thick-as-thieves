// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "OSEEngineSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class OSECORE_API UOSEEngineSubsystem : public UEngineSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
