// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATWeatherManager.generated.h"

class ATATWeatherPreset;
class ATATWeatherPresetDedicatedServer;
class UArrowComponent;
class UBoxComponent;
class UTATAudioRoomComponent;
class UPhysicalMaterial;

/// Represents a single temporal trace (traces that happen at random around the camera used to detect weather events)
USTRUCT(BlueprintType)
struct TAT_API FTATTemporalEnvironmentTraceEvent
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporal Trace Event")
   float TraceTime = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporal Trace Event")
   bool IsInside = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporal Trace Event")
   FVector WorldLocation = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporal Trace Event")
   TObjectPtr<UPhysicalMaterial> OutsideTraceHitPhysicalMaterial;

   FORCEINLINE bool IsValid() const
   {
      return TraceTime > 0;
   }

   FORCEINLINE float TimeSinceTrace(float currentTime) const
   {
      return (TraceTime > 0) ? (currentTime - TraceTime) : 0.0f;
   }

   void Reset()
   {
      TraceTime = 0;
      IsInside = false;
      WorldLocation = FVector::ZeroVector;
      OutsideTraceHitPhysicalMaterial = nullptr;
   }
};

///
UCLASS(Blueprintable, HideCategories = (Physics, ComponentReplication, Replication, Collision, Input, Sockets, Tags, Cooking))
class TAT_API ATATWeatherManager : public AActor
{
   GENERATED_BODY()

   ATATWeatherManager();

public:
   UFUNCTION(BlueprintPure, DisplayName = "Get Weather Manager", Category = "Weather Manager", Meta = (WorldContext = "contextObject", CompactNodeTitle = "Weather Manager"))
   static ATATWeatherManager* Get(const UObject* contextObject);

   // From AActor
   virtual void PostInitializeComponents() override;
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;

   UFUNCTION(CallInEditor, Category = "Weather Manager - Editor Tools")
   void RefreshLevelDepthmap();

#if WITH_EDITORONLY_DATA
   /// [Editor Only] If enabled, the wind direction arrow component will be visible during simulate/PIE for debugging wind direction and strength.
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Weather Manager - Editor Tools")
   bool ShowWeatherVaneInGame = false;
#endif

   /// Allow overriding what weather preset is used for each weather type in this level
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Weather Manager", Meta = (Categories = "Weather.Type"))
   TMap<FGameplayTag, TSoftClassPtr<ATATWeatherPreset>> PresetOverrides;

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<UBillboardComponent> WeatherManagerIconComponent;
#endif

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Manager")
   TObjectPtr<UWindDirectionalSourceComponent> WindDirectionSourceComponent;

#if WITH_EDITORONLY_DATA
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Manager")
   TObjectPtr<UArrowComponent> WindDirectionArrowComponent;
#endif

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Manager")
   TObjectPtr<UBoxComponent> WeatherBoundsComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Manager")
   TObjectPtr<UTATAudioRoomComponent> WorldAudioRoomComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Manager")
   TObjectPtr<USceneCaptureComponent2D> LevelDepthSceneCaptureComponent;

   /// Checks if rain is currently enabled
   /// NB. This is only useful in game and can't be used in editor contexts
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   bool IsRainEnabled() const;

   /// Checks if wind is currently enabled
   /// NB. This is only useful in game and can't be used in editor contexts
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   bool IsWindEnabled() const;

   /// Gets the currently spawned weather preset actor.
   /// NB. This is only useful in game and can't be used in editor contexts
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   ATATWeatherPreset* GetCurrentWeatherPreset() const { return _weatherPreset; }

   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   bool GetCameraViewpoint(FVector& outViewLocation) const;

   /// Returns the current wind strength and direction. These values are computed by weather presets.
   /// NB. If you need the normalized wind strength, use the NormalizeWindStrength helper function from UTATWeatherUtilities.
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   void GetWindDirectionAndStrength(FVector& windDirection, float& windStrength) const;

   /// Gets the current weather bounding box.
   /// This is the world-space volume used to check if a point in the world is inside or outside.
   /// The level depthmap is mapped 1:1 with this region.
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   FBox GetWeatherBoundingBox() const;

   /// Gets the current camera indoor state (as opposed to outdoors)
   /// NB. Not useful except on local controllers!
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   bool IsCameraViewpointInside() const { return _cameraViewpointIsInside; }

   /// Only called by weather presets to let the weather manager know about the new values
   void UpdateWindDirectionAndStrengthFromPreset(ATATWeatherPreset* preset, const FVector& newWindDirection, float newWindStrength);

   UFUNCTION(BlueprintCallable, Category = "Weather Manager")
   void SetWeatherPresetType(TSubclassOf<ATATWeatherPreset> presetType);

   /// Convert a bounding box to depthmap parameters that can be passed to material and niagara parameter collections.
   /// @param weatherWorldBoundingBox The bounding box to convert to depthmap parameters. @see GetWeatherBoundingBox
   /// @param depthmapWorldSize       Size of the world in X and Y that the depthmap texture represents
   /// @param depthmapWorldHeight     World height that the depthmap texture represents
   /// @param depthmapWorldOrigin     World origin of the space that the depthmap represents
   ///                                (NB. this is the corner point - add FVector(worldSize, worldSize, worldHeight) to get the other corner)
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   static void CalcDepthmapParameters(const FBox& weatherWorldBoundingBox, float& depthmapWorldSize, float& depthmapWorldHeight, FVector& depthmapWorldOrigin);

   /// Rebuild the scene depthmap if needed (unless force is true, in which case it is always rebuilt)
   UFUNCTION(BlueprintCallable, Category = "Weather Manager")
   bool UpdateSceneDepthTexture(bool forceUpdate);

   /// Rebuild the scene depthmap. If a preset actor is specified, always call its OnSceneDepthTextureUpdated event even if the depthmap was already up to date.
   /// Returns true if the depthmap was rebuilt.
   /// Note that this is a public function to allow the weather editor tools to use this for preset previews.
   bool UpdateSceneDepthTextureInternal(bool forceUpdate, ATATWeatherPreset* notifyWeatherPreset);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSceneDepthTextureEvent, UTexture*, sceneDepthmapTexture, const FBox&, depthmapBoundingBox, bool, textureUpdated);

   /// Event fired every time an update to the scene depthmap is requested
   UPROPERTY(BlueprintAssignable, Category = "Weather Manager")
   FSceneDepthTextureEvent OnUpdateSceneDepthTexture;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIndoorStateChangeEvent, bool, newIndoorState, FVector, cameraViewpoint);

   /// Event fired whenever the camera moves to an indoor or outdoor location (generally, the player pawn, but could also be a spectator or editor camera)
   UPROPERTY(BlueprintAssignable, Category = "Weather Manager")
   FIndoorStateChangeEvent OnCameraIndoorStateChange;

   TOptional<FVector> GetLastKnownOutdoorWorldLocation() const { return _lastKnownOutdoorWorldLocation; }
   const TArray<FTATTemporalEnvironmentTraceEvent>& GetTemporalEnvironmentTraceData() const { return _temporalEnvironmentTraceData; }

   void SetTemporalTraceDebuggingEnabled(bool debugEnabled) { _temporalEnvTraceDebugging = debugEnabled; }
   bool GetTemporalTraceDebuggingEnabled() const { return _temporalEnvTraceDebugging; }

protected:
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Manager")
   void OnWeatherPresetCreated(ATATWeatherPreset* preset);
   void OnWeatherPresetCreated_Implementation(ATATWeatherPreset* preset) {}

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Manager")
   void OnWeatherPresetAboutToBeDestroyed(ATATWeatherPreset* preset);
   void OnWeatherPresetAboutToBeDestroyed_Implementation(ATATWeatherPreset* preset) {}

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Manager")
   void OnCameraViewportIndoorStateChange(bool newIndoorState, const FVector& cameraViewpoint);
   void OnCameraViewportIndoorStateChange_Implementation(bool newIndoorState, const FVector& cameraViewpoint) {}

   void LoadWeatherPresetTypeForDedicatedServer();

   /// Rotation of the scene depth render target camera (the location is the top-center of the WeatherBoundsComponent box)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Weather Manager")
   FRotator _sceneDepthRenderWorldRotation;

private:
   UPROPERTY(Transient)
   TSet<ULevelStreaming*> _currentStreamingLevels;

   void _PollForStreamingLevelChanges();

   void _TickTemporalTraces(const FVector& cameraViewpoint);

   FTimerHandle _levelStreamingChangeTimer;

   UFUNCTION()
   void _OnLevelStreamingChange();

   UFUNCTION()
   void _PostLevelStreamingChange();

   bool _sceneDepthMapGenerated = false;

   FVector _windDirection = FVector(1, 0, 0);
   float _windStrength = 0.0f;

   bool _cameraViewpointStateValid = false;
   bool _cameraViewpointIsInside = false;

   float _lastTemporalEnvTraceTimeSeconds = 0;
   FRandomStream _temporalEnvTraceRandomStream;
   int32 _temporalEnvTraceNextIndex = 0;
   bool _temporalEnvTraceDebugging = false;
   TOptional<FVector> _lastKnownOutdoorWorldLocation;
#if STATS
   double _lastTemporalTraceEnvPerfCounterUpdateTime = 0;
   int32 _temporalTracesPerSecondPerfCounter = 0;
#endif

   /// The current weather preset actor
   /// This is the one that is spawned in game and owned by this weather manager, NOT the preview one spawned in the editor
   UPROPERTY(Transient)
   TObjectPtr<ATATWeatherPreset> _weatherPreset;

   UPROPERTY(Transient)
   TObjectPtr<ATATWeatherPresetDedicatedServer> _weatherPresetDedicatedServer;

   /// Render target to use for the depthmap
   UPROPERTY(Transient)
   TObjectPtr<UTextureRenderTarget2D> _sceneDepthRenderTarget;

   UPROPERTY(Transient)
   TObjectPtr<APlayerController> _localPlayerController;

   UPROPERTY(Transient)
   TArray<FTATTemporalEnvironmentTraceEvent> _temporalEnvironmentTraceData;
};
