// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Utl/OSEUtlFunctionLibrary.h"

// ose
#include "OSECommon.h"
#include "Utl/OSENamedObjectInterface.h"

// ue
#include "PhysicsEngine/BodySetup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUtlFunctionLibrary)



void UOSEUtlFunctionLibrary::MarkAllComponentsOfClassNetAddressable(TSubclassOf<UActorComponent> subclass, AActor* actor)
{
   if (actor)
   {
      TInlineComponentArray<UActorComponent*> splineMeshes;
      actor->GetComponents(subclass, splineMeshes);
      for (UActorComponent* splineMesh : splineMeshes)
      {
         MarkComponentNetAddressable(splineMesh);
      }
   }
}

void UOSEUtlFunctionLibrary::MarkComponentNetAddressable(UActorComponent* component)
{
   if (component)
   {
      component->SetNetAddressable();
   }
}

FString UOSEUtlFunctionLibrary::GetPlayerFacingActorName(AActor* actor)
{
   if (!actor)
      return FString();
   
   if (APlayerState* ps = UOSECommon::GetPlayerState<APlayerState>(actor))
   {
      return ps->GetPlayerName();
   }

   if (actor->Implements<UOSENamedObjectInterface>())
   {
      FText name = IOSENamedObjectInterface::Execute_GetObjectName(actor);
      if (!name.IsEmpty())
         return name.ToString();
   }

   return actor->GetHumanReadableName();
}

FString UOSEUtlFunctionLibrary::GetPlayerFacingComponentName(UActorComponent* component)
{
   if (!component)
      return FString();

   if (component->Implements<UOSENamedObjectInterface>())
   {
      FText name = IOSENamedObjectInterface::Execute_GetObjectName(component);
      if (!name.IsEmpty())
         return name.ToString();
   }

   if (AActor* ownerActor = component->GetOwner())
   {
      return GetPlayerFacingActorName(ownerActor);
   }

   return component->GetName();
}

FName UOSEUtlFunctionLibrary::GetStaticMeshCollisionProfileName(const UStaticMesh* staticMesh)
{
   if (staticMesh)
   {
      if (const UBodySetup* bodySetup = staticMesh->GetBodySetup())
      {
         return bodySetup->DefaultInstance.GetCollisionProfileName();
      }
   }
   return NAME_None;
}

FText UOSEUtlFunctionLibrary::FormatTextFromTextArrayArgs(const FText& fmt, const TArray<FText>& textArgs)
{
   if (textArgs.Num() == 0)
      return fmt;

   TArray<FFormatArgumentValue> args;
   args.Reserve(textArgs.Num());
   for (const FText& textArg : textArgs)
   {
      args.Add(textArg);
   }
   return FText::Format(fmt, args);
}

void UOSEUtlFunctionLibrary::Array_Shuffle_Stream(const FRandomStream& randomStream, const TArray<int32>& targetArray)
{
   // We should never hit these!  They're stubs to avoid NoExport on the class.  Call the Generic* equivalent instead
   check(0);
}

void UOSEUtlFunctionLibrary::GenericArray_Shuffle_Stream(void* targetArray, const FArrayProperty* arrayProp, const FRandomStream* randomStream)
{
   if (targetArray && randomStream)
   {
      FScriptArrayHelper arrayHelper(arrayProp, targetArray);
      int32 lastIndex = arrayHelper.Num() - 1;
      for (int32 i = 0; i < lastIndex; ++i)
      {
         int32 index = randomStream->RandRange(i, lastIndex);
         if (i != index)
         {
            arrayHelper.SwapValues(i, index);
         }
      }
   }
}


void UOSEUtlFunctionLibrary::PurgeInvalidObjectsFromArray(TArray<UObject*>& objArray)
{
   objArray.RemoveAll([](UObject* obj) { return !IsValid(obj); });
}

bool UOSEUtlFunctionLibrary::Map_FindIndex(const TMap<int32, int32>& targetMap, const int32& key, int32& index)
{
   checkNoEntry();
   return false;
}

DEFINE_FUNCTION(UOSEUtlFunctionLibrary::execMap_FindIndex)
{
   Stack.MostRecentProperty = nullptr;
   Stack.StepCompiledIn<FMapProperty>(nullptr);
   const FMapProperty* mapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
   const void* mapAddress = Stack.MostRecentPropertyAddress;

   if (!mapProperty)
   {
      Stack.bArrayContextFailed = true;
      return;
   }

   const FProperty* keyProperty = mapProperty->KeyProp;
   const int32 keyPropertySize = keyProperty->GetElementSize() * keyProperty->ArrayDim;
   void* keyAddress = FMemory_Alloca(keyPropertySize);
   keyProperty->InitializeValue(keyAddress);

   Stack.MostRecentPropertyAddress = nullptr;
   Stack.MostRecentPropertyContainer = nullptr;
   Stack.StepCompiledIn<FProperty>(keyAddress);

   P_GET_PROPERTY_REF(FIntProperty, index);

   P_FINISH;

   bool bResult = false;

   P_NATIVE_BEGIN;
      if (mapAddress)
      {
         FScriptMapHelper mapHelper(mapProperty, mapAddress);
         index = mapHelper.FindMapPairIndexFromHash(keyAddress);

         bResult = index != INDEX_NONE;
      }
   P_NATIVE_END;

   keyProperty->DestroyValue(keyAddress);

   *static_cast<bool*>(RESULT_PARAM) = bResult;
}
