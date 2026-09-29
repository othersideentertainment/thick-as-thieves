// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSESchedulerTestWorkload.h"
#include "GameFramework/Actor.h"
#include "OSESchedulerTestWorkloadActor.generated.h"

UCLASS()
class OSESCHEDULERTEST_API AOSESchedulerTestWorkloadActor : public AActor
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   AOSESchedulerTestWorkloadActor();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

public:
   // Called every frame
   virtual void Tick(float DeltaTime) override;

   UPROPERTY(EditDefaultsOnly)
   UOSESchedulerTestWorkload* TestWorkload { nullptr };
};
