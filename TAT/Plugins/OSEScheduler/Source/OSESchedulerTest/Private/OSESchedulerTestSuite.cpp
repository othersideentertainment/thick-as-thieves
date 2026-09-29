// (c) 2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tests/AutomationCommon.h"
#if WITH_DEV_AUTOMATION_TESTS

// ue5
#include "OSESchedulerTestWorkload.h"
#include "OSESchedulerTestWorkloadActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OSEScheduler_AddAndRunTests, "OSE.Scheduler.AddAndRunTest", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)


bool OSEScheduler_AddAndRunTests::RunTest(const FString& Parameters)
{
   UWorld* world = FAutomationEditorCommonUtils::CreateNewMap();
   world->InitializeActorsForPlay(FURL());

   TArray<AOSESchedulerTestWorkloadActor*> workloadObjects;
   for(int i = 0; i < 3; ++i)
   {
      AOSESchedulerTestWorkloadActor* workloadActor = world->SpawnActor<AOSESchedulerTestWorkloadActor>();
      workloadObjects.Add(workloadActor);
      workloadActor->DispatchBeginPlay();
   }
   
   world->Tick(LEVELTICK_All, .1f);
   GFrameCounter++;

   TestTrue(TEXT("First test workload ticked"), workloadObjects[0]->TestWorkload->TickCount == 1);
   TestTrue(TEXT("Second test workload didn't tick"), workloadObjects[1]->TestWorkload->TickCount == 0);
   TestTrue(TEXT("Third test workload didn't tick"), workloadObjects[2]->TestWorkload->TickCount == 0);

   world->Tick(LEVELTICK_All, .1f);
   GFrameCounter++;

   TestTrue(TEXT("First test workload ticked"), workloadObjects[0]->TestWorkload->TickCount == 1);
   TestTrue(TEXT("Second test workload ticked"), workloadObjects[1]->TestWorkload->TickCount == 1);
   TestTrue(TEXT("Third test workload didn't tick"), workloadObjects[2]->TestWorkload->TickCount == 0);

   world->Tick(LEVELTICK_All, .1f);
   GFrameCounter++;

   TestTrue(TEXT("First test workload ticked"), workloadObjects[0]->TestWorkload->TickCount == 1);
   TestTrue(TEXT("Second test workload ticked"), workloadObjects[1]->TestWorkload->TickCount == 1);
   TestTrue(TEXT("Third test workload ticked"), workloadObjects[2]->TestWorkload->TickCount == 1);
   
   world->CleanupActors();
   world->CleanupWorld();
   world->DestroyWorld(false);
   return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
