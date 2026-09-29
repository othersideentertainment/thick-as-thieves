// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSESchedulerTestWorkload.h"

#include "OSESchedulerTaskSet.h"
#include "OSESchedulerWorldSubsystem.h"

static FName SchedulerWorkloadGroupName(TEXT("SchedulerWorkloadGroup"));

void UOSESchedulerTestWorkload::BeginPlay()
{
   Super::BeginPlay();
   UWorld* world = GetWorld();
   UOSESchedulerWorldSubsystem* schedulerWorldSubsystem = world->GetSubsystem<UOSESchedulerWorldSubsystem>();
   PrimaryComponentTick.UnRegisterTickFunction();
   
   schedulerWorldSubsystem->CreateOrAddScheduledTask(
      SchedulerWorkloadGroupName,
      *this,
      [world] { return MakeShareable(new FOSESchedulerTaskSet_MaxFrameTimeTick(TG_PrePhysics, world, 0.1f)); }
   );
}

void UOSESchedulerTestWorkload::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   if(UWorld* world = GetWorld())
   {
      UOSESchedulerWorldSubsystem* schedulerWorldSubsystem = world->GetSubsystem<UOSESchedulerWorldSubsystem>();
      schedulerWorldSubsystem->RemoveScheduledTask(SchedulerWorkloadGroupName, *this);
   }
   Super::EndPlay(EndPlayReason);
}

void UOSESchedulerTestWorkload::TickComponent(float DeltaTime,
                                              ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
   FPlatformProcess::Sleep(.1f);
   ++TickCount;
}
