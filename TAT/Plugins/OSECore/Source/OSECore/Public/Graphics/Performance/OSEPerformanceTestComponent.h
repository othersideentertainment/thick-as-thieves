// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "Components/ActorComponent.h"
#include "Stats/StatsData.h"

#include "OSEPerformanceTestComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSEPerformanceTest, Log, All);

class AOSEPerformanceTestCameraActor;
class ULevelStreamingDynamic;
class UDataLayerInstance;

struct FStatUnitData;

// explicitly allow in test builds
#ifndef OSE_ALLOW_PERFTEST
#define OSE_ALLOW_PERFTEST !(UE_BUILD_SHIPPING)
#endif

//---------------------------------------------------------------------------------------
/// FStatTracker
//---------------------------------------------------------------------------------------

#if STATS
class FStatTracker : public IItemFilter
{
public:
   TQueue<TMap<FName, FStatMessage>> Queue;

   void Init(const TArray<FName>* stats);
   virtual bool Keep(FStatMessage const& msg) override;
   ~FStatTracker();

private:
   TMap<FName, FName> _longToShortNames;
   FDelegateHandle _delegateHandle;

   void _Init_StatsThread(const TArray<FName>* stats);
   void _Cleanup_StatsThread();
   void _OnNewFrame(int64 frame);
};
#endif // STATS

//---------------------------------------------------------------------------------------
/// UOSEPerformanceTestComponent
//---------------------------------------------------------------------------------------

UENUM()
enum class EPerformanceTestState : uint8
{
   LoadingMap,
   LoadPerfLevels,
   FindPerfCameras,
   SetupStats,
   InitPawn,
   WaitForSettle,
   Running,
   WaitForScalabilityToSettle,
   WritingLogs,
   Complete,
};

struct FPerformanceTestCameraData
{
public:
   FString Name;
   int NumFrames = 0;
   float TotalTime = 0.0f;
   int ScalabilityLevel = 0;

   // total
   float TotalFrameTime = 0.0f;
   float TotalRenderThreadTime = 0.0f;
   float TotalRenderThreadTimeCriticalPath = 0.0f;
   float TotalGameThreadTime = 0.0f;
   float TotalRhiTime = 0.0f;
   float TotalGPUFrameTime[MAX_NUM_GPUS] = {}; // 0-initialized
   float TotalSlateRenderTime = 0.0f;
   int64 TotalPrimitivesDrawn = 0;
   int64 TotalDrawCalls = 0;

   // avg
   float AvgFPS = 0.0f;
   float AvgFrameTime = 0.0f;
   float AvgRenderThreadTime = 0.0f;
   float AvgRenderThreadTimeCriticalPath = 0.0f;
   float AvgGameThreadTime = 0.0f;
   float AvgRhiTime = 0.0f;
   float AvgGPUFrameTime[MAX_NUM_GPUS] = {};
   float AvgSlateRenderTime = 0.0f;
   int64 AvgPrimitivesDrawn = 0.0f;
   int64 AvgDrawCalls = 0;

   // max
   float MaxFrameTime = 0.0f;
   float MaxRenderThreadTime = 0.0f;
   float MaxRenderThreadTimeCriticalPath = 0.0f;
   float MaxGameThreadTime = 0.0f;
   float MaxRhiTime = 0.0f;
   float MaxGPUFrameTime[MAX_NUM_GPUS] = {};
   float MaxSlateRenderTime = 0.0f;
   int64 MaxPrimitivesDrawn = 0;
   int64 MaxDrawCalls = 0;
   float MaxPhysicalMemMB = 0.0f;
   float MaxVirtualMemMB = 0.0f;

   // misc
   int32 NumAI = 0;
   int32 NumAIRendered = 0;

   static TArray<FString> GetHeaderStrArray();
   TArray<FString> ToStrArray() const;
};

UCLASS(NotBlueprintable, NotBlueprintType)
class OSECORE_API UOSEPerformanceTestComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEPerformanceTestComponent();

   // static
   static const FString& GetPerformanceTestMap();
   static bool IsRunningPerformanceTest();

   // from AActor
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

private:
   void _SetState(EPerformanceTestState newState);
   void _Init();
   void _TickLoadPerfLevels();
   void _TickFindPerfCameras();
   void _TickSetupStats();
   void _TickInitPawn();
   void _TickWaitForSettle(float deltaTime);
   void _TickWaitForScalabilityToSettle(float deltaTime);
   void _TickPerformanceTest(float deltaTime);
   void _WriteLogs();
   void _WriteStrArrayToCsvLineStr(const TArray<FString>& strArr, FString& outCvsLineStr);
   void _ExitApp();

   // utl
   APlayerController& _GetOwnerController() const;
   const FStatUnitData* _GetStatUnitData() const;
   TArray<ULevelStreamingDynamic*> _GetPerfLevels() const;
   TArray<const UDataLayerInstance*> _GetPerfDataLayers() const;
   FPerformanceTestCameraData& _GetPerfData(AOSEPerformanceTestCameraActor* camera, int scalabilityLevelIndex);
   void _SetScalabilityLevelIndex(int scalabilityLevelIndex);
   void _SetCameraIndex(int cameraIndex);
   
private:
   // state
   EPerformanceTestState _state = EPerformanceTestState::LoadingMap;
   TArray<AOSEPerformanceTestCameraActor*> _perfCameras;
   int _currentCameraIndex = INDEX_NONE;
   int _currentScalabilityLevelIndex = 0;
   TMap<AOSEPerformanceTestCameraActor*, TArray<FPerformanceTestCameraData>> _perfCameraData;

   // config set in constructor; not sure that this stuff needs to be data-driven...?
   float _timePerCamera = 0.0f;
   TArray<int> _supportedScalabilityLevels;
   float _settleWaitSeconds = 0.0f;
   float _settleWaitSecondsTotal = 0.0f;
   float _settleWaitForScalabilitySecondsTotal = 0.0f;

   // stats
   TArray<FName> _statsToTrack;
#if STATS
   FStatTracker _statTracker;
#endif
};
