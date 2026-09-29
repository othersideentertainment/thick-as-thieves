// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/EditorValidator_MixedTransparencyMeshes.h"

// ue5
#include "Engine/StaticMesh.h"
#include "NaniteSceneProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_MixedTransparencyMeshes)


#define LOCTEXT_NAMESPACE "AssetValidation"

bool UEditorValidator_MixedTransparencyMeshes::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   UStaticMesh* meshAsset = Cast<UStaticMesh>(asset);
   if (meshAsset == nullptr)
   {
      return false;
   }
   
   if (meshAsset->GetNumTriangles(0) < MinTrianglesToWarnAboutTransparency)
   {
      return false;
   }

   const FString pathName = asset->GetPathName(nullptr);
   return PathsToValidate.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); }) 
         && !PathsToSkipValidation.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); });
}

EDataValidationResult UEditorValidator_MixedTransparencyMeshes::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   UStaticMesh* meshAsset = CastChecked<UStaticMesh>(asset);

   bool hasOpaqueOrMasked = false;
   bool hasOther = false;
   for (const FStaticMaterial& entry : meshAsset->GetStaticMaterials())
   {
      const UMaterialInterface* materialInterface = entry.MaterialInterface;
      if (!IsValid(materialInterface))
      {
         continue;
      }

      const UMaterial* material = materialInterface->GetMaterial_Concurrent();
      check(material != nullptr);

      switch (material->GetBlendMode())
      {
         case BLEND_Opaque:
         case BLEND_Masked:
            hasOpaqueOrMasked = true;
            break;
         default:
            hasOther = true;
      }
   }

   if (hasOpaqueOrMasked && hasOther)
   {
      AssetFails(asset, LOCTEXT("MixedTransparencyMeshesValidator_Mixed", "The mesh has a mix of both opaque/masked and transparent materials. This prevents enabling nanite for the opaque parts."));
      return EDataValidationResult::Invalid;
   }

   AssetPasses(asset);
   return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
