// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Templates/UnrealTemplate.h"

#include "OSEUtlFunctionLibrary.generated.h"

class UStaticMesh;

UCLASS()
class OSECORE_API UOSEUtlFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // We should only ever need to call this in some very specific circumstances, don't call this unless you know what you're doing! 
   UFUNCTION(BlueprintCallable, Category="OSE Utility")
   static void MarkAllComponentsOfClassNetAddressable(TSubclassOf<UActorComponent> subclass, AActor* actor);
   
   // We should only ever need to call this in some very specific circumstances, don't call this unless you know what you're doing!
   UFUNCTION(BlueprintCallable, Category = "OSE Utility")
   static void MarkComponentNetAddressable(UActorComponent* component);

   // Get a name for an actor, prioritizing player name, then OSENamedObjectInterface, then falling back to HumanReadableName
   UFUNCTION(BlueprintPure, Category = "OSE Utility")
   static FString GetPlayerFacingActorName(AActor* actor);

   // Get a name for a component, prioritizing player name, then OSENamedObjectInterface, then falling back to HumanReadableName
   UFUNCTION(BlueprintPure, Category = "OSE Utility")
   static FString GetPlayerFacingComponentName(UActorComponent* component);

   // Get the collision profile for a static mesh
   UFUNCTION(BlueprintPure, Category = "OSE Utility")
   static FName GetStaticMeshCollisionProfileName(const UStaticMesh* staticMesh);

   // Given an array of localized FText and a format string, return the resulting FText
   UFUNCTION(BlueprintPure, Category = "OSE Utility")
   static FText FormatTextFromTextArrayArgs(const FText& fmt, const TArray<FText>& textArgs);
   
   // Fisher-Yates shuffle using an FRandomStream rather than FMath as provided by Algo::RandomShuffle
   template <typename RangeType>
   static void ShuffleWithRandomStream(RangeType& range, const FRandomStream& stream)
   {
      auto data = GetData(range);

      using SizeType = decltype(GetNum(range));
      const SizeType num = GetNum(range);

      for (SizeType index = 0; index < num - 1; ++index)
      {
         // Get a random integer in [Index, Num)
         const SizeType randomIndex = index + (SizeType)stream.RandHelper(num - index);
         if (randomIndex != index)
         {
            Swap(data[index], data[randomIndex]);
         }
      }
   }

   // Shuffle (randomize) the elements of an array given a random stream
   UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Shuffle (by Random Stream)", CompactNodeTitle = "SHUFFLE", ArrayParm = "TargetArray"), Category = "Utilities|Array|OSE")
   static void Array_Shuffle_Stream(const FRandomStream& stream, const TArray<int32>& targetArray);
   static void GenericArray_Shuffle_Stream(void* targetArray, const FArrayProperty* arrayProp, const FRandomStream* randomStream);

   UFUNCTION(BlueprintCallable, Category = "OSE Utility")
   static void PurgeInvalidObjectsFromArray(UPARAM(ref) TArray<UObject*>& objArray);

   DECLARE_FUNCTION(execArray_Shuffle_Stream)
   {
      Stack.MostRecentProperty = nullptr;
      Stack.StepCompiledIn<FProperty>(nullptr);
      FRandomStream* randomStream = (FRandomStream*)Stack.MostRecentPropertyAddress;
      if (!randomStream)
      {
         Stack.bArrayContextFailed = true;
         return;
      }

      Stack.MostRecentProperty = nullptr;
      Stack.StepCompiledIn<FArrayProperty>(NULL);
      void* arrayAddr = Stack.MostRecentPropertyAddress;
      FArrayProperty* arrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
      if (!arrayProperty)
      {
         Stack.bArrayContextFailed = true;
         return;
      }

      P_FINISH;
      P_NATIVE_BEGIN;
      MARK_PROPERTY_DIRTY(Stack.Object, arrayProperty);
      GenericArray_Shuffle_Stream(arrayAddr, arrayProperty, randomStream);
      P_NATIVE_END;
   }

   UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Find Index", CompactNodeTitle = "FIND INDEX", MapParam = "targetMap", MapKeyParam = "key", AutoCreateRefTerm = "key", BlueprintThreadSafe), Category = "Utilities|Map|OSE")
   static bool Map_FindIndex(const TMap<int32, int32>& targetMap, const int32& key, int32& index);
   DECLARE_FUNCTION(execMap_FindIndex);
};
