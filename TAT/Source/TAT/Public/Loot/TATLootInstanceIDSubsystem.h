// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"

#include "TATLootInstanceIDSubsystem.generated.h"

UCLASS()
class TAT_API UTATLootInstanceIDSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:

   static UTATLootInstanceIDSubsystem* Get(const UObject* contextObj);

   // USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   int32 AuthorityGetNextLootID();

private:
   int32 _nextIDToGiveOut = 0;
};
