// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OSESchedulerTestWorkload.generated.h"

UCLASS()
class OSESCHEDULERTEST_API UOSESchedulerTestWorkload : public UActorComponent
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
   virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
   int TickCount { 0 };
};
