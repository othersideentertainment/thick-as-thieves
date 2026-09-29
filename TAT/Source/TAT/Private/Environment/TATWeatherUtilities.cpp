// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherUtilities.h"

// tat
#include "Developer/TATWeatherSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Environment/TATWeatherManager.h"
#include "Environment/TATWeatherTypeInfo.h"

// ue
#include "NiagaraDataInterfaceTexture.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraParameterCollection.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherUtilities)
DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherUtilities, Log, All);

namespace WeatherHelpers
{
   /// Get a "world" time that's useful outside of an actual game world - essentially the number of seconds that the engine has been running.
   double GetEngineTimeSeconds()
   {
      static const double baseTimeSeconds = FPlatformTime::Seconds();
      return FPlatformTime::Seconds() - baseTimeSeconds;
   }
}

float FTATTemporalNoise1D::Evaluate(float worldTimeSeconds) const
{
   return FMath::GetMappedRangeValueClamped(
      FVector2f(-1.0f, 1.0f),
      FVector2f(MinAmplitude, MaxAmplitude),
      FMath::PerlinNoise1D(worldTimeSeconds * Frequency));
}

bool UTATWeatherUtilities::SetNiagaraParameterCollectionTexture(UObject* worldContext, UNiagaraParameterCollection* parameterCollection, const FName& paramName, UTexture* texture, bool logOnError)
{
   if (worldContext == nullptr || parameterCollection == nullptr || paramName.IsNone())
   {
      return false;
   }

   auto errorPrefix = [&]() -> FString
   {
      return FString::Printf(TEXT("Failed to set parameter '%s' in Niagara parameter collection '%s'"),
         (parameterCollection != nullptr ? *parameterCollection->GetName() : TEXT("NULL")), *paramName.ToString());
   };

   // null texture is fine (to clear the reference), but if it's not null, verify its type
   if (texture != nullptr && !(texture->IsA<UTexture2D>() || texture->IsA<UTextureRenderTarget2D>()))
   {
      UE_CLOG(logOnError, LogTATWeatherUtilities, Error, TEXT("%s: expected Texture2D or TextureRenderTarget2D, got '%s'"), *errorPrefix(), *texture->GetClass()->GetName());
      return false;
   }

   UNiagaraParameterCollectionInstance* collectionInst = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(worldContext, parameterCollection);
   if (collectionInst == nullptr)
   {
      UE_CLOG(logOnError, LogTATWeatherUtilities, Error, TEXT("%s: failed to find parameter collection instance"), *errorPrefix());
      return false;
   }

   FNiagaraParameterStore& parameterStore = collectionInst->GetParameterStore();

   const FName actualParamName = parameterCollection->ConditionalAddFullNamespace(paramName);
   const FNiagaraVariable niagaraVariable = FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceTexture::StaticClass()), actualParamName);
   const int32 niagaraVariableIndex = parameterStore.IndexOf(niagaraVariable);
   if (niagaraVariableIndex == INDEX_NONE)
   {
      UE_CLOG(logOnError, LogTATWeatherUtilities, Error, TEXT("%s: no such parameter with texture sample type"), *errorPrefix());
      return false;
   }

   UNiagaraDataInterfaceTexture* textureDataInterface = Cast<UNiagaraDataInterfaceTexture>(parameterStore.GetDataInterface(niagaraVariableIndex));
   if (textureDataInterface == nullptr)
   {
      UE_CLOG(logOnError, LogTATWeatherUtilities, Error, TEXT("%s: parameter data interface object was not valid"), *errorPrefix());
      return false;
   }

   textureDataInterface->SetTexture(texture);
   return true;
}

// static
void UTATWeatherUtilities::SetWeatherDepthmapParameters(
   UObject* worldContext,
   UMaterialParameterCollection* materialParameterCollection,
   UNiagaraParameterCollection* niagaraParameterCollection,
   UTexture* depthmapTexture,
   float depthmapWorldSize,
   float depthmapWorldHeight,
   const FVector& depthmapWorldOrigin)
{
   if (worldContext == nullptr)
   {
      return;
   }

   if (niagaraParameterCollection != nullptr)
   {
      const FTATWeatherParameterNames& paramNames = UTATWeatherSettings::Get().NiagaraParameterNames;

      constexpr bool logOnError = true;
      SetNiagaraParameterCollectionTexture(worldContext, niagaraParameterCollection, paramNames.LevelDepthmapTexture, depthmapTexture, logOnError);

      if (UNiagaraParameterCollectionInstance* paramInst = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(worldContext, niagaraParameterCollection))
      {
         paramInst->SetFloatParameter(paramNames.LevelDepthmapWorldSize.ToString(), depthmapWorldSize);
         paramInst->SetFloatParameter(paramNames.LevelDepthmapWorldHeight.ToString(), depthmapWorldHeight);
         paramInst->SetVectorParameter(paramNames.LevelDepthmapWorldOrigin.ToString(), depthmapWorldOrigin);
      }
   }

   if (materialParameterCollection != nullptr)
   {
      const FTATWeatherParameterNames& paramNames = UTATWeatherSettings::Get().MaterialParameterNames;
      UKismetMaterialLibrary::SetScalarParameterValue(worldContext, materialParameterCollection, paramNames.LevelDepthmapWorldSize, depthmapWorldSize);
      UKismetMaterialLibrary::SetScalarParameterValue(worldContext, materialParameterCollection, paramNames.LevelDepthmapWorldHeight, depthmapWorldHeight);
      UKismetMaterialLibrary::SetVectorParameterValue(worldContext, materialParameterCollection, paramNames.LevelDepthmapWorldOrigin,
         FLinearColor(depthmapWorldOrigin.X, depthmapWorldOrigin.Y, depthmapWorldOrigin.Z, 0.0f));
   }
}

// static
double UTATWeatherUtilities::GetWeatherTimeSeconds(const UObject* contextObject)
{
#if WITH_EDITOR
   // If this is the editor and we're _not_ in PIE or simulate, always return engine time
   if (GEditor && !GEditor->IsPlaySessionInProgress())
   {
      return WeatherHelpers::GetEngineTimeSeconds();
   }
#endif

   // In a normal game world, just return the normal world time
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      return world->GetTimeSeconds();
   }

   return WeatherHelpers::GetEngineTimeSeconds();
}

// static
float UTATWeatherUtilities::EvaluateWeatherNoise(const UObject* contextObject, const FTATTemporalNoise1D& noise)
{
   return noise.Evaluate(GetWeatherTimeSeconds(contextObject));
}

// static
float UTATWeatherUtilities::NormalizeWindStrength(float windStrength)
{
   return FMath::Clamp(FMath::Max(0.0f, windStrength) / FMath::Max(1.0f, UTATWeatherSettings::Get().MaxWindStrength), 0.0f, 1.0f);
}

//static
bool UTATWeatherUtilities::IsWeatherTypeAllowedInLevel(FGameplayTag weatherType, const TSoftObjectPtr<UWorld>& level)
{
   if (const FTATWeatherTypeInfo* weatherInfo = UTATWeatherSettings::Get().FindWeatherTypeInfo(weatherType))
   {
      return weatherInfo->IsWeatherTypeAllowedInLevel(level);
   }
   return false;
}

// static
bool UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(const UObject* contextObject,
                                                            FVector worldLocation,
                                                            float minTraceDistance,
                                                            AActor* ignoreActor)
{
   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   constexpr float defaultTraceDistance = 10000.0f;
   float traceDistance = 0.0f;

   ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   if (worldSettings != nullptr && worldSettings->WeatherManager != nullptr)
   {
      const FBox weatherBounds = worldSettings->WeatherManager->GetWeatherBoundingBox();
      ensure(weatherBounds.Min.Z <= weatherBounds.Max.Z);
      if (worldLocation.Z >= weatherBounds.Max.Z)
      {
         // point is over the weather bounding box, assume outside
         return false;
      }
      if (worldLocation.Z <= weatherBounds.Min.Z)
      {
         // point is under the weather bounding box, assume inside
         return true;
      }

      // Only trace upwards as far as the top of the bounding box
      traceDistance = weatherBounds.Max.Z - worldLocation.Z;
   }

   if (minTraceDistance >= 0 && traceDistance < minTraceDistance)
   {
      traceDistance = minTraceDistance;
   }
   if (traceDistance <= 0)
   {
      traceDistance = defaultTraceDistance;
   }

   static const FName traceTag(TEXT("TATWeatherUtilities_LineTraceCheckIfLocationIsInside"));
   constexpr ECollisionChannel collisionChannel = ECC_WorldStatic;
   const FVector traceEnd = worldLocation + FVector(0, 0, traceDistance);
   constexpr bool traceComplex = false;
   FHitResult hitResult;
   FCollisionQueryParams queryParams(traceTag, traceComplex);
   if (ignoreActor != nullptr)
   {
      queryParams.AddIgnoredActor(ignoreActor);
   }
   return world->LineTraceSingleByObjectType(hitResult, worldLocation, traceEnd, FCollisionObjectQueryParams(collisionChannel), queryParams);
}
