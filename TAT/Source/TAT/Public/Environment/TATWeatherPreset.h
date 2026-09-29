// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATWeatherUtilities.h"

// ue
#include "CoreMinimal.h"

#include "TATWeatherPreset.generated.h"

class ATATWeatherManager;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UPostProcessComponent;
class UNiagaraParameterCollection;
class UNiagaraComponent;
class UNiagaraSystem;
class UArrowComponent;
class UBillboardComponent;
class UPhysicalMaterial;

/// Tracks metadata about nearby physical surfaces to support weather audio hooks
/// Private struct for internal use by ATATWeatherPreset
USTRUCT()
struct FTATNearbyPhysicalSurface
{
   GENERATED_BODY()

   UPROPERTY()
   bool IsValid = false;

   UPROPERTY()
   FVector WorldLocation = FVector::ZeroVector;

   UPROPERTY()
   float DistanceAtLastSurfaceEvent = 0.0f;

   void Reset()
   {
      IsValid = false;
      WorldLocation = FVector::ZeroVector;
      DistanceAtLastSurfaceEvent = 0.0f;
   }
};

/// The artist-oriented data related to weather types
UCLASS(BlueprintType, HideCategories = (Physics, ComponentReplication, Replication, Collision, Input, Events))
class TAT_API ATATWeatherPreset : public AActor
{
   GENERATED_BODY()

public:
   ATATWeatherPreset();

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<UDirectionalLightComponent> PrimaryDirectionalLightComponent;

#if WITH_EDITORONLY_DATA
   /// Editor visualizer for directional light angle
   UPROPERTY(Meta = (HideInWeatherEditor))
   TObjectPtr<UArrowComponent> PrimaryDirectionalLightArrowComponent;

   /// Editor visualizer for the location of the height fog component
   UPROPERTY(Meta = (HideInWeatherEditor))
   TObjectPtr<UBillboardComponent> HeightFogSpriteComponent;
#endif

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<UExponentialHeightFogComponent> HeightFogComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<USkyLightComponent> SkyLightComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<USkyAtmosphereComponent> SkyAtmosphereComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<UPostProcessComponent> PostProcessComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<UNiagaraComponent> RainNiagaraComponent;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Preset")
   TObjectPtr<UNiagaraComponent> WindNiagaraComponent;

   /// Enables rain effects for this weather preset
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rain Settings")
   bool EnableRain = false;

   /// Normalized rain intensity value sent to materials and particle systems
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rain Settings", Meta = (UIMin = "0.0", UIMax = "1.0", EditCondition = "EnableRain"))
   float RainIntensity = 0.2f;

   /// How wet the ground is. Normalized value sent to materials and particle systems (materials can use for things like when to show puddles)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rain Settings", Meta = (UIMin = "0.0", UIMax = "1.0", EditCondition = "EnableRain"))
   float GroundWetness = 0.2f;

   /// Normalized wetness intensity value sent to materials and particle systems. Intended for use by the rain postprocess effect.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rain Settings", Meta = (UIMin = "0.0", UIMax = "1.0", EditCondition = "EnableRain"))
   float CameraWetness = 0.2f;

   /// If true (and rain is enabled), the OnTickRain event will be fired on tick.
   /// This is not normally required - only enable if needed to support custom rain-related features.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rain Settings", AdvancedDisplay, Meta = (EditCondition = "EnableRain"))
   bool UpdateRainOnTick = false;

   /// Enables rain effects for this weather preset
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings")
   bool EnableWind = false;

   /// How hard the wind is blowing.
   /// Note that the maximum value is in Project Settings -> [TAT] Weather Settings -> MaxWindStrength.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", Meta = (UIMin = "0.0", ClampMin = "0.0", EditCondition = "EnableWind"))
   float WindStrength = 350.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", EditCondition = "EnableWind"))
   float WindDebrisAmount = 0.05f;

   /// How the strength of the wind changes over time
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", Meta = (EditCondition = "EnableWind"))
   FTATTemporalNoise1D WindStrengthTurbulence{ 0.25f, 0.0f, 1.0f };

   /// Direction the wind is blowing in (world space)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", Meta = (EditCondition = "EnableWind"))
   FRotator WindDirection = FRotator(340.0, 0, 0);

   /// How the direction of the wind changes over time
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", Meta = (EditCondition = "EnableWind"))
   FTATTemporalNoise1D WindDirectionTurbulence{ 0.25f, -0.5f, 0.5f };

   /// If true (and wind is enabled), the OnTickWind event will be fired on tick.
   /// This is required for wind strength and direction turbulence to work.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wind Settings", AdvancedDisplay, Meta = (EditCondition = "EnableWind"))
   bool UpdateWindOnTick = true;

   /// Base weather noise level indoors (before taking distance to an exterior area into account)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
   float NoiseLevelInside = 0.0f;

   /// Weather noise level in outside spaces (eg. how loud the rain storm is)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
   float NoiseLevelOutside = 0.0f;

   /// Registers to receive temporal environment trace events for these physical materials.
   /// The OnOutsidePhysicalSurfaceNearbyBegin and OnOutsidePhysicalSurfaceNearbyEnd events will fire for each of these that appears or disappears.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", Meta = (EditCondition = "!TrackAllPhysicalSurfaceTypes"))
   TArray<TObjectPtr<UPhysicalMaterial>> TrackNearbyPhysicalSurfaceTypes;

   /// Max distance to consider as "nearby" for nearby physical surfaces
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", Meta = (UIMin = "1.0", ClampMin = "1.0"))
   float MaxDistanceToNearbyPhysicalSurface = 1200.0f;

   /// Min amount that the distance to a physical surface needs to change before OnOutsidePhysicalSurfaceNearbyUpdate is called (to avoid calling it extremely frequently for very small changes)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", AdvancedDisplay, Meta = (UIMin = "10.0", ClampMin = "10.0"))
   float MinDistanceToNearbyPhysicalSurfaceToTriggerChangeEvent = 100.0f;

   /// Instead of registering for specific physical surface types, track all of them.
   /// IMPORTANT: Enabling this has a perf hit - it's mostly just useful for testing and debugging.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Settings", AdvancedDisplay)
   bool TrackAllPhysicalSurfaceTypes = false;

   // From AActor
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;

   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   bool GetCameraViewpoint(FVector& cameraViewpoint) const;

   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   bool IsRainEnabled() const;

   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   bool IsWindEnabled() const;

   /// Gets the wind strength value (range is 0 to TATWeatherSettings.MaxWindStrength)
   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   float CalcWindStrength(float worldTimeSeconds) const;

   /// Gets the wind direction value
   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   FVector CalcWindDirection(float worldTimeSeconds) const;

   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalMaterialParameterInt(FName paramName, int32 value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalMaterialParameterFloat(FName paramName, float value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalMaterialParameterVector2D(FName paramName, const FVector2D& value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalMaterialParameterVector(FName paramName, const FVector& value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalMaterialParameterColor(FName paramName, const FLinearColor& value);
   //TODO: find an elegant way to support UTexture* values (there's not currently an easy way to handle global texture material params)
   // UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   // void SetGlobalMaterialParameterTexture(FName paramName, UTexture* value);

   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterInt(FName paramName, int32 value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterFloat(FName paramName, float value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterVector2D(FName paramName, const FVector2D& value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterVector(FName paramName, const FVector& value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterColor(FName paramName, const FLinearColor& value);
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   void SetGlobalNiagaraParameterTexture(FName paramName, UTexture* value);

   template<typename T>
   FORCEINLINE void SetGlobalMaterialAndNiagaraParameter(FName materialParamName, FName niagaraParamName, const T& value)
   {
      if constexpr (std::is_same_v<T, int32>) { SetGlobalMaterialParameterInt(materialParamName, value); SetGlobalNiagaraParameterInt(niagaraParamName, value); }
      else if constexpr (std::is_same_v<T, float>) { SetGlobalMaterialParameterFloat(materialParamName, value); SetGlobalNiagaraParameterFloat(niagaraParamName, value); }
      else if constexpr (std::is_same_v<T, FVector2D>) { SetGlobalMaterialParameterVector2D(materialParamName, value); SetGlobalNiagaraParameterVector2D(niagaraParamName, value); }
      else if constexpr (std::is_same_v<T, FVector>) { SetGlobalMaterialParameterVector(materialParamName, value); SetGlobalNiagaraParameterVector(niagaraParamName, value); }
      else if constexpr (std::is_same_v<T, FLinearColor>) { SetGlobalMaterialParameterColor(materialParamName, value); SetGlobalNiagaraParameterColor(niagaraParamName, value); }
      //TODO: UTexture* support
      static_assert(std::is_same_v<T, int32> || std::is_same_v<T, float> || std::is_same_v<T, FVector2D> || std::is_same_v<T, FVector> || std::is_same_v<T, FLinearColor>, "Expected float, FVector2D, FVector, or FLinearColor");
   }

   /// Updates the current wind direction and strength.
   /// Intended to be called from OnTickWind.
   /// Returns true if the values have changed since the last call to this function.
   UFUNCTION(BlueprintCallable, Category = "Weather Preset")
   bool UpdateCurrentWindDirectionAndStrength(const FVector& newWindDirection, float newWindStrength, bool forceUpdate = false);

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnSetupRain();
   virtual void OnSetupRain_Implementation();

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnTickRain(float deltaSeconds);
   virtual void OnTickRain_Implementation(float deltaSeconds);

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnSetupWind();
   virtual void OnSetupWind_Implementation();

   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnTickWind(float deltaSeconds);
   virtual void OnTickWind_Implementation(float deltaSeconds);

   /// Called when then level depthmap texture is updated (used to allow particles and materials to determine indoor/outdoor state)
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnSceneDepthTextureUpdated(UTextureRenderTarget2D* renderTarget, const FBox& weatherWorldBoundingBox);
   virtual void OnSceneDepthTextureUpdated_Implementation(UTextureRenderTarget2D* renderTarget, const FBox& weatherWorldBoundingBox);

   /// Called when the current camera location enters or exits an indoor location
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnCameraViewportIndoorStateChange(bool newIndoorState, const FVector& cameraViewpoint);
   virtual void OnCameraViewportIndoorStateChange_Implementation(bool newIndoorState, const FVector& cameraViewpoint);

   /// Called when entering the range of a physical surface (the surface must be registered in the TrackNearbyPhysicalSurfaceTypes array)
   /// NOTE: this event only tracks outside/exterior physical surfaces
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnOutsidePhysicalSurfaceNearbyBegin(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation);
   virtual void OnOutsidePhysicalSurfaceNearbyBegin_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation);

   /// Called when when the closest world location to a physical surface that was already in range changes (the surface must be registered in the TrackNearbyPhysicalSurfaceTypes array)
   /// NOTE: this event only tracks outside/exterior physical surfaces
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnOutsidePhysicalSurfaceNearbyUpdate(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation);
   virtual void OnOutsidePhysicalSurfaceNearbyUpdate_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation);

   /// Called when exiting the range of a physical surface (the surface must be registered in the TrackNearbyPhysicalSurfaceTypes array)
   /// NOTE: this event only tracks outside/exterior physical surfaces
   UFUNCTION(BlueprintNativeEvent, Category = "Weather Preset")
   void OnOutsidePhysicalSurfaceNearbyEnd(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation);
   virtual void OnOutsidePhysicalSurfaceNearbyEnd_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation);

   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   bool GetClosestWorldLocationToPhysicalSurface(UPhysicalMaterial* physicalMaterial, FVector& worldLocation) const;

   /// Gets the distance to a nearby physical surface.
   /// If there isn't one within range, returns false.
   /// Otherwise, returns true with the closest world location, distance, and normalized distance (where 0.0 is at the camera location and 1.0 is MaxDistanceToNearbyPhysicalSurface)
   UFUNCTION(BlueprintPure, Category = "Weather Preset")
   bool GetDistanceToNearbyPhysicalSurface(UPhysicalMaterial* physicalMaterial, FVector& cameraWorldLocation, FVector& surfaceWorldLocation, float& distance, float& normalizedDistance) const;

   /// If the camera viewpoint is inside and we have a known closest outdoor world location, returns true along with that location.
   /// If the camera viewpoint is outside or we don't have a known outdoor world location, returns false.
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   bool FindClosestKnownOutsideWorldLocationToCameraViewpoint(FVector& outdoorWorldLocation) const;

   /// Returns the target noise level based on inside/outside state
   UFUNCTION(BlueprintPure, Category = "Weather Manager")
   float GetTargetNoiseLevel() const;

protected:
   UPROPERTY(Transient)
   TObjectPtr<UMaterialParameterCollection> _cachedMaterialParameterCollection;

   UPROPERTY(Transient)
   TObjectPtr<UNiagaraParameterCollection> _cachedNiagaraParameterCollection;

   float _currentWindStrength = 0.0f;
   FVector _currentWindDirection = FVector(1.0f, 0.0f, 0.0f);

   /// A map of physical materials to their nearby world location (if any)
   UPROPERTY(Transient)
   TMap<TObjectPtr<UPhysicalMaterial>, FTATNearbyPhysicalSurface> _nearbyPhysicalSurfaces;

private:
   void _PollNearbyOutsidePhysicalSurfaces();
   ATATWeatherManager* _GetWeatherManager() const;
   UMaterialParameterCollection* _GetMaterialParameterCollection() const;
   UNiagaraParameterCollection* _GetNiagaraParameterCollection() const;
   void _CalcCurrentWindDirectionAndStrength(FVector& outCurrentDirection, float& outCurrentStrength) const;
   void _TrySetParticleComponentActivated(UNiagaraComponent* comp, bool activated, const TSoftObjectPtr<UNiagaraSystem>& defaultAsset, const TCHAR* particleType);
};
