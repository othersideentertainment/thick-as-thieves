// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Disguise/TATDisguiseTargetInterface.h"

// ue
#include "Engine/SkinnedAssetCommon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDisguiseTargetInterface)

// static
USkeletalMesh* ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(USkeletalMesh* meshAsset, TArray<UMaterialInterface*>& outOverrideMaterials)
{
   outOverrideMaterials.Reset();
   if (meshAsset == nullptr)
   {
      return nullptr;
   }
   const TArray<FSkeletalMaterial>& skeletalMaterials = meshAsset->GetMaterials();
   outOverrideMaterials.SetNum(skeletalMaterials.Num());
   for (int32 i = 0; i < skeletalMaterials.Num(); i++)
   {
      outOverrideMaterials[i] = skeletalMaterials[i].MaterialInterface;
   }
   return meshAsset;
}

// static
USkeletalMesh* ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(USkeletalMeshComponent* meshComponent, TArray<UMaterialInterface*>& outOverrideMaterials)
{
   if (meshComponent == nullptr)
   {
      outOverrideMaterials.Reset();
      return nullptr;
   }
   outOverrideMaterials = meshComponent->GetMaterials();
   return meshComponent->GetSkeletalMeshAsset();
}
