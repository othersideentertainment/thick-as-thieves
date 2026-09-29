// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherManager.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherPreset.h"
#include "Environment/TATWeatherPresetDedicatedServer.h"
#include "Audio/TATAudioRoomComponent.h"
#include "Environment/TATWeatherSubsystem.h"

// ose
#include "OSECoreCheats.h"
#include "UI/OSERadialPaintLibrary.h"

// wwise
#include "AkAudioEvent.h"

// ue
#include "GameFramework/PlayerController.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/LevelStreaming.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherManager)
DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherManager, Log, All);

DECLARE_CYCLE_STAT(TEXT("Weather Manager: Tick (Total)"), STAT_WeatherManagerTick, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Manager: Tick (Camera Inside Trace)"), STAT_WeatherManagerTick_CameraInsideTrace, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Manager: Tick (Temporal Environment Traces)"), STAT_WeatherManagerTick_TemporalEnvTraces, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Manager: Level Depthmap Update"), STAT_WeatherManager_UpdateSceneDepthTexture, STATGROUP_Weather);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Weather Manager: Num Depthmap Updates"), STAT_WeatherManager_UpdateSceneDepthTexture_Calls, STATGROUP_Weather);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Weather Manager: Temporal Environment Traces/Second"), STAT_WeatherManager_TemporalEnvironmentTraceCountPerSecond, STATGROUP_Weather);

ATATWeatherManager::ATATWeatherManager()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeatherManagerRootComponent"));

#if WITH_EDITORONLY_DATA
   // Structure to hold one-time initialization
   struct FConstructorStatics
   {
      ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTexture = TEXT("/Game/Editor/ClassIcons/T_TATWeatherManagerIcon");
   };
   static FConstructorStatics sConstructorStatics;
   WeatherManagerIconComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("WeatherManagerIconComponent"));
   WeatherManagerIconComponent->SetupAttachment(RootComponent);
   WeatherManagerIconComponent->SetSprite(sConstructorStatics.IconTexture.Get());
   WeatherManagerIconComponent->bHiddenInGame = true;
#endif

   WindDirectionSourceComponent = CreateDefaultSubobject<UWindDirectionalSourceComponent>(TEXT("WindDirectionSource"));
   WindDirectionSourceComponent->SetupAttachment(RootComponent);
   WindDirectionSourceComponent->SetRelativeLocation(FVector(0, 0, -50.0f));
   WindDirectionSourceComponent->Strength = 0.0f;
   WindDirectionSourceComponent->Speed = 0.0f;

#if WITH_EDITORONLY_DATA
   WindDirectionArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("WindDirectionArrowComponent"));
   WindDirectionArrowComponent->SetupAttachment(WindDirectionSourceComponent);
   WindDirectionArrowComponent->ArrowColor = FColor::Cyan;
   WindDirectionArrowComponent->ArrowLength = 200.0f;
   WindDirectionArrowComponent->bHiddenInGame = true;
   WindDirectionArrowComponent->bHiddenInSceneCapture = true;
#endif

   WeatherBoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("WeatherBoundsComponent"));
   WeatherBoundsComponent->SetupAttachment(RootComponent);
   WeatherBoundsComponent->SetBoxExtent(FVector(5000, 5000, 5000));
   WeatherBoundsComponent->ShapeColor = FColor::Cyan;
   WeatherBoundsComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   WeatherBoundsComponent->SetGenerateOverlapEvents(false);

   struct FConstructorAudioStatics
   {
      ConstructorHelpers::FObjectFinderOptional<UAkAudioEvent> DefaultWeatherAudio = TEXT("/Game/Audio/Events/Weather/Play_Amb_Weather_Ext");
   };
   static FConstructorAudioStatics sConstructorAudioStatics;

   WorldAudioRoomComponent = CreateDefaultSubobject<UTATAudioRoomComponent>(TEXT("WorldAudioRoomComponent"));
   WorldAudioRoomComponent->SetupAttachment(WeatherBoundsComponent);
   WorldAudioRoomComponent->UseForNoiseStimPropagation = false;
   WorldAudioRoomComponent->bDynamic = false;
   WorldAudioRoomComponent->Priority = -1.0f;
   WorldAudioRoomComponent->WallOcclusion = 0.0f;
   WorldAudioRoomComponent->AkAudioEvent = sConstructorAudioStatics.DefaultWeatherAudio.Get();

   LevelDepthSceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("LevelDepthSceneCaptureComponent"));
   LevelDepthSceneCaptureComponent->SetupAttachment(RootComponent);
   LevelDepthSceneCaptureComponent->SetRelativeLocation(FVector(0, 0, 50));
   LevelDepthSceneCaptureComponent->SetRelativeRotation(FRotator(0, -90, 0));
   LevelDepthSceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
   LevelDepthSceneCaptureComponent->OrthoWidth = 32000.0f;
   LevelDepthSceneCaptureComponent->CaptureSource = SCS_SceneDepth;
   LevelDepthSceneCaptureComponent->bCaptureEveryFrame = false;
   LevelDepthSceneCaptureComponent->bAlwaysPersistRenderingState = true;

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   PrimaryActorTick.bAllowTickOnDedicatedServer = false;

   _sceneDepthRenderWorldRotation = FRotationMatrix::MakeFromXZ(FVector(0, 0, -1), FVector(0, -1, 0)).Rotator();
}

// static
ATATWeatherManager* ATATWeatherManager::Get(const UObject* contextObject)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
      {
         return worldSettings->WeatherManager;
      }
   }
   return nullptr;
}

void ATATWeatherManager::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   if (_sceneDepthRenderTarget == nullptr)
   {
      _sceneDepthRenderTarget = UTATWeatherSettings::Get().DepthmapRenderTarget.LoadSynchronous();
   }

   // Assign the scene depth render target to the scene capture component if we have one
   if (LevelDepthSceneCaptureComponent != nullptr && _sceneDepthRenderTarget != nullptr)
   {
      LevelDepthSceneCaptureComponent->TextureTarget = _sceneDepthRenderTarget;
   }
}

void ATATWeatherManager::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);

#if WITH_EDITORONLY_DATA
   WindDirectionArrowComponent->SetHiddenInGame(!ShowWeatherVaneInGame);
#endif
}

void ATATWeatherManager::BeginPlay()
{
   Super::BeginPlay();
   if (GetWorld()->IsNetMode(NM_DedicatedServer))
   {
      LoadWeatherPresetTypeForDedicatedServer();
      return;
   }

   // Reset the depthmap update count on BeginPlay - we only care about the number of updates done during a single match
   SET_DWORD_STAT(STAT_WeatherManager_UpdateSceneDepthTexture_Calls, 0);

   for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
   {
      APlayerController* pc = it->Get();
      if (pc != nullptr && pc->IsLocalPlayerController())
      {
         _localPlayerController = pc;
         break;
      }
   }

   //TODO: Only enable tick if weather-related postprocess effects are enabled
   SetActorTickEnabled(true);

   // Poll for level streaming changes so we can rebuild the scene depth texture when sublevels are loaded, unloaded, or have their visible state changed
   FTimerHandle streamingLevelPollTimerHandle;
   constexpr float streamingLevelPollInterval = 1.0f;
   constexpr float streamingLevelPollFirstDelayTime = 0.25f;
   constexpr bool streamingLevelPollLoop = true;
   GetWorldTimerManager().SetTimer(streamingLevelPollTimerHandle, this, &ATATWeatherManager::_PollForStreamingLevelChanges,
      streamingLevelPollInterval, streamingLevelPollLoop, streamingLevelPollFirstDelayTime);
   // Reserve enough space for more streaming levels than we'll ever have to avoid allocations while polling
   _currentStreamingLevels.Reserve(256);

   // Set the max number of temporal traces we ever want to store to avoid repeated allocations in tick
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();
   const int32 targetNumTemporalTracesPerSecond = weatherSettings.TemporalEnvironmentTraceFrequency * weatherSettings.TemporalEnvironmentTraceTargetFramerate;
   _temporalEnvironmentTraceData.SetNum(FMath::Max(0, FMath::FloorToInt32(targetNumTemporalTracesPerSecond * weatherSettings.TemporalEnvironmentTraceTimeWindowSeconds)));

   TSoftClassPtr<ATATWeatherPreset> presetClassSoft = UTATWeatherSettings::GetCurrentWeatherPresetClass(this);
   if (!presetClassSoft.IsNull())
   {
      constexpr bool forceUpdate = true;
      UpdateSceneDepthTexture(forceUpdate);

      // In theory we could easily async-load this, but that would result in a jarring visual hitch.
      // Better to do a blocking load and spawn it immediately.
      SetWeatherPresetType(presetClassSoft.LoadSynchronous());
   }
}

void ATATWeatherManager::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);
#if !WITH_EDITOR
   if (_localPlayerController == nullptr)
   {
      return;
   }
#endif

   SCOPE_CYCLE_COUNTER(STAT_WeatherManagerTick);

   FVector cameraViewpoint = FVector::ZeroVector;
   {
      SCOPE_CYCLE_COUNTER(STAT_WeatherManagerTick_CameraInsideTrace);

      const bool firstViewpointTrace = !_cameraViewpointStateValid;

      //TODO: Only run this on tick if we actually need it (eg. using the rain postprocess effect)
      const bool prevCameraInside = _cameraViewpointIsInside;

      if (GetCameraViewpoint(cameraViewpoint))
      {
         _cameraViewpointIsInside = UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(this, cameraViewpoint);
      }
      else
      {
         _cameraViewpointIsInside = false;
      }

      _cameraViewpointStateValid = true;

      if (firstViewpointTrace || prevCameraInside != _cameraViewpointIsInside)
      {
         OnCameraViewportIndoorStateChange(_cameraViewpointIsInside, cameraViewpoint);

         OnCameraIndoorStateChange.Broadcast(_cameraViewpointIsInside, cameraViewpoint);

         if (_weatherPreset != nullptr)
         {
            _weatherPreset->OnCameraViewportIndoorStateChange(_cameraViewpointIsInside, cameraViewpoint);
         }
      }
   }

   _TickTemporalTraces(cameraViewpoint);
}

void ATATWeatherManager::RefreshLevelDepthmap()
{
   constexpr bool forceUpdate = true;
   UpdateSceneDepthTexture(forceUpdate);
}

bool ATATWeatherManager::IsRainEnabled() const
{
   return (_weatherPreset != nullptr) ? _weatherPreset->IsRainEnabled() : false;
}

bool ATATWeatherManager::IsWindEnabled() const
{
   return (_weatherPreset != nullptr) ? _weatherPreset->IsWindEnabled() : false;
}

bool ATATWeatherManager::GetCameraViewpoint(FVector& outViewLocation) const
{
   if (_localPlayerController != nullptr)
   {
      FRotator playerViewRotation;
      _localPlayerController->GetActorEyesViewPoint(outViewLocation, playerViewRotation);
      return true;
   }

#if WITH_EDITOR
   // One way to get the editor camera
   if (!GIsPlayInEditorWorld)
   {
      if (GEditor && GEditor->GetActiveViewport())
      {
         if (FEditorViewportClient* viewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient()))
         {
            outViewLocation = viewportClient->GetViewLocation();
            return true;
         }
      }

      // Another approach in case the first one fails
      if (UWorld* world = GetWorld())
      {
         const TArray<FVector>& viewLocations = world->ViewLocationsRenderedLastFrame;
         if (viewLocations.Num() > 0)
         {
            outViewLocation = viewLocations[0];
            return true;
         }
      }
   }
#endif

   outViewLocation = FVector::ZeroVector;
   return false;
}

void ATATWeatherManager::GetWindDirectionAndStrength(FVector& windDirection, float& windStrength) const
{
   windDirection = _windDirection;
   windStrength = _windStrength;
}

FBox ATATWeatherManager::GetWeatherBoundingBox() const
{
   check(WeatherBoundsComponent != nullptr);
   const FVector baseLocation = WeatherBoundsComponent->GetComponentLocation();
   const FVector boxExtent = WeatherBoundsComponent->GetScaledBoxExtent();
   const FBox box{ baseLocation - boxExtent, baseLocation + boxExtent };
   return box;
}

void ATATWeatherManager::UpdateWindDirectionAndStrengthFromPreset(ATATWeatherPreset* preset, const FVector& newWindDirection, float newWindStrength)
{
   _windDirection = newWindDirection;
   _windStrength = newWindStrength;

   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      return;
   }

   // update the wind direction source
   WindDirectionSourceComponent->SetWorldRotation(FRotationMatrix::MakeFromX(_windDirection).ToQuat());

   //TODO: How exactly does the wind direction source component define strength vs speed? Can't seem to find any documentation on this.
   //TODO: Do we actually need a wind direction source component? They seem exist just for SpeedTree and I don't think we use that.
   WindDirectionSourceComponent->SetSpeed(newWindStrength);
   WindDirectionSourceComponent->SetStrength(0.1f);

#if WITH_EDITORONLY_DATA
   if (ShowWeatherVaneInGame || (GEditor != nullptr && !GEditor->IsPlayingSessionInEditor()))
   {
      const float baseArrowLength = GetDefault<ATATWeatherManager>(GetClass())->WindDirectionArrowComponent->ArrowLength;
      const FFloatInterval arrowLengthRange{ baseArrowLength * 0.75f, baseArrowLength * 1.25f };
      static const FFloatInterval arrowScaleRange{ 1.0f, 1.5f };

      if (_windStrength > 0)
      {
         // Set arrow scale to a constant value based on the base (configured) wind strength
         const float baseWindStrength = (preset != nullptr && preset->EnableWind)
            ? UTATWeatherUtilities::NormalizeWindStrength(preset->WindStrength)
            : 0.0f;
         WindDirectionArrowComponent->SetWorldScale3D(FVector(FMath::Lerp(arrowScaleRange.Min, arrowScaleRange.Max, baseWindStrength)));

         // Set arrow length to a variable value based on the current wind strength, which fluctuates with time
         const float normalizedWindStrength = UTATWeatherUtilities::NormalizeWindStrength(_windStrength);
         WindDirectionArrowComponent->SetArrowLength(FMath::Lerp(arrowLengthRange.Min, arrowLengthRange.Max, normalizedWindStrength));

         // Set arrow color to cool or warm depending on the current wind strength
         static const FColor coolColor{ 154, 202, 244 };
         static const FColor warmColor{ 242, 114, 105 };
         WindDirectionArrowComponent->SetArrowColor(FLinearColor::LerpUsingHSV(FLinearColor(coolColor), FLinearColor(warmColor), normalizedWindStrength));
      }
      else
      {
         // Wind disabled? Set a small constant size and color
         WindDirectionArrowComponent->SetArrowLength(arrowLengthRange.Min);
         WindDirectionArrowComponent->SetWorldScale3D(FVector(arrowScaleRange.Min));
         WindDirectionArrowComponent->SetArrowColor(FLinearColor::Gray);
      }
   }
#endif
}

void ATATWeatherManager::SetWeatherPresetType(TSubclassOf<ATATWeatherPreset> presetType)
{
   if (_weatherPreset != nullptr)
   {
      OnWeatherPresetAboutToBeDestroyed(_weatherPreset);

      _weatherPreset->Destroy();
      _weatherPreset = nullptr;
   }

   if (presetType != nullptr)
   {
      FActorSpawnParameters spawnParams{};
      spawnParams.Name = FName(FString::Printf(TEXT("PRESET_%s"), *presetType->GetName()));
      spawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
      spawnParams.Owner = this;
      spawnParams.ObjectFlags = RF_Transient | RF_DuplicateTransient;
      spawnParams.bNoFail = true;
      _weatherPreset = GetWorld()->SpawnActor<ATATWeatherPreset>(presetType, spawnParams);
      check(_weatherPreset != nullptr);

      OnWeatherPresetCreated(_weatherPreset);

      // If we know if the camera viewpoint is inside or outside, call the event on the preset for init purposes
      if (_cameraViewpointStateValid)
      {
         FVector cameraViewpoint = FVector::ZeroVector;
         const bool viewpointSuccess = GetCameraViewpoint(cameraViewpoint);
         ensure(viewpointSuccess); // _cameraViewpointStateValid should not be true if we can't get a camera viewpoint
         _weatherPreset->OnCameraViewportIndoorStateChange(_cameraViewpointIsInside, cameraViewpoint);
      }

      // Make sure the scene depthmap is up to date.
      // Even if it was already up to date, this will pass the depthmap params to the preset actor.
      constexpr bool forceUpdate = false;
      UpdateSceneDepthTextureInternal(forceUpdate, _weatherPreset);
   }
}

void ATATWeatherManager::LoadWeatherPresetTypeForDedicatedServer()
{
   TSoftClassPtr<ATATWeatherPreset> presetClassSoft = UTATWeatherSettings::GetCurrentWeatherPresetClass(this);
   if (presetClassSoft.IsNull())
   {
      UE_LOG(LogTATWeatherManager, Error, TEXT("Found null class for weather preset, failed to load dedicated server weather preset!"));
      return;
   }

   FActorSpawnParameters spawnParams;
   spawnParams.Name = FName(FString::Printf(TEXT("PRESET_%s_DEDICATEDSERVER"), *presetClassSoft.GetAssetName()));
   spawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
   spawnParams.Owner = this;
   spawnParams.ObjectFlags = RF_Transient | RF_DuplicateTransient;
   spawnParams.bNoFail = true;
   _weatherPresetDedicatedServer = GetWorld()->SpawnActor<ATATWeatherPresetDedicatedServer>(spawnParams);
}

// static
void ATATWeatherManager::CalcDepthmapParameters(const FBox& weatherWorldBoundingBox, float& depthmapWorldSize, float& depthmapWorldHeight, FVector& depthmapWorldOrigin)
{
   const FVector boundsSize = weatherWorldBoundingBox.GetSize();

   // Size of the world in X and Y that the depthmap texture represents
   depthmapWorldSize = FMath::Max(boundsSize.X, boundsSize.Y);

   // World height that the depthmap texture represents
   depthmapWorldHeight = boundsSize.Z;

   // World origin of the space that the depthmap represents
   depthmapWorldOrigin = weatherWorldBoundingBox.GetCenter();
   depthmapWorldOrigin.X -= depthmapWorldSize * 0.5f;
   depthmapWorldOrigin.Y -= depthmapWorldSize * 0.5f;
   depthmapWorldOrigin.Z -= depthmapWorldHeight * 0.5f;
}

bool ATATWeatherManager::UpdateSceneDepthTexture(bool forceUpdate)
{
   return UpdateSceneDepthTextureInternal(forceUpdate, _weatherPreset);
}

bool ATATWeatherManager::UpdateSceneDepthTextureInternal(bool forceUpdate, ATATWeatherPreset* notifyWeatherPreset)
{
   if (_sceneDepthRenderTarget == nullptr && !UTATWeatherSettings::Get().DepthmapRenderTarget.IsNull())
   {
      _sceneDepthRenderTarget = UTATWeatherSettings::Get().DepthmapRenderTarget.LoadSynchronous();
   }

   if (_sceneDepthRenderTarget == nullptr)
   {
      return false;
   }

   bool updatedDepthmap = false;

   const FBox sceneDepthBounds = GetWeatherBoundingBox();

   // If we already have a valid scene depthmap, don't update unless forceUpdate is true
   if (forceUpdate || !_sceneDepthMapGenerated)
   {
      SCOPE_CYCLE_COUNTER(STAT_WeatherManager_UpdateSceneDepthTexture);

      const uint64 sceneCaptureStartTime = FPlatformTime::Cycles64();

      FVector sceneCaptureOrigin = sceneDepthBounds.GetCenter();
      sceneCaptureOrigin.Z = sceneDepthBounds.Max.Z;
      LevelDepthSceneCaptureComponent->SetWorldLocation(sceneCaptureOrigin);
      LevelDepthSceneCaptureComponent->SetWorldRotation(FRotationMatrix::MakeFromXZ(FVector(0, 0, -1), FVector(0, -1, 0)).Rotator());

      const FVector weatherExtent = sceneDepthBounds.GetExtent();
      LevelDepthSceneCaptureComponent->OrthoWidth = FMath::Max(weatherExtent.X, weatherExtent.Y) * 2.0f;

      LevelDepthSceneCaptureComponent->TextureTarget = _sceneDepthRenderTarget;
      LevelDepthSceneCaptureComponent->CaptureScene();

      _sceneDepthMapGenerated = true;
      updatedDepthmap = true;

      const uint64 sceneCaptureDuration = FPlatformTime::Cycles64() - sceneCaptureStartTime;
      const double sceneCaptureDurationSeconds = FPlatformTime::ToSeconds64(sceneCaptureDuration);
      UE_LOG(LogTATWeatherManager, Log, TEXT("UpdateSceneDepthTexture took %llu cycles (%.4f seconds)"), sceneCaptureDuration, sceneCaptureDurationSeconds);

      INC_DWORD_STAT(STAT_WeatherManager_UpdateSceneDepthTexture_Calls);
   }

   OnUpdateSceneDepthTexture.Broadcast(_sceneDepthRenderTarget, sceneDepthBounds, updatedDepthmap);

   if (notifyWeatherPreset != nullptr)
   {
      notifyWeatherPreset->OnSceneDepthTextureUpdated(_sceneDepthRenderTarget, sceneDepthBounds);
   }

   return updatedDepthmap;
}

void ATATWeatherManager::_PollForStreamingLevelChanges()
{
   for (ULevelStreaming* streamingLevel : GetWorld()->GetStreamingLevels())
   {
      if (streamingLevel == nullptr || _currentStreamingLevels.Contains(streamingLevel))
      {
         continue;
      }
      streamingLevel->OnLevelLoaded.AddDynamic(this, &ATATWeatherManager::_OnLevelStreamingChange);
      streamingLevel->OnLevelUnloaded.AddDynamic(this, &ATATWeatherManager::_OnLevelStreamingChange);
      streamingLevel->OnLevelHidden.AddDynamic(this, &ATATWeatherManager::_OnLevelStreamingChange);
      streamingLevel->OnLevelShown.AddDynamic(this, &ATATWeatherManager::_OnLevelStreamingChange);

      // already loaded? fire an event immediately
      if (streamingLevel->IsLevelLoaded())
      {
         _OnLevelStreamingChange();
      }

      _currentStreamingLevels.Add(streamingLevel);
   }
}

void ATATWeatherManager::_OnLevelStreamingChange()
{
   // Set or reset a timer when a level streaming change event is fired.
   // This allows the _PostLevelStreamingChange event to fire just once when we get a series of level streaming events all at once.
   constexpr float delayTimeSeconds = 0.5f;
   GetWorldTimerManager().SetTimer(_levelStreamingChangeTimer, this, &ATATWeatherManager::_PostLevelStreamingChange, delayTimeSeconds);
}

void ATATWeatherManager::_PostLevelStreamingChange()
{
   UE_LOG(LogTATWeatherManager, Log, TEXT("Detected streaming level change event, triggering a scene depth texture update"));
   constexpr bool forceUpdate = true;
   UpdateSceneDepthTexture(forceUpdate);
}

void ATATWeatherManager::_TickTemporalTraces(const FVector& cameraViewpoint)
{
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();

   if (weatherSettings.TemporalEnvironmentTraceFrequency <= 0
      || weatherSettings.TemporalEnvironmentTraceTimeWindowSeconds <= 0
      || weatherSettings.TemporalEnvironmentTraceRadius <= 0
      || _temporalEnvironmentTraceData.Num() <= 0)
   {
      return;
   }

   const float now = GetWorld()->GetTimeSeconds();

   static constexpr FLinearColor debugColorInside = FLinearColor(0.5f, 1.0f, 0.0f);
   static constexpr FLinearColor debugColorOutside = FLinearColor(0.0f, 0.1f, 1.0f);

#if OSE_CHEATS_ENABLED
   if (_temporalEnvTraceDebugging)
   {
      for (const FTATTemporalEnvironmentTraceEvent& evt : _temporalEnvironmentTraceData)
      {
         if (!evt.IsValid())
         {
            continue;
         }

         const float timeSinceTrace = evt.TimeSinceTrace(now);

         // 0.0 if it just happened, 1.0 if it's about to expire
         const float normalizedAge = FMath::Clamp(timeSinceTrace / weatherSettings.TemporalEnvironmentTraceTimeWindowSeconds, 0.0f, 1.0f);

         const FLinearColor& srcColor = evt.IsInside ? debugColorInside : debugColorOutside;
         const FLinearColor color{
            FMath::Lerp(srcColor.R, 0.0f, normalizedAge),
            FMath::Lerp(srcColor.G, 0.0f, normalizedAge),
            FMath::Lerp(srcColor.B, 0.0f, normalizedAge),
         };

         // in theory, setting this to -1 should make it draw for exactly one frame, but it doesn't seem to be working correctly (especially for DrawDebugString)
         const float debugDrawDuration = GetWorld()->GetDeltaSeconds() * 1.1f;
         constexpr bool persistent = false;

         DrawDebugPoint(GetWorld(), evt.WorldLocation, 16.0f, color.ToFColorSRGB(), persistent, debugDrawDuration);

         if (evt.OutsideTraceHitPhysicalMaterial)
         {
            // Don't bother cluttering up the screen with the name of the default physical material.
            // Theoretically we could check this against a reference to the actual default physical material instead of a string comparison, but for a simple debug draw this is fine.
            const FString name = evt.OutsideTraceHitPhysicalMaterial->GetName();
            if (name != TEXT("DefaultPhysicalMaterial"))
            {
               constexpr bool drawShadow = true;
               constexpr AActor* textBaseActor = nullptr;
               DrawDebugString(GetWorld(), evt.WorldLocation + FVector(0, 0, 10), name, textBaseActor, FColor::White, debugDrawDuration, drawShadow);
            }
         }
      }
   }
#endif

   const int32 targetNumTemporalTracesPerSecond = weatherSettings.TemporalEnvironmentTraceFrequency * weatherSettings.TemporalEnvironmentTraceTargetFramerate;
   const float maxTraceTimeInterval = 1.0f / static_cast<float>(targetNumTemporalTracesPerSecond);

   // We just did a temporal trace recently enough that we should skip some frames before the next one
   if (_lastTemporalEnvTraceTimeSeconds != 0 && (now - _lastTemporalEnvTraceTimeSeconds < maxTraceTimeInterval))
   {
      return;
   }

   _lastTemporalEnvTraceTimeSeconds = now;

   auto getRandomTemporalTraceLocation = [this](FRandomStream& rng, const FVector& start, const FFloatInterval& traceRadius) -> FVector
   {
      const float angleDegrees = rng.FRand() * 360.0f;
      // We use sqrt(rand()) here because the smaller the value, the smaller the circumference at that point in the circle, so using sqrt gives us an even distribution
      const float radius = FMath::Lerp(traceRadius.Min, traceRadius.Max, FMath::Sqrt(rng.FRand()));
      return FVector(UOSERadialPaintLibrary::FindPointOnCircle(FVector2D(start.X, start.Y), radius, angleDegrees), start.Z);
   };

   // If the camera is outside, we know the closest possible outdoor world location
   if (!_cameraViewpointIsInside)
   {
      _lastKnownOutdoorWorldLocation = cameraViewpoint;
   }

#if STATS
   int32 totalNumTracesThisFrame = 0;
   const uint64 temporalEnvTraceStartTime = FPlatformTime::Cycles64();

   // Reset our "traces/second" counter if it's been more than a second since the last reset
   const double platformTimeNow = FPlatformTime::ToSeconds64(temporalEnvTraceStartTime);
   if (_lastTemporalTraceEnvPerfCounterUpdateTime == 0 || platformTimeNow - _lastTemporalTraceEnvPerfCounterUpdateTime > 1.0)
   {
      SET_DWORD_STAT(STAT_WeatherManager_TemporalEnvironmentTraceCountPerSecond, _temporalTracesPerSecondPerfCounter);
      _temporalTracesPerSecondPerfCounter = 0;
      _lastTemporalTraceEnvPerfCounterUpdateTime = platformTimeNow;
   }
#endif

   SCOPE_CYCLE_COUNTER(STAT_WeatherManagerTick_TemporalEnvTraces);

   const ECollisionChannel collisionChannel = weatherSettings.TemporalEnvironmentTraceCollisionChannel;
   static const FName traceTag = FName("WeatherTemporalTrace");
   const bool traceComplex = weatherSettings.TemporalEnvironmentTraceComplex;
   FHitResult hitResult;
   for (int32 i = 0; i < weatherSettings.TemporalEnvironmentTraceFrequency; i++)
   {
      const FFloatInterval traceRadius{ weatherSettings.TemporalEnvironmentTraceRadius * 0.15f, weatherSettings.TemporalEnvironmentTraceRadius };
      FVector traceStart = getRandomTemporalTraceLocation(_temporalEnvTraceRandomStream, cameraViewpoint, traceRadius);

      if (_temporalEnvTraceNextIndex >= _temporalEnvironmentTraceData.Num())
      {
         _temporalEnvTraceNextIndex = 0;
      }

      const int32 traceIndex = _temporalEnvTraceNextIndex;
      _temporalEnvTraceNextIndex++;
      check(_temporalEnvironmentTraceData.IsValidIndex(traceIndex));

      // Determine exactly what nearby world location to check for inside/outside state.
      // If the camera viewpoint is outside, add a random height offset to the test location.
      if (!_cameraViewpointIsInside)
      {
         // Our starting point is going to be at the same Z height as the players eyes, but we need to go a bit higher than this to account for world geometry.
         // As an example, if a player is standing next to a truck, this point may be inside that geometry. If we move the point on top of the truck, we get a
         // correct "outside" value and can then trace down to get the truck's physical material. The problem is, we don't know the actual dimensions of that truck
         // and don't want to do extra traces to find out, so we'll just add a random factor to the height offset.
         traceStart.Z += _temporalEnvTraceRandomStream.FRandRange(
            weatherSettings.TemporalEnvironmentTraceOutsideVerticalOffset.Min,
            weatherSettings.TemporalEnvironmentTraceOutsideVerticalOffset.Max);
      }

      FTATTemporalEnvironmentTraceEvent& traceEvent = _temporalEnvironmentTraceData[traceIndex];
      traceEvent.Reset();
      traceEvent.TraceTime = now;
      traceEvent.WorldLocation = traceStart;
      traceEvent.IsInside = UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(this, traceStart);
#if STATS
      ++totalNumTracesThisFrame;
#endif
      if (!traceEvent.IsInside)
      {
         // If our trace start location is outside and the camera is inside, update our last known outdoor world location if this location is closer
         if (_cameraViewpointIsInside)
         {
            if (_lastKnownOutdoorWorldLocation)
            {
               if (FVector::DistSquared(cameraViewpoint, traceStart) < FVector::DistSquared(cameraViewpoint, *_lastKnownOutdoorWorldLocation))
               {
                  _lastKnownOutdoorWorldLocation = traceStart;
               }
            }
            else
            {
               // we didn't have an existing outdoor world location, but this point is a valid one
               _lastKnownOutdoorWorldLocation = traceStart;
            }
         }

         FCollisionQueryParams queryParams(traceTag, traceComplex);
         queryParams.bReturnPhysicalMaterial = true;
         const bool isHit = GetWorld()->LineTraceSingleByObjectType(
            hitResult,
            traceStart + FVector(0, 0, 1200),
            traceStart - FVector(0, 0, 500),
            FCollisionObjectQueryParams(collisionChannel),
            queryParams);

#if STATS
         ++totalNumTracesThisFrame;
#endif

#if OSE_CHEATS_ENABLED
         if (_temporalEnvTraceDebugging)
         {
            constexpr bool persistent = false;
            constexpr float drawDuration = 0.33f;
            constexpr float thickness = 1.5f;
            DrawDebugLine(GetWorld(), hitResult.TraceStart, hitResult.bBlockingHit ? hitResult.Location : hitResult.TraceEnd, debugColorOutside.ToFColorSRGB(),
               persistent, drawDuration, 0, thickness);
         }
#endif

         if (isHit)
         {
            traceEvent.WorldLocation = hitResult.Location;
            traceEvent.OutsideTraceHitPhysicalMaterial = hitResult.PhysMaterial.Get();
         }
      }
   }

#if STATS
   _temporalTracesPerSecondPerfCounter += totalNumTracesThisFrame;

   const uint64 temporalTraceDuration = FPlatformTime::Cycles64() - temporalEnvTraceStartTime;
   UE_CLOG(_temporalEnvTraceDebugging, LogTATWeatherManager, Verbose, TEXT("Temporal trace tick did %i traces in %llu cycles (%.4f seconds)"),
      totalNumTracesThisFrame, temporalTraceDuration, FPlatformTime::ToSeconds64(temporalTraceDuration));
#endif
}
