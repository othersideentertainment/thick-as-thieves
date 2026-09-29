// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TATDamageAssetSubsystem.generated.h"

struct FStreamableHandle;

// A subsystem that purely exists to async-preload the damage effects
//
// Could generalize later if a thing other stuff wants to do, but damage effects are small
UCLASS()
class TAT_API UTATDamageAssetSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()
   
public:

   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

private:
   TSharedPtr<FStreamableHandle> _loadingHandle;
};
