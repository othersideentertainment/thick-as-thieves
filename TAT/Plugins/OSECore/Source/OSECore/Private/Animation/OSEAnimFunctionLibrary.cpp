// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/OSEAnimFunctionLibrary.h"

// ue
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogOSEAnimFunctionLibrary, Log, All);

bool UOSEAnimFunctionLibrary::GetMontageSectionStartAndEndTime(const UAnimMontage* montage, FName sectionName, float& outStartTime, float& outEndTime)
{
   outStartTime = 0;
   outEndTime = 0;
   if (!montage)
   {
      UE_LOG(LogOSEAnimFunctionLibrary, Error, TEXT("GetMontageSectionStartAndEndTime() called without montage!"));
      return false;
   }

   if (sectionName.IsNone())
   {
      UE_LOG(LogOSEAnimFunctionLibrary, Error, TEXT("GetMontageSectionStartAndEndTime() called with invalid section name!"));
      return false;
   }

   const int32 sectionIndex = montage->GetSectionIndex(sectionName);
   if (sectionIndex == INDEX_NONE)
   {
      UE_LOG(LogOSEAnimFunctionLibrary, Error, TEXT("GetMontageSectionStartAndEndTime() called with section name %s not found in montage!"), *sectionName.ToString());
      return false;
   }

   if (!montage->IsValidSectionIndex(sectionIndex))
   {
      UE_LOG(LogOSEAnimFunctionLibrary, Error, TEXT("GetMontageSectionStartAndEndTime() called with section index %d out of bounds! (only %d sections found)")
         , sectionIndex
         , montage->GetNumSections());
      return false;
   }

   montage->GetSectionStartAndEndTime(sectionIndex, outStartTime, outEndTime);
   return true;
}

void UOSEAnimFunctionLibrary::ForcePoseUpdate(USkeletalMeshComponent* skeletalMesh)
{
   if (!skeletalMesh)
   {
      UE_LOG(LogOSEAnimFunctionLibrary, Error, TEXT("ForcePoseUpdate() called without valid skeletal mesh component!"));
      return;
   }

   constexpr bool needsValidRootMotion = false;
   constexpr float deltaTime = 0.f;
   skeletalMesh->TickAnimation(deltaTime, needsValidRootMotion);
   skeletalMesh->RefreshBoneTransforms();
}
