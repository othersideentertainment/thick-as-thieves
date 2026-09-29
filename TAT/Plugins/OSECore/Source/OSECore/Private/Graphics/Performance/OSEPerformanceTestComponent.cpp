// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Performance/OSEPerformanceTestComponent.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Graphics/Performance/OSEPerformanceTestCameraActor.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Stats/StatsData.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"
#include "WorldPartition/DataLayer/WorldDataLayers.h"
#include "GameFramework/SpectatorPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerformanceTestComponent)

DEFINE_LOG_CATEGORY(LogOSEPerformanceTest);


namespace PerformanceTestHelpers
{
   struct FAICount
   {
      int NumAI = 0;
      int NumAIRendered = 0;
   };
   FAICount CountAI(const UWorld* world, float renderedTimePeriod)
   {
      int count = 0;
      int numRendered = 0;
      for (FConstControllerIterator iterator = world->GetControllerIterator(); iterator; ++iterator)
      {
         AController* controller = iterator->Get();
         if (controller && !Cast<APlayerController>(controller))
         {
            ++count;
            if (controller->GetPawn() && controller->GetPawn()->WasRecentlyRendered(renderedTimePeriod))
            {
               ++numRendered;
            }
         }
      }
      return FAICount { .NumAI = count, .NumAIRendered = numRendered };
   }
}
//---------------------------------------------------------------------------------------
// FStatTracker
//---------------------------------------------------------------------------------------

#if STATS
void FStatTracker::Init(const TArray<FName>* stats)
{
   // Run init task in stats thread.
   DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.FStatTracker_Init"), STAT_FSimpleDelegateGraphTask_FStatTracker_Init, STATGROUP_TaskGraphTasks);

   // UE5 Port TODO: We should port to the new ue5 stat public apis...?  Removed stats thread usage in the port
   FGraphEventRef completeHandle = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
      FSimpleDelegateGraphTask::FDelegate::CreateRaw(this, &FStatTracker::_Init_StatsThread, stats),
      GET_STATID(STAT_FSimpleDelegateGraphTask_FStatTracker_Init), NULL, ENamedThreads::GameThread
   );
   FTaskGraphInterface::Get().WaitUntilTaskCompletes(completeHandle);

   TArray<FName> foundNames;
   _longToShortNames.GenerateValueArray(foundNames);
   const TArray<FName>& statsArray = *stats;
   for (const FName& stat : statsArray)
   {
      // Can't log from the stats thread, so do this check here instead.
      if (!foundNames.Contains(stat))
         UE_LOG(LogOSEPerformanceTest, Warning, TEXT("Couldn't find long name for stat %s; can't track it."), *stat.ToString());
   }
}

void FStatTracker::_Init_StatsThread(const TArray<FName>*stats)
{
   // Should run in the stats thread.
   //ensure(!(FPlatformProcess::SupportsMultithreading() && IsInGameThread()));

   // thread-local state
   FStatsThreadState& state = FStatsThreadState::GetLocalState();

   // map requested shortnames to long names
   for (const FName& stat : *stats)
   {
      FStatMessage const *msg = state.ShortNameToLongName.Find(stat);
      if (msg)
      {
         const FName& longName = msg->NameAndInfo.GetRawName();
         _longToShortNames.Add(longName, stat);
      }
   }

   // listen for each stat frame
   _delegateHandle = state.NewFrameDelegate.AddRaw(this, &FStatTracker::_OnNewFrame);

   // bump the ref count of things listening for stats to keep the system running
   StatsPrimaryEnableAdd();
}

FStatTracker::~FStatTracker()
{
   // Run cleanup task in stats thread.
   DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.FStatTracker_Cleanup"), STAT_FSimpleDelegateGraphTask_FStatTracker_Cleanup, STATGROUP_TaskGraphTasks);

   // UE5 Port TODO: We should port to the new ue5 stat public apis...?  Removed stats thread usage in the port
   FGraphEventRef completeHandle = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
      FSimpleDelegateGraphTask::FDelegate::CreateRaw(this, &FStatTracker::_Cleanup_StatsThread),
      GET_STATID(STAT_FSimpleDelegateGraphTask_FStatTracker_Cleanup), NULL, ENamedThreads::GameThread
   );
   FTaskGraphInterface::Get().WaitUntilTaskCompletes(completeHandle);
}

void FStatTracker::_Cleanup_StatsThread()
{
   // Should run in the stats thread.
   //ensure(!(FPlatformProcess::SupportsMultithreading() && IsInGameThread()));

   // decrement stats system ref, could potentially disable it
   StatsPrimaryEnableSubtract();

   // Deregister handler for stats frames, so it doesn't get called after this object has been destroyed.
   FStatsThreadState& state = FStatsThreadState::GetLocalState();
   state.NewFrameDelegate.Remove(_delegateHandle);
}

// Filter method for uncondensing stack stats.
bool FStatTracker::Keep(FStatMessage const& msg)
{
   return _longToShortNames.Contains(msg.NameAndInfo.GetRawName());
}

void FStatTracker::_OnNewFrame(int64 frame)
{
   // Should run in the stats thread.
   //ensure(!(FPlatformProcess::SupportsMultithreading() && IsInGameThread()));

   FStatsThreadState& state = FStatsThreadState::GetLocalState();

   FRawStatStackNode root;
   TArray<FStatMessage> statMessages;
   state.UncondenseStackStats(frame, root, this, &statMessages);
   TArray<FRawStatStackNode*> stack;
   root.Children.GenerateValueArray(stack);
   while (stack.Num())
   {
      const FRawStatStackNode* node = stack.Pop();
      statMessages.Add(node->Meta);
      node->Children.GenerateValueArray(stack);
      for (const auto& child : node->Children)
         stack.Add(child.Value);
   }

   TMap<FName, FStatMessage> stats;
   for (const FStatMessage& msg : statMessages)
   {
      const FName& longName = msg.NameAndInfo.GetRawName();
      const FName& name = _longToShortNames.FindRef(longName);
      if (name.IsValid())
         stats.Add(name, msg);
   }
   // Note that Enqueue has move semantics, so this isn't copying a map.
   Queue.Enqueue(stats);
}

#endif // STATS

//---------------------------------------------------------------------------------------
// FPerformanceTestCameraData
//---------------------------------------------------------------------------------------

/* static */
TArray<FString> FPerformanceTestCameraData::GetHeaderStrArray()
{
   TArray<FString> headers =
   {
      // common data
      TEXT("name"),
      TEXT("scalability"),

      // avg
      TEXT("avg_fps"),
      TEXT("avg_frame"),
      TEXT("avg_render_thread"),
      TEXT("avg_render_thread_critical_path"),
      TEXT("avg_game_thread"),
      TEXT("avg_rhi"),
      TEXT("avg_gpu_0_frame"),
      //TEXT("avg_gpu_1_frame"),
      //TEXT("avg_gpu_2_frame"),
      //TEXT("avg_gpu_3_frame"),
      TEXT("avg_slate"),
      TEXT("avg_prims_drawn"),
      TEXT("avg_draw_calls"),

      // max
      TEXT("max_frame"),
      TEXT("max_render_thread"),
      TEXT("max_render_thread_critical_path"),
      TEXT("max_game_thread"),
      TEXT("max_rhi"),
      TEXT("max_gpu_frame_0"),
      //TEXT("max_gpu_frame_1"),
      //TEXT("max_gpu_frame_2"),
      //TEXT("max_gpu_frame_3"),
      TEXT("max_slate"),
      TEXT("max_prims_drawn"),
      TEXT("max_draw_calls"),
      TEXT("max_physical_mem_mb"),
      TEXT("max_virtual_mem_mb"),

      // misc
      TEXT("max_ai"),
      TEXT("max_ai_rendered"),
   };
   return headers;
}

TArray<FString> FPerformanceTestCameraData::ToStrArray() const
{
   return
   {
      // common data
      Name,
      FString::Printf(TEXT("%d"), ScalabilityLevel),

      // avg
      FString::Printf(TEXT("%.03f"), AvgFPS),
      FString::Printf(TEXT("%.03f"), AvgFrameTime),
      FString::Printf(TEXT("%.03f"), AvgRenderThreadTime),
      FString::Printf(TEXT("%.03f"), AvgRenderThreadTimeCriticalPath),
      FString::Printf(TEXT("%.03f"), AvgGameThreadTime),
      FString::Printf(TEXT("%.03f"), AvgRhiTime),
      FString::Printf(TEXT("%.03f"), AvgGPUFrameTime[0]),
      //FString::Printf(TEXT("%.03f"), AvgGPUFrameTime[1]),
      //FString::Printf(TEXT("%.03f"), AvgGPUFrameTime[2]),
      //FString::Printf(TEXT("%.03f"), AvgGPUFrameTime[3]),
      FString::Printf(TEXT("%.03f"), AvgSlateRenderTime),
      FString::Printf(TEXT("%lld"),  AvgPrimitivesDrawn),
      FString::Printf(TEXT("%lld"),  AvgDrawCalls),

      // max
      FString::Printf(TEXT("%.03f"), MaxFrameTime),
      FString::Printf(TEXT("%.03f"), MaxRenderThreadTime),
      FString::Printf(TEXT("%.03f"), MaxRenderThreadTimeCriticalPath),
      FString::Printf(TEXT("%.03f"), MaxGameThreadTime),
      FString::Printf(TEXT("%.03f"), MaxRhiTime),
      FString::Printf(TEXT("%.03f"), MaxGPUFrameTime[0]),
      //FString::Printf(TEXT("%.03f"), MaxGPUFrameTime[1]),
      //FString::Printf(TEXT("%.03f"), MaxGPUFrameTime[2]),
      //FString::Printf(TEXT("%.03f"), MaxGPUFrameTime[3]),
      FString::Printf(TEXT("%.03f"), MaxSlateRenderTime),
      FString::Printf(TEXT("%lld"),  MaxPrimitivesDrawn),
      FString::Printf(TEXT("%lld"),  MaxDrawCalls),
      FString::Printf(TEXT("%.03f"), MaxPhysicalMemMB),
      FString::Printf(TEXT("%.03f"), MaxVirtualMemMB),

      // misc
      FString::Printf(TEXT("%d"),  NumAI),
      FString::Printf(TEXT("%d"),  NumAIRendered),
   };
}

//---------------------------------------------------------------------------------------
// UOSEPerformanceTestComponent
//---------------------------------------------------------------------------------------

UOSEPerformanceTestComponent::UOSEPerformanceTestComponent()
   : Super()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = true;

   // hardcoded config stuff for now!
   _statsToTrack = { "STAT_SlateRenderingRTTime" };
   _timePerCamera = 5.0f;
   _settleWaitSecondsTotal = 20.0f;
   _settleWaitForScalabilitySecondsTotal = 5.f;

   // TODO: allow configuration, but we never actually care about cinematic
   _supportedScalabilityLevels = { 0, 1, 2, 3 }; // 0:low, 1:medium, 2:high, 3:epic, 4:cinematic
}

/* static */
const FString& UOSEPerformanceTestComponent::GetPerformanceTestMap()
{
   struct ParsedMapName
   {
      ParsedMapName()
      {
         FParse::Value(FCommandLine::Get(), TEXT("perf_test="), MapName);
      }

      FString MapName;
   };
   static const ParsedMapName sCache;
   return sCache.MapName;
}

/* static */
bool UOSEPerformanceTestComponent::IsRunningPerformanceTest()
{
   return !GetPerformanceTestMap().IsEmpty();
}

void UOSEPerformanceTestComponent::BeginPlay()
{
   Super::BeginPlay();

   // I'm assuming we're only running perf test on a server / single player for now
   ensure(GetOwner()->HasAuthority());

   // load the map we're intended to be in if we're not already in it
   FString mapName = GetPerformanceTestMap();

   // why does this component exist if we don't have a perf test map?
   check(!mapName.IsEmpty());

   if (GetWorld()->GetMapName() != mapName)
   {
      // not in our perf test map; load it
      _SetState(EPerformanceTestState::LoadingMap);
      UGameplayStatics::OpenLevel(this, FName(mapName));
   }
   else
   {
      // in our perf test map!  let's do some setup.
      _Init();
   }
}

void UOSEPerformanceTestComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   switch(_state)
   {
   case EPerformanceTestState::LoadPerfLevels:
      {
         _TickLoadPerfLevels();
      }
      break;
   case EPerformanceTestState::FindPerfCameras:
      {
         _TickFindPerfCameras();
      }
      break;
   case EPerformanceTestState::SetupStats:
      {

         _TickSetupStats();
      }
      break;
   case EPerformanceTestState::InitPawn:
      {
         _TickInitPawn();
      }
      break;
   case EPerformanceTestState::WaitForSettle:
      {
         _TickWaitForSettle(deltaTime);
      }
      break;
   case EPerformanceTestState::Running:
      {
         _TickPerformanceTest(deltaTime);
      }
      break;
   case EPerformanceTestState::WaitForScalabilityToSettle:
      {
         _TickWaitForScalabilityToSettle(deltaTime);
      }
      break;
   }

   // just spamming this in tick to override any game logic.  maybe not safe?  I'm going
   // to see how long it holds up before doing anything more custom
   {
      APlayerController& pc = _GetOwnerController();
      if (!pc.IsMoveInputIgnored())
         pc.SetIgnoreMoveInput(true);
      if (!pc.IsLookInputIgnored())
         pc.SetIgnoreLookInput(true);

      if (UOSEAbilitySystemComponent* asc = Cast<UOSEAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(pc.GetPawn())))
      {
         if (!asc->IsUserInputInhibited())
         {
            asc->SetUserInputInhibited(true);
         }
      }
   }
}

void UOSEPerformanceTestComponent::_Init()
{
#if STATS
   // init stats tracker
   _statTracker.Init(&_statsToTrack);
#endif // STATS

   // load the perf level which should not be loaded by default -- this happens async
   for (ULevelStreamingDynamic* level : _GetPerfLevels())
   {
      level->SetShouldBeLoaded(true);
      level->SetShouldBeVisible(true);
   }

   if (UDataLayerManager* dataLayerManager = UDataLayerManager::GetDataLayerManager(this))
   {
      for (const UDataLayerInstance* dataLayer : _GetPerfDataLayers())
      {
         dataLayerManager->SetDataLayerInstanceRuntimeState(dataLayer, EDataLayerRuntimeState::Activated, true);
      }
   }

   // perf levels are loading!
   _SetState(EPerformanceTestState::LoadPerfLevels);
}

void UOSEPerformanceTestComponent::_TickLoadPerfLevels()
{
   check(_state == EPerformanceTestState::LoadPerfLevels);

   bool allPerfLevelsLoaded = true;
   for (ULevelStreamingDynamic* level : _GetPerfLevels())
   {
      if (!level->IsLevelLoaded())
      {
         allPerfLevelsLoaded = false;
         break;
      }
   }

   // it is not entirely clear if the data layer effective state actually implies it is loaded
   // but it doesn't hurt, and waiting for the cameras should accomplish that goal
   if (UDataLayerManager* dataLayerManager = UDataLayerManager::GetDataLayerManager(this))
   {
      for (const UDataLayerInstance* dataLayer : _GetPerfDataLayers())
      {
         if (dataLayerManager->GetDataLayerInstanceEffectiveRuntimeState(dataLayer) != EDataLayerRuntimeState::Activated)
         {
            allPerfLevelsLoaded = false;
            break;
         }
      }
   }

   if (allPerfLevelsLoaded)
   {
      // all levels are loaded, next tick we can find our cameras
      _SetState(EPerformanceTestState::FindPerfCameras);
   }
}

void UOSEPerformanceTestComponent::_TickFindPerfCameras()
{
   check(_state == EPerformanceTestState::FindPerfCameras);

   // find all the perf test cameras now that the perf levels are loaded -- we need to do this
   // in Tick() because it takes a couple frames for them to register with the world
   for (TActorIterator<AOSEPerformanceTestCameraActor> it(GetWorld()); it; ++it)
   {
      _perfCameras.Add(CastChecked<AOSEPerformanceTestCameraActor>(*it));
   }

   // TODO: what if there are no perf cameras to be found?
   if (_perfCameras.Num() > 0)
   {
      // pre-populate our data map so we don't have to grow it over time
      // and setup any state we know about now
      _perfCameraData.Reset();
      for(AOSEPerformanceTestCameraActor* camera : _perfCameras)
      {
         TArray<FPerformanceTestCameraData>& dataArr = _perfCameraData.FindOrAdd(camera);
         dataArr.SetNum(_supportedScalabilityLevels.Num());
         for(int idx = 0; idx < _supportedScalabilityLevels.Num(); ++idx)
         {
            FPerformanceTestCameraData& data = dataArr[idx];
            data.Name = camera->PerformanceTestCameraName;
            data.ScalabilityLevel = _supportedScalabilityLevels[idx];
         }
      }

      // next state!
      _SetState(EPerformanceTestState::SetupStats);
   }
}

void UOSEPerformanceTestComponent::_TickSetupStats()
{
   check(_state == EPerformanceTestState::SetupStats);

   UGameViewportClient* viewportClient = GetWorld()->GetGameViewport();
   const FStatUnitData* statUnitData = _GetStatUnitData();
   if (viewportClient && statUnitData)
   {
      // Enable basic stats
      TArray<FString> stats;
      stats.Add("Unit");
      viewportClient->SetEnabledStats(stats);

      // RHI stats (also allows us to get callbacks for FStatTracker)
      GEngine->Exec(GetWorld(), TEXT("stat rhi"));

      // Disable Vsync and framerate smoothing
      GEngine->Exec(GetWorld(), TEXT("r.vsync 0"));
      GEngine->Exec(GetWorld(), TEXT("t.maxfps 120"));
      GEngine->bForceDisableFrameRateSmoothing = true;

      // Unbuilt shadow previews are expensive, let's not draw them?
      GEngine->Exec(GetWorld(), TEXT("r.Shadow.UnbuiltPreviewInGame 0"));

      // Turn off dynamic resolution so we don't just downscale
      GEngine->Exec(GetWorld(), TEXT("r.DynamicRes.OperationMode 0"));

      _SetState(EPerformanceTestState::InitPawn);
   }

   // set initial scalability level
   _SetScalabilityLevelIndex(_currentScalabilityLevelIndex);
}

void UOSEPerformanceTestComponent::_TickInitPawn()
{
   // Some spectator pawns can continuously override the view target, clobbering the
   // view target changes by the perf test itself. Just destroying the spectator
   // appears to be enough to make this work without obvious errors
   GetWorld()->DestroyActor(_GetOwnerController().GetSpectatorPawn());
   _SetState(EPerformanceTestState::WaitForSettle);
}

void UOSEPerformanceTestComponent::_TickWaitForSettle(float deltaTime)
{
   check(_state == EPerformanceTestState::WaitForSettle);

   // wait a bit for the game to settle down once we change all the graphics settings, and for a moment after loading the level.
   // the engine continues loading things in and we should wait for that to finish up before moving on
   _settleWaitSeconds += deltaTime;
   if (_settleWaitSeconds >= _settleWaitSecondsTotal)
   {
      _settleWaitSeconds = 0.0f;
      _SetState(EPerformanceTestState::Running);
   }
}

void UOSEPerformanceTestComponent::_TickWaitForScalabilityToSettle(float deltaTime)
{
   check(_state == EPerformanceTestState::WaitForScalabilityToSettle);

   // wait a bit for the game to settle down once we change all the scalability settings
   _settleWaitSeconds += deltaTime;
   if (_settleWaitSeconds >= _settleWaitForScalabilitySecondsTotal)
   {
      _settleWaitSeconds = 0.0f;
      _SetState(EPerformanceTestState::Running);
   }
}

void UOSEPerformanceTestComponent::_TickPerformanceTest(float deltaTime)
{  
   check(_state == EPerformanceTestState::Running);

   // we should have stats by now, but if not for some reason, wait till we do
   const FStatUnitData* statUnitData = _GetStatUnitData();
   if (!statUnitData)
      return;

   AOSEPerformanceTestCameraActor* currentCamera = _perfCameras.IsValidIndex(_currentCameraIndex) ? _perfCameras[_currentCameraIndex] : nullptr;
   bool readyForNextCamera = true;

   // gather stats for the current camera
   if (currentCamera)
   {
      FPerformanceTestCameraData& data = _GetPerfData(currentCamera, _currentScalabilityLevelIndex);
      data.NumFrames++;
      data.TotalTime += deltaTime;

      // total
      data.TotalFrameTime += statUnitData->RawFrameTime;
      data.TotalRenderThreadTime += statUnitData->RawRenderThreadTime;
      data.TotalRenderThreadTimeCriticalPath += statUnitData->RawRenderThreadTimeCriticalPath;
      data.TotalGameThreadTime += statUnitData->RawGameThreadTime;
      data.TotalRhiTime += statUnitData->RawRHITTime;
      data.TotalPrimitivesDrawn += GNumPrimitivesDrawnRHI[0];
      data.TotalDrawCalls += GNumDrawCallsRHI[0];
      for (int gpuIdx = 0; gpuIdx < MAX_NUM_GPUS; ++gpuIdx)
      {
         data.TotalGPUFrameTime[gpuIdx] += statUnitData->RawGPUFrameTime[gpuIdx];
      }

      // max
      data.MaxFrameTime = FMath::Max(data.MaxFrameTime, statUnitData->RawFrameTime);
      data.MaxRenderThreadTime = FMath::Max(data.MaxRenderThreadTime, statUnitData->RawRenderThreadTime);
      data.MaxRenderThreadTimeCriticalPath = FMath::Max(data.MaxRenderThreadTimeCriticalPath, statUnitData->RawRenderThreadTimeCriticalPath);
      data.MaxGameThreadTime = FMath::Max(data.MaxGameThreadTime, statUnitData->RawGameThreadTime);
      data.MaxRhiTime = FMath::Max(data.MaxRhiTime, statUnitData->RawRHITTime);
      data.MaxPrimitivesDrawn = FMath::Max(data.MaxPrimitivesDrawn, GNumPrimitivesDrawnRHI[0]);
      data.MaxDrawCalls = FMath::Max(data.MaxDrawCalls, GNumDrawCallsRHI[0]);
      for (int gpuIdx = 0; gpuIdx < MAX_NUM_GPUS; ++gpuIdx)
      {
         data.MaxGPUFrameTime[gpuIdx] = FMath::Max(data.MaxGPUFrameTime[gpuIdx], statUnitData->RawGPUFrameTime[gpuIdx]);
      }

#if STATS
      // pull some data from the stats thread
      TMap<FName, FStatMessage> stats;

      // on average, this will run once per tick, but in practice it can run ~0-3 times
      while (_statTracker.Queue.Dequeue(stats))
      {
         // these should only be null when the game is exiting.
         FStatMessage* slateRender = stats.Find("STAT_SlateRenderingRTTime");

         float slateRenderTime = slateRender ? FPlatformTime::ToMilliseconds(slateRender->GetValue_Duration()) : 0.0f;

         // total
         data.TotalSlateRenderTime += slateRenderTime;

         // max
         data.MaxSlateRenderTime = FMath::Max(data.MaxSlateRenderTime, slateRenderTime);
      }
#endif // STATS

      if (data.TotalTime < _timePerCamera)
      {
         // stay in this camera till we have enough data
         readyForNextCamera = false;
      }
      else
      {
         // finalize the stats for this camera

         check(data.NumFrames > 0);

         data.AvgFrameTime = data.TotalFrameTime / data.NumFrames;
         data.AvgRenderThreadTime = data.TotalRenderThreadTime / data.NumFrames;
         data.AvgRenderThreadTimeCriticalPath = data.TotalRenderThreadTimeCriticalPath / data.NumFrames;
         data.AvgGameThreadTime = data.TotalGameThreadTime / data.NumFrames;
         data.AvgRhiTime = data.TotalRhiTime / data.NumFrames;
         for (int gpuIdx = 0; gpuIdx < MAX_NUM_GPUS; ++gpuIdx)
         {
            data.AvgGPUFrameTime[gpuIdx] = data.TotalGPUFrameTime[gpuIdx] / data.NumFrames;
         }
         data.AvgSlateRenderTime = data.TotalSlateRenderTime / data.NumFrames;
         data.AvgPrimitivesDrawn = (int64)((double)data.TotalPrimitivesDrawn / data.NumFrames);
         data.AvgDrawCalls = (int64)((double)data.TotalDrawCalls / data.NumFrames);
         data.AvgFPS = 1.0f / (data.TotalTime / (float)data.NumFrames);

         // this is an expensive call so we only make it once at the end of the camera measurement instead of each frame
         FPlatformMemoryStats memStats = FPlatformMemory::GetStats();
         data.MaxPhysicalMemMB = float(memStats.UsedPhysical) / (1024.0f * 1024.0f);
         data.MaxVirtualMemMB  = float(memStats.UsedVirtual)  / (1024.0f * 1024.0f);
         
         const PerformanceTestHelpers::FAICount aiCount = PerformanceTestHelpers::CountAI(GetWorld(), _timePerCamera * 0.9f);
         data.NumAI = aiCount.NumAI;
         data.NumAIRendered = aiCount.NumAIRendered;
      }
   }

   // next camera
   if (readyForNextCamera)
   {
      if (currentCamera)
      {
         currentCamera->StopTesting();
      }

      int nextCameraIdx = _currentCameraIndex + 1;
      if (_perfCameras.IsValidIndex(nextCameraIdx))
      {
         // new camera!
         _SetCameraIndex(nextCameraIdx);
      }
      else if (_currentScalabilityLevelIndex < _supportedScalabilityLevels.Num() - 1)
      {
         // new scalability level!
         _SetScalabilityLevelIndex(_currentScalabilityLevelIndex + 1);

         _currentCameraIndex = INDEX_NONE;

         // wait for it to settle first, then change camera spots again
         _SetState(EPerformanceTestState::WaitForScalabilityToSettle);
      }
      else
      {
         // we're done, write logs out!
         _currentCameraIndex = INDEX_NONE;
         _SetState(EPerformanceTestState::WritingLogs);
         _WriteLogs();
      }
   }
}

#define WRITE_WORST_VALUE_SUMMARY(Category, VarName) \
{ \
   float worstValue = 0.0f; \
   FString worstName; \
   int worstScalability = 0; \
   for (const FPerformanceTestCameraData& data : allResults) \
   { \
      if (data.VarName > worstValue) \
      { \
         worstValue = data.VarName; \
         worstScalability = data.ScalabilityLevel; \
         worstName = data.Name; \
      } \
   } \
   summaryRows.Add({ TEXT(#Category), worstName, FString::Printf(TEXT("%d"), worstScalability), FString::Printf(TEXT("%.02f"), worstValue) }); \
}

void UOSEPerformanceTestComponent::_WriteLogs()
{
   check(_perfCameras.Num() > 0);

   // write a csv version for dead simple excel/sheets importing

   // all our results
   TArray<FPerformanceTestCameraData> allResults;
   for(auto it = _perfCameraData.CreateConstIterator(); it; ++it)
   {
      const TArray<FPerformanceTestCameraData>& data = it.Value();
      allResults.Append(data);
   }
   // sorted from worst => best based on avg frame time
   // TODO: do we want to keep the same cameras together grouped up?  might be more readable that way?
   allResults.Sort([&](const FPerformanceTestCameraData& lhs, const FPerformanceTestCameraData& rhs)
      {
         return lhs.AvgFrameTime > rhs.AvgFrameTime;
      }
   );

   //
   // summary
   //
   UGameUserSettings* gameUserSettings = UGameUserSettings::GetGameUserSettings();
   check(gameUserSettings);
   FIntPoint resolution = gameUserSettings->GetScreenResolution();

   TArray<FString> summaryHeader = { TEXT("summary:") };
   TArray<TArray<FString>> summaryRows;

   summaryRows.Add({ TEXT("map"), GetPerformanceTestMap() });
#if !UE_BUILD_SHIPPING
   const bool isLightingBuilt = GetWorld()->NumLightingUnbuiltObjects > 0;
   summaryRows.Add({ TEXT("lighting_built"), isLightingBuilt ? TEXT("yes") : TEXT("no") });
#endif
   summaryRows.Add({ TEXT("cpu"), FPlatformMisc::GetCPUBrand() });
   summaryRows.Add({ TEXT("num_cores"), FString::Printf(TEXT("%d"), FPlatformMisc::NumberOfCores()) });
   summaryRows.Add({ TEXT("gpu"), FPlatformMisc::GetPrimaryGPUBrand() });
   summaryRows.Add({ TEXT("resolution"), FString::Printf(TEXT("%dx%d"), resolution.X, resolution.Y) });
   summaryRows.Add({ TEXT("physical_ram"), FString::Printf(TEXT("%d"), FPlatformMemory::GetPhysicalGBRam()) });
   WRITE_WORST_VALUE_SUMMARY(worst_frame_time,  AvgFrameTime);
   WRITE_WORST_VALUE_SUMMARY(worst_game_time,   AvgGameThreadTime);
   WRITE_WORST_VALUE_SUMMARY(worst_gpu_time,    AvgGPUFrameTime[0]);
   WRITE_WORST_VALUE_SUMMARY(worst_phys_mem,    MaxPhysicalMemMB);
   WRITE_WORST_VALUE_SUMMARY(worst_virtual_mem, MaxVirtualMemMB);

   // a couple empty rows after the summary ends...
   static const int kNumBufferRows = 3;
   for(int rowIdx = 0; rowIdx < kNumBufferRows; ++rowIdx)
   {
      summaryRows.Add({});
   }

   //
   // individual camera stats as a table
   //

   TArray<FString> statsHeader = FPerformanceTestCameraData::GetHeaderStrArray();

   TArray<TArray<FString>> statsRows;
   for(const FPerformanceTestCameraData& result : allResults)
   {
      statsRows.Add(result.ToStrArray());
   }

   //
   // write all the arrays out in order
   //

   // make a big ole string to write out
   FString csvStr;

   // write header
   _WriteStrArrayToCsvLineStr(summaryHeader, csvStr);
   for (const TArray<FString>& row : summaryRows)
   {
      _WriteStrArrayToCsvLineStr(row, csvStr);
   }

   // write cams
   _WriteStrArrayToCsvLineStr(statsHeader, csvStr);
   for(const TArray<FString>& row : statsRows)
   {
      _WriteStrArrayToCsvLineStr(row, csvStr);
   }

   // file name
   FDateTime now = FDateTime::Now();
   FString fileName = FString::Printf(TEXT("perf_%s_%d_%d-%d-%d_%d-%d-%d.csv"), 
      *GetPerformanceTestMap(),
      FEngineVersion::Current().GetChangelist(),
      now.GetMonth(),
      now.GetDay(),
      now.GetYear(),
      now.GetHour(),
      now.GetMinute(),
      now.GetSecond());

   // file path
   FString filePath = FPaths::Combine(FPlatformMisc::ProjectDir(), TEXT("Saved"), TEXT("PerformanceTest"), fileName);

   // write
   FFileHelper::SaveStringToFile(csvStr, *filePath);

   UE_LOG(LogOSEPerformanceTest, Log, TEXT("Wrote perf test file to %s"), *filePath);

   // TODO: Also write a json file?  Might be useful?

   // done!
   _SetState(EPerformanceTestState::Complete);
   _ExitApp();
}

void UOSEPerformanceTestComponent::_WriteStrArrayToCsvLineStr(const TArray<FString>& strArr, FString& outCvsLineStr)
{
   const int strArrayNum = strArr.Num();

   if (strArr.Num() == 0)
   {
      outCvsLineStr.Append(TEXT("\n"));
   }
   else
   {
      for (int idx = 0; idx < strArrayNum; ++idx)
      {
         const FString& strEntry = strArr[idx];
         if (idx == 0)
         {
            // line start
            outCvsLineStr.Append(strEntry);
         }
         else
         {
            // line progression
            outCvsLineStr.Append(FString::Printf(TEXT(",%s"), *strEntry));
         }

         // line end
         if (idx == strArrayNum - 1)
         {
            outCvsLineStr.Append(TEXT("\n"));
         }
      }
   }
}

void UOSEPerformanceTestComponent::_ExitApp()
{
   FGenericPlatformMisc::RequestExit(false);
}

APlayerController& UOSEPerformanceTestComponent::_GetOwnerController() const
{
   return *Cast<APlayerController>(GetOwner());
}

const FStatUnitData* UOSEPerformanceTestComponent::_GetStatUnitData() const
{
   UGameViewportClient* viewportClient = GetWorld()->GetGameViewport();
   return viewportClient ? viewportClient->GetStatUnitData() : nullptr;
}

TArray<ULevelStreamingDynamic*> UOSEPerformanceTestComponent::_GetPerfLevels() const
{
   TArray<ULevelStreamingDynamic*> perfLevels;
   for (ULevelStreaming* level : GetWorld()->GetStreamingLevels())
   {
      // If we have a streaming level equal to our scenario map name, set to loaded and visible on the server.
      // This will be replicated down to clients so they load it as well.
      if (ULevelStreamingDynamic* dynamicLevel = Cast<ULevelStreamingDynamic>(level))
      {
         FString mapName = dynamicLevel->GetWorldAsset().GetAssetName();

         // ASSUMPTION: perf maps will contain _Perf
         if (mapName.Contains("_Perf") || mapName.Contains("_perf"))
         {
            perfLevels.Add(dynamicLevel);
         }
      }
   }
   return perfLevels;
}

TArray<const UDataLayerInstance*> UOSEPerformanceTestComponent::_GetPerfDataLayers() const
{
   TArray<const UDataLayerInstance*> result;

   // This probably could have used a fixed name, since it could, unlike sublevels, but :shrug:
   if (AWorldDataLayers* dataLayers = GetWorld()->GetWorldDataLayers())
   {
      dataLayers->ForEachDataLayerInstance([&result](const UDataLayerInstance* dataLayer)
         {
            // ASSUMPTION: perf data layers will start with Perf
            if (dataLayer->IsRuntime() && dataLayer->GetDataLayerShortName().StartsWith(TEXT("Perf")))
            {
               result.Add(dataLayer);
            }
            return true;
         });
   }

   return result;
}

FPerformanceTestCameraData& UOSEPerformanceTestComponent::_GetPerfData(AOSEPerformanceTestCameraActor* camera, int scalabilityLevelIndex)
{
   TArray<FPerformanceTestCameraData>& dataArray = _perfCameraData.FindChecked(camera);
   check(dataArray.IsValidIndex(scalabilityLevelIndex));
   return dataArray[scalabilityLevelIndex];
}

void UOSEPerformanceTestComponent::_SetScalabilityLevelIndex(int scalabilityLevelIndex)
{
   check(_supportedScalabilityLevels.IsValidIndex(scalabilityLevelIndex));
   _currentScalabilityLevelIndex = scalabilityLevelIndex;

   int scalabilityLevel = _supportedScalabilityLevels[_currentScalabilityLevelIndex];
   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   check(settings);
   settings->SetOverallScalabilityLevel(scalabilityLevel);
   settings->ApplyNonResolutionSettings();
   UE_LOG(LogOSEPerformanceTest, Log, TEXT("Scalability level set to %d"), scalabilityLevel);
}

void UOSEPerformanceTestComponent::_SetCameraIndex(int cameraIndex)
{
   check(_perfCameras.IsValidIndex(cameraIndex));

   _currentCameraIndex = cameraIndex;

   AOSEPerformanceTestCameraActor* newCamera = _perfCameras[_currentCameraIndex];
   check(newCamera);

   newCamera->StartTesting();

   // set the new view target
   _GetOwnerController().SetViewTarget(newCamera);
}

void UOSEPerformanceTestComponent::_SetState(EPerformanceTestState newState)
{
   _state = newState;
   UE_LOG(LogOSEPerformanceTest, Log, TEXT("State = %s"), *UEnum::GetValueAsString(newState));
}

