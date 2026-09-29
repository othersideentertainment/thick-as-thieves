// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "TATWeatherSettings.generated.h"

class ATATWeatherPreset;
class UDataTable;
struct FTATWeatherTypeInfo;
class UMaterialParameterCollection;
class UNiagaraParameterCollection;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct TAT_API FTATWeatherParameterNames
{
   GENERATED_BODY()

   /// [float] How hard it's raining - normalized value between 0 and 1
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Rain")
   FName RainIntensity = "RainIntensity";

   /// [float] How wet the ground should be (for example, materials could use this for puddles) - normalized value between 0 and 1
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Rain")
   FName GroundWetness = "GroundWetness";

   /// [float] Intensity of the rain post process effect - normalized value between 0 and 1
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Rain")
   FName CameraWetness = "CameraWetness";

   /// [Vector] Direction vector that the wind is facing in world space
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Wind")
   FName WindDirection = "WindDirection";

   /// [float] How strong the wind is blowing - value between 0 and TATWeatherSettings.MaxWindStrength
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Wind")
   FName WindStrength = "WindStrength";

   /// [float] How strong the wind is blowing - value between 0 and 1
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Wind")
   FName WindStrengthNormalized = "WindStrengthNormalized";

   /// [float] Normalized amount of debris to spawn in wind particles - value between 0 and 1
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Wind")
   FName WindDebrisAmount = "WindDebrisAmount";

   /// [Texture] The depth map of the level that maps to the weather manager's bounding box
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Depthmap")
   FName LevelDepthmapTexture = "LevelDepthmapTexture";

   /// [float] The world-space height that the depthmap represents
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Depthmap")
   FName LevelDepthmapWorldHeight = "LevelDepthmapWorldHeight";

   /// [float] The X and Y size of the depthmap in worldspace
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Depthmap")
   FName LevelDepthmapWorldSize = "LevelDepthmapWorldSize";

   /// [Vector2D] The origin of the depthmap in world-space
   UPROPERTY(Config, EditAnywhere, Category = "Global Weather Parameters|Depthmap")
   FName LevelDepthmapWorldOrigin = "LevelDepthmapWorldOrigin";
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Weather Settings"))
class TAT_API UTATWeatherSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATWeatherSettings& Get() { return *GetDefault<UTATWeatherSettings>(); }

   const FTATWeatherTypeInfo* FindWeatherTypeInfo(FGameplayTag weatherType, const UDataTable* weatherDataTable = nullptr) const;

   UFUNCTION(BlueprintCallable, DisplayName = "Find Weather Type Info", Category = "Weather Settings")
   static bool BP_FindWeatherTypeInfo(UPARAM(Meta = (Categories = "Weather.Type")) FGameplayTag weatherType, FTATWeatherTypeInfo& weatherTypeInfo);

   UFUNCTION(BlueprintCallable, DisplayName = "Get All Weather Types", Category = "Weather Settings")
   static void BP_GetAllWeatherTypes(TArray<FGameplayTag>& weatherTypes);

   UPROPERTY(Config, EditAnywhere, Category = "Gameplay", Meta=(RowType="/Script/TAT.TATWeatherTypeInfo"))
   TSoftObjectPtr<UDataTable> WeatherTypeDataTable;

   UPROPERTY(Config, EditAnywhere, Category = "Gameplay")
   float MaxWindStrength = 300.0f;

   /// If not set to None, the match settings property with this name will be read to get the current weather type.
   UPROPERTY(Config, EditAnywhere, Category = "Gameplay")
   FName MatchSettingsWeatherTypePropertyName = "WeatherType";

   /// What weather type to use as a fallback if one is not specified anywhere
   UPROPERTY(Config, EditAnywhere, Category = "Gameplay", Meta = (Categories = "Weather.Type"))
   FGameplayTag FallbackWeatherType;

   /// What weather preset to spawn as a fallback if we don't have a current weather type
   UPROPERTY(Config, EditAnywhere, Category = "Gameplay")
   TSoftClassPtr<ATATWeatherPreset> FallbackWeatherPreset;

   /// [Server-only] How frequently to check if actors are indoors or outdoors on the server.
   /// The actual interval used will be a random value in this range (to spread out traces so we're not checking all actors in the same frame).
   /// Note that this is only for server data use (like AI state)
   UPROPERTY(Config, EditAnywhere, Category = "Weather Subsystem Polling")
   FFloatInterval ServerIndoorCheck_ActorPollingRateRange = FFloatInterval(0.5f, 1.0f);

   /// [Server-only] Target tick rate - don't even try to poll more frequently than this.
   /// If we end up having a large number of actors being polled it may be worth reducing this to spread out the perf cost more evenly.
   UPROPERTY(Config, EditAnywhere, Category = "Weather Subsystem Polling", Meta = (Units = "seconds", UIMin = "0.0", ClampMin = "0.0"))
   float ServerIndoorCheck_TargetTickRateSeconds = 0.125f;

   /// [Server-only] Never check more than this many actors per frame.
   /// This should generally be set higher than actually needed (it's primarily here to avoid server stalls), but keep in mind that after BeginPlay we can have
   /// a large number of actors to test (even when randomly assigning initial setup delay times).
   UPROPERTY(Config, EditAnywhere, Category = "Weather Subsystem Polling", Meta = (UIMin = "0", ClampMin = "0"))
   int32 ServerIndoorCheck_MaxActorsToPollPerTick = 35;

   /// [Server-only] If we take longer than this time per tick, stop ticking and wait til the next frame
   UPROPERTY(Config, EditAnywhere, Category = "Weather Subsystem Polling", Meta = (UIMin = "0", ClampMin = "0"))
   float ServerIndoorCheck_CutoffFrameTimePerTick = 0.1f;

   /// The particle system asset to load for weather presets with rain enabled but no rain particle system asset assigned
   UPROPERTY(Config, EditAnywhere, Category = "Assets")
   TSoftObjectPtr<UNiagaraSystem> DefaultRainParticleSystem;

   /// The particle system asset to load for weather presets with wind enabled but no wind particle system asset assigned
   UPROPERTY(Config, EditAnywhere, Category = "Assets")
   TSoftObjectPtr<UNiagaraSystem> DefaultWindParticleSystem;

   /// Global render target to use for the scene's depthmap texture
   UPROPERTY(Config, EditAnywhere, Category = "Assets")
   TSoftObjectPtr<UTextureRenderTarget2D> DepthmapRenderTarget;

   /// The global material parameter collection used when setting material parameter values in weather presets
   UPROPERTY(Config, EditAnywhere, Category = "Global Material Parameters")
   TSoftObjectPtr<UMaterialParameterCollection> MaterialParameterCollection;

   UPROPERTY(Config, EditAnywhere, Category = "Global Material Parameters")
   FTATWeatherParameterNames MaterialParameterNames;

   /// The global particle parameter collection used when setting particle parameter values in weather presets
   UPROPERTY(Config, EditAnywhere, Category = "Global Niagara Parameters")
   TSoftObjectPtr<UNiagaraParameterCollection> NiagaraParameterCollection;

   UPROPERTY(Config, EditAnywhere, Category = "Global Niagara Parameters")
   FTATWeatherParameterNames NiagaraParameterNames;

   /// Radius around the camera viewpoint around which to do temporal traces
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces")
   float TemporalEnvironmentTraceRadius = 1200.0f;

   /// When the camera viewpoint is outside, add a random Z-height offset to the trace check location.
   /// Because the starting Z-height is the players eyes, this makes it easier to get accurate data about objects adjacent to (but taller than) the player.
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces")
   FFloatInterval TemporalEnvironmentTraceOutsideVerticalOffset = FFloatInterval(100.0f, 300.0f);

   /// Max number of temporal traces to do around the camera viewpoint per frame
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces", Meta = (UIMin = 0, UIMax = 16))
   int32 TemporalEnvironmentTraceFrequency = 2;

   /// Max number of times per second to run temporal environment traces
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces", Meta = (UIMin = 0))
   int32 TemporalEnvironmentTraceTargetFramerate = 25;

   /// How long to store a temporal trace (and how long to consider the data relevant)
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces", Meta = (UIMin = 0, UIMax = 30))
   float TemporalEnvironmentTraceTimeWindowSeconds = 2.0f;

   /// Trace channel to use for temporal traces
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces")
   TEnumAsByte<ECollisionChannel> TemporalEnvironmentTraceCollisionChannel = ECC_WorldStatic;

   /// Should temporal traces trace against complex collision?
   UPROPERTY(Config, EditAnywhere, Category = "Temporal Environment Traces")
   bool TemporalEnvironmentTraceComplex = false;

   /// Given a weather type, find the appropriate weather preset class to load
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Weather", Meta = (WorldContext = "worldContext"))
   static TSoftClassPtr<ATATWeatherPreset> GetWeatherPresetClassForWeatherType(const UObject* worldContext, FGameplayTag weatherType);

   /// Find the appropriate weather preset class to load for the current level, checking level and editor overrides as needed
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Weather", Meta = (WorldContext = "worldContext"))
   static TSoftClassPtr<ATATWeatherPreset> GetCurrentWeatherPresetClass(const UObject* worldContext, bool logOrEnsureWhenReturningFallbacks = true);

   /// Find the current weather type gameplay tag, if one is set.
   /// Note that there are many cases where there is not currently a valid one - in those cases, returnedDefaultType will be true and a default one will be returned.
   /// This function is intended for gameplay logic and is generally NOT useful for VFX purposes.
   UFUNCTION(BlueprintPure, Category = "Weather", Meta = (WorldContext = "worldContext"))
   static FGameplayTag GetCurrentWeatherType(const UObject* worldContext, bool& returnedDefaultType);

   /// Finds the data table row for the current weather type, if one is set.
   /// If one is not set, returnedDefault is set to true and the data table row for the default weather type will be returned.
   /// This function is intended for gameplay logic and is generally NOT useful for VFX purposes.
   const FTATWeatherTypeInfo* GetCurrentWeatherTypeInfo(const UObject* worldContext, bool* returnedDefault = nullptr, const UDataTable* weatherDataTable = nullptr) const;
};
