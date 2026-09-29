// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSESchedulerTestWorkloadActor.h"


// Sets default values
AOSESchedulerTestWorkloadActor::AOSESchedulerTestWorkloadActor()
{
   // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true;
   TestWorkload = CreateDefaultSubobject<UOSESchedulerTestWorkload>(TEXT("Workload"));
}

// Called when the game starts or when spawned
void AOSESchedulerTestWorkloadActor::BeginPlay()
{
   Super::BeginPlay();
   
}

// Called every frame
void AOSESchedulerTestWorkloadActor::Tick(float DeltaTime)
{
   Super::Tick(DeltaTime);
}

