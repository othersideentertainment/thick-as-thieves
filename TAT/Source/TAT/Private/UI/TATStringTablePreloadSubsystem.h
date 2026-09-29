// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TATStringTablePreloadSubsystem.generated.h"

struct FStreamableHandle;

// We have been getting bugs where certain text in string tables
// was not showing up. Often replicated.
// It seems like FText loads referencing string tables async, but
// may not always be available soon enough.
//
// So for now, testing the sledgehammer of loading all the string tables
// up-front. Could likely be more selective about it, but starting here.
UCLASS()
class TAT_API UTATStringTablePreloadSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

private:
   TSharedPtr<FStreamableHandle> _streamHandle;
};
