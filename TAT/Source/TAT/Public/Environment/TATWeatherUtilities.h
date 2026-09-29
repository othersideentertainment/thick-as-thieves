// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATWeatherUtilities.generated.h"

class UTexture;
class UNiagaraParameterCollection;

DECLARE_STATS_GROUP(TEXT("TAT Weather System"), STATGROUP_Weather, STATCAT_Advanced);

/// Simple noise parameters that uses 1D perlin noise to fluctuate values over time.
/// Intended for wind direction and strength variation, but is generic enough to be useful for other purposes if needed.
USTRUCT(BlueprintType)
struct TAT_API FTATTemporalNoise1D
{
   GENERATED_BODY()

   /// How fast the value changes over time
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Noise")
   float Frequency = 0.25f;

   /// How large the value changes are (min). This is the minimum output value.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Noise")
   float MinAmplitude = -0.5f;

   /// How large the value changes are (max). This is the maximum output value.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Noise")
   float MaxAmplitude = 0.5f;

   FTATTemporalNoise1D() = default;
   FTATTemporalNoise1D(float frequency, float minAmplitude, float maxAmplitude) : Frequency(frequency), MinAmplitude(minAmplitude), MaxAmplitude(maxAmplitude) {}
   FTATTemporalNoise1D(float frequency, const FFloatInterval& amplitude) : Frequency(frequency), MinAmplitude(amplitude.Min), MaxAmplitude(amplitude.Max) {}

   /// Given a world time in seconds, returns the current noise value in the range MinAmplitude..MaxAmplitude
   float Evaluate(float worldTimeSeconds) const;
};

///
UCLASS()
class TAT_API UTATWeatherUtilities : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // For some reason, there's no helper function for this in UNiagaraParameterCollectionInstance like there is for types like float
   static bool SetNiagaraParameterCollectionTexture(UObject* worldContext, UNiagaraParameterCollection* parameterCollection, const FName& paramName, UTexture* texture, bool logOnError = false);

   /// Helper function to assign all weather depthmap parameters to material and niagara parameter collections at once
   static void SetWeatherDepthmapParameters(
      UObject* worldContext,
      UMaterialParameterCollection* materialParameterCollection,
      UNiagaraParameterCollection* niagaraParameterCollection,
      UTexture* depthmapTexture,
      float depthmapWorldSize,
      float depthmapWorldHeight,
      const FVector& depthmapWorldOrigin);

   /// A time function that is specifically intended for temporal weather-related VFX.
   /// During a normal game, this simply returns world time, but anywhere else (like the editor) it works anywhere, regardless of play/simulate state.
   UFUNCTION(BlueprintPure, Category = "Weather Utilities", Meta = (WorldContext = "contextObject"))
   static double GetWeatherTimeSeconds(const UObject* contextObject);

   /// Returns the current noise value in the range MinAmplitude..MaxAmplitude using the current world time
   /// Note that if you want to use a custom time value, use the EvaluateWeatherNoiseAtTime function instead.
   UFUNCTION(BlueprintPure, Category = "Weather Utilities", Meta = (WorldContext = "contextObject"))
   static float EvaluateWeatherNoise(const UObject* contextObject, const FTATTemporalNoise1D& noise);

   /// Given a world time in seconds, returns the current noise value in the range MinAmplitude..MaxAmplitude
   UFUNCTION(BlueprintPure, Category = "Weather Utilities")
   static float EvaluateWeatherNoiseAtTime(const FTATTemporalNoise1D& noise, float timeSeconds) { return noise.Evaluate(timeSeconds); }

   /// Normalizes a wind strength value to get a value between 0 and 1
   UFUNCTION(BlueprintPure, Category = "Weather Utilities")
   static float NormalizeWindStrength(float windStrength);

   UFUNCTION(BlueprintPure, Category = "Weather Utilities")
   static bool IsWeatherTypeAllowedInLevel(FGameplayTag weatherType, const TSoftObjectPtr<UWorld>& level);

   UFUNCTION(BlueprintCallable, Category = "Weather Utilities", Meta = (WorldContext = "contextObject"))
   static bool LineTraceCheckIfLocationIsInside(const UObject* contextObject, FVector worldLocation, float minTraceDistance = 0.0f, AActor* ignoreActor = nullptr);

};
