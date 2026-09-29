// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat

// ue4
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATMapVariation, Log, All);

class UTATSpawnDataAsset;
class UTATSceneSetAsset;
class UTATSpawnerComponent;

///////////////////////////////////////////////////////////////////
///        MapVariationValidationHelper
///////////////////////////////////////////////////////////////////

enum class ETATSpawnValidationReason : uint8
{
   Standalone,
   MapCheck
};

class TAT_API MapVariationValidationHelper
{
public:
   static FName kValidationLogName;
   static void InitLog();
   static void ClearLog();
   static void ValidateSpawnConfig(const UTATSpawnDataAsset& spawnDataAsset, UWorld* world, FMessageLog& messageLog, ETATSpawnValidationReason reason = ETATSpawnValidationReason::Standalone);
   static void LogInfo(const FString& infoStr, const UObject* refObject = nullptr);
   static void LogError(const FString& errorStr, const UObject* refObject = nullptr);

#if WITH_EDITOR
   static void FindAllSpawnersInWorld(UWorld* world, TArray<UTATSpawnerComponent*>& outSpawners);
   static TConstArrayView<TObjectPtr<UTATSceneSetAsset>> FindRelevantSceneSets(const AActor* actor);
#endif

private:
   static bool sInit;
};

// This still allocates an extra string, but at least it can be compiled out in shipping
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#define VALIDATE_MAPVARIATION_INFO(text, ...) \
   MapVariationValidationHelper::LogInfo(FString::Printf(text, ##__VA_ARGS__));

#define VALIDATE_MAPVARIATION_ERROR(text, ...) \
   MapVariationValidationHelper::LogError(FString::Printf(text, ##__VA_ARGS__));

#define VALIDATE_MAPVARIATION_ERROR_OBJ(refObject,text, ...) \
   MapVariationValidationHelper::LogError(FString::Printf(text, ##__VA_ARGS__), refObject);
#else
#define VALIDATE_MAPVARIATION_INFO(text, ...)
#define VALIDATE_MAPVARIATION_ERROR(text, ...)
#define VALIDATE_MAPVARIATION_ERROR_OBJ(refObject,text, ...)
#endif


///////////////////////////////////////////////////////////////////
///        Mission Validation Helper Macros
///////////////////////////////////////////////////////////////////

#define VALIDATE_ADDERROR(ErrorStr) \
      context.AddError(FText::FromString(FString::Format(TEXT("[{0}] {1}"), {GetName(), ErrorStr})));

#define VALIDATE_MISSION_TEXT(VarName) \
   if (VarName.IsEmpty()) \
   { \
      VALIDATE_ADDERROR(FString::Printf(TEXT("Text %s is empty!"), TEXT(#VarName))); \
   }

#define VALIDATE_MISSION_GAMEPLAYTAG(VarName) \
   if (!VarName.IsValid()) \
   { \
      VALIDATE_ADDERROR(FString::Printf(TEXT("GameplayTag %s is not valid!"), TEXT(#VarName))); \
   }

#define VALIDATE_MISSION_TSUBCLASSOF(VarName) \
   if (!VarName.Get()) \
   { \
      VALIDATE_ADDERROR(FString::Printf(TEXT("Class in variable %s is null!"), TEXT(#VarName))); \
   }

#define VALIDATE_MISSION_TSOFTOBJPTR(VarName) \
   if (VarName.IsNull()) \
   { \
      VALIDATE_ADDERROR(FString::Printf(TEXT("Object in variable %s is null!"), TEXT(#VarName))); \
   }

#define VALIDATE_MISSION_NULLPTR(VarName) \
   if (!VarName) \
   { \
      VALIDATE_ADDERROR(FString::Printf(TEXT("Object in variable %s is null!"), TEXT(#VarName))); \
   }

#define VALIDATE_MISSION_ARRAY_NO_NULL_ENTRIES(VarName) \
   for (int idx = 0; idx < VarName.Num(); ++idx) \
   { \
      auto entry = VarName[idx]; \
      if (!entry) \
      { \
         VALIDATE_ADDERROR(FString::Printf(TEXT("Array %s has an empty/missing asset at index %d!"), TEXT(#VarName), idx)); \
      } \
   }

#define VALIDATE_MISSION_ARRAY_SOFTCLASSPTR_NO_NULL_ENTRIES(VarName) \
   for (int idx = 0; idx < VarName.Num(); ++idx) \
   { \
      auto entry = VarName[idx]; \
      if (entry.IsNull()) \
      { \
         VALIDATE_ADDERROR(FString::Printf(TEXT("Array %s has an empty/missing asset at index %d!"), TEXT(#VarName), idx)); \
      } \
   }
