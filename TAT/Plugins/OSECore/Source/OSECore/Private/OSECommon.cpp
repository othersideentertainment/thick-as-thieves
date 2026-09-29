// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSECommon.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Player/OSEPlayerController.h"
#include "Player/OSEPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECommon)

static uint32 s_UniqueIDCounter = 0;

FName UOSECommon::GenerateUniqueID(const FString& BaseString)
{
   FName Name(*BaseString);
   Name.SetNumber(++s_UniqueIDCounter);
   return Name;
}

FName UOSECommon::GenerateUniqueID(const TCHAR* BaseString)
{
   FName Name(BaseString);
   Name.SetNumber(++s_UniqueIDCounter);
   return Name;
}

FName UOSECommon::GenerateUniqueIDFromName(FName BaseName)
{
   BaseName.SetNumber(++s_UniqueIDCounter);
   return BaseName;
}

AOSECharacterBase* UOSECommon::GetCharacterFromActor(AActor* Actor)
{
   return UOSECommon::GetPawn<AOSECharacterBase>(Actor);
}

AOSECharacterBase* UOSECommon::GetCharacterFromComponent(UActorComponent* Component)
{
   return UOSECommon::GetPawnFromComponent<AOSECharacterBase>(Component);
}

bool UOSECommon::IsAPlayer(AActor* actor)
{
   if (auto Pawn = UOSECommon::GetPawn(actor))
   {
      return Pawn->IsPlayerControlled();
   }
   return false;
}

UPhysicalMaterial* UOSECommon::GetPhysicalMaterialFromHitResult(const FHitResult& hitResult)
{
   if (hitResult.PhysMaterial.IsValid())
   {
      return hitResult.PhysMaterial.Get();
   }

   if (hitResult.Component.IsValid())
   {
      UPhysicalMaterial* physMat = GetPhysicalMaterialFromComponent(hitResult.Component.Get(), hitResult.FaceIndex);
      if (physMat != nullptr)
      {
         return physMat;
      }
   }

   AActor* actor = hitResult.GetActor();
   if (actor != nullptr)
   {
      UPrimitiveComponent* primitiveComponent = actor->FindComponentByClass<UPrimitiveComponent>();
      if (primitiveComponent != nullptr)
      {
         return GetPhysicalMaterialFromComponent(primitiveComponent, hitResult.FaceIndex);
      }
   }

   return nullptr;
}

UPhysicalMaterial* UOSECommon::GetPhysicalMaterialFromComponent(const UPrimitiveComponent* primitiveComponent, int32 faceIndex)
{
   if (primitiveComponent != nullptr)
   {
      // This will respect UPrimitiveComponent::SetPHysMatOverride, so it gets checked first.
      UPhysicalMaterial* simplePhysMat = primitiveComponent->BodyInstance.GetSimplePhysicalMaterial();
      if (simplePhysMat != nullptr)
      {
         return simplePhysMat;
      }

      int32 sectionIndex = 0;
      UMaterialInterface* material = primitiveComponent->GetMaterialFromCollisionFaceIndex(faceIndex, sectionIndex);
      if (material != nullptr)
      {
         UPhysicalMaterial* faceIndexPhysMat = material->GetPhysicalMaterial();
         if (faceIndexPhysMat != nullptr)
         {
            return faceIndexPhysMat;
         }
      }

      for (int i = 0; i < primitiveComponent->GetNumMaterials(); i++)
      {
         material = primitiveComponent->GetMaterial(i);
         if (material != nullptr)
         {
            UPhysicalMaterial* physMat = material->GetPhysicalMaterial();
            if (physMat != nullptr)
            {
               return physMat;
            }
         }
      }
   }

   return nullptr;
}


static int32 RoundFloatToInt(float f)
{
   return int32(f + FPlatformMath::Sign(f) * 0.5f);
}

static int64 RoundFloatToInt(double f)
{
   return int64(f + FPlatformMath::Sign(f) * 0.5f);
}

static void OSECommon_QuantizeUnscaled(FVector& vector)
{
   using ScalarType = FVector::FReal;
   constexpr SIZE_T ScalarTypeSize = sizeof(ScalarType);
   using IntType = typename TSignedIntType<ScalarTypeSize>::Type;

   // Rounding of large values can introduce additional precision errors and the extra cost to serialize with full precision is small.
   constexpr uint32 MaxExponentAfterScaling = ScalarTypeSize == 4 ? 30U : 62U;
   constexpr ScalarType MaxScaledValue = ScalarType(IntType(1) << MaxExponentAfterScaling);

   if( vector.GetAbsMax() < MaxScaledValue)
   {
      vector.X = RoundFloatToInt(vector.X);
      vector.Y = RoundFloatToInt(vector.Y);
      vector.Z = RoundFloatToInt(vector.Z);
   }
}

static void OSECommon_QuantizeScaled(FVector& vector,  int32 scale)
{
   using ScalarType = FVector::FReal;
   constexpr SIZE_T ScalarTypeSize = sizeof(ScalarType);
   using IntType = typename TSignedIntType<ScalarTypeSize>::Type;

   // Beyond 2^MaxExponentForScaling scaling cannot improve the precision as the next floating point value is at least 1.0 more. 
   constexpr uint32 MaxExponentForScaling = ScalarTypeSize == 4 ? 23U : 52U;
   constexpr ScalarType MaxValueToScale = ScalarType(IntType(1) << MaxExponentForScaling);

   FVector scaledVector = vector * scale;
   if(scaledVector.GetAbsMin() < MaxValueToScale)
   { 
      OSECommon_QuantizeUnscaled(scaledVector);
      vector = scaledVector / scale;
   }
   else
   {
      OSECommon_QuantizeUnscaled(vector);
   }
}

FVector_NetQuantize UOSECommon::PreQuantize(const FVector_NetQuantize& value)
{
   FVector_NetQuantize result = value;
   OSECommon_QuantizeUnscaled(result);
   return result;
}

FVector_NetQuantize10 UOSECommon::PreQuantize(const FVector_NetQuantize10& value)
{
   FVector_NetQuantize10 result = value;
   OSECommon_QuantizeScaled(result, 10);
   return result;
}

FVector_NetQuantize100 UOSECommon::PreQuantize(const FVector_NetQuantize100& value)
{
   FVector_NetQuantize100 result = value;
   OSECommon_QuantizeScaled(result,  100);
   return result;
}

void UOSECommon::PreQuantizeHitResult(FHitResult& hit)
{
   hit.Location = PreQuantize(hit.Location);
   hit.ImpactPoint = PreQuantize(hit.ImpactPoint);
   // TODO: normals
   hit.TraceStart = PreQuantize(hit.TraceStart);
   hit.TraceEnd = PreQuantize(hit.TraceEnd);
}

// static
AActor* UOSECommon::FindUltimateInstigator(AActor* originalActor)
{
   if (originalActor == nullptr)
   {
      return nullptr;
   }

   AActor* actor = originalActor;
   int32 numRounds = 0;
   // NB: Player pawns are their own instigator: if we find that, stop iterating and return it
   while (actor->GetInstigator() != nullptr && actor->GetInstigator() != actor)
   {
      actor = actor->GetInstigator();
      numRounds++;

      // unlikely to get a chain this long, so if we hit it, then we've probably found a cycle
      if (numRounds > 100)
      {
         checkf(false, TEXT("Found likely cycle following instigator chain, current actor: '%s'"), *actor->GetName());
         return nullptr;
      }
   }

   // We shouldn't return a null if we get here
   ensure(actor);
   return actor;
}

