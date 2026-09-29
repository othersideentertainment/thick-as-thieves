// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Performance/OSEPerformanceTestCameraActor.h"

// ose
#include "Camera/OSECameraComponent.h"
#include "Camera/OSECameraUtils.h"

// ue5
#include "Camera/CameraComponent.h"
#include "Engine/Level.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "WorldPartition/DataLayer/DataLayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerformanceTestCameraActor)

namespace PerfTestCvars {
   static TAutoConsoleVariable<int32> AllowRotation(
      TEXT("OSE.PerfTest.Rotation.Allow"),
      0,
      TEXT("Whether perf test cameras will spin around the Z axis"),
      ECVF_Default
   );

   static TAutoConsoleVariable<float> RotationInterval(
      TEXT("OSE.PerfTest.Rotation.SecondsPerRotation"),
      2.5,
      TEXT("Duration of a rotation in seconds"),
      ECVF_Default
   );
}

AOSEPerformanceTestCameraActor::AOSEPerformanceTestCameraActor(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer.SetDefaultSubobjectClass<UOSECameraComponent>(FCameraComponentName::Component))
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   PerformanceTestCameraName = TEXT("Performance Test Camera");
}

void AOSEPerformanceTestCameraActor::BeginPlay()
{
   Super::BeginPlay();
   _originalRotation = GetActorRotation();
}

void AOSEPerformanceTestCameraActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void AOSEPerformanceTestCameraActor::BecomeViewTarget(APlayerController* pc)
{
   Super::BecomeViewTarget(pc);
}

void AOSEPerformanceTestCameraActor::EndViewTarget(APlayerController* pc)
{
   Super::EndViewTarget(pc);
}

void AOSEPerformanceTestCameraActor::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   FRotator rotation(0);
   rotation.Yaw = (360 / PerfTestCvars::RotationInterval.GetValueOnGameThread()) * deltaSeconds;

   AddActorWorldRotation(rotation);
}

void AOSEPerformanceTestCameraActor::StartTesting()
{
   if (AllowRotation && PerfTestCvars::AllowRotation.GetValueOnGameThread())
   {
      SetActorRotation(_originalRotation);
      SetActorTickEnabled(true);
   }
}

void AOSEPerformanceTestCameraActor::StopTesting()
{
   SetActorTickEnabled(false);
}

#if WITH_EDITOR
void AOSEPerformanceTestCameraActor::CheckForErrors()
{
   Super::CheckForErrors();

   // we will have a valid level when running this w/ MapCheck and can ensure we're on a perf level
   
   if (ULevel* level = GetLevel())
   {
      bool isOnPerfSublevelOrDataLayer = false;
      if (UObject* outer = level->GetOuter())
      {
         // ensure we're on a _Perf level
         FString outerName = outer->GetName();
         if (outerName.Contains(TEXT("_Perf")) || outerName.Contains(TEXT("_perf")))
         {
            isOnPerfSublevelOrDataLayer = true;
         }
      }

      for (const UDataLayerInstance* dataLayer : GetDataLayerInstances())
      {
         if (dataLayer->IsRuntime() && dataLayer->GetDataLayerShortName().StartsWith(TEXT("Perf")))
         {
            isOnPerfSublevelOrDataLayer = true;
         }
      }

      if (!isOnPerfSublevelOrDataLayer)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("[OSEPerformanceTestCameraActor] %s is a perf test camera that isn't on a perf sublevel or datalayer!"), *GetDebugName(this)))));
      }
   }
}
#endif

