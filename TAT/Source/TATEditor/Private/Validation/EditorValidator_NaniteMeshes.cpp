// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/EditorValidator_NaniteMeshes.h"

// ue5
#include "Engine/StaticMesh.h"
#include "NaniteSceneProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_NaniteMeshes)


#define LOCTEXT_NAMESPACE "AssetValidation"

bool UEditorValidator_NaniteMeshes::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   if (!asset->IsA<UStaticMesh>())
   {
      return false;
   }

   const FString pathName = asset->GetPathName(nullptr);
   return PathsToValidateNanite.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); }) 
         && !PathsToSkipValidation.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); });
}

EDataValidationResult UEditorValidator_NaniteMeshes::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   UStaticMesh* meshAsset = CastChecked<UStaticMesh>(asset);

   bool hasWpo = false;
   bool hasUnsupportedBlendMode = false;
   for (const FStaticMaterial& entry : meshAsset->GetStaticMaterials())
   {
      const UMaterialInterface* materialInterface = entry.MaterialInterface;
      if (!IsValid(materialInterface))
      {
         continue;
      }

      const UMaterial* material = materialInterface->GetMaterial_Concurrent();
      check(material != nullptr);

      hasWpo |= material->HasVertexPositionOffsetConnected();
      hasUnsupportedBlendMode |= !Nanite::IsSupportedBlendMode(material->GetBlendMode());
   }

   if (meshAsset->NaniteSettings.bEnabled)
   {
      if (hasWpo && !AllowNaniteWithWPO)
      {
         AssetFails(asset, LOCTEXT("NaniteValidator_NaniteWPO", "Nanite is enabled on a mesh using world-position offset. This is not recommended at this time."));
         return EDataValidationResult::Invalid;
      }

      if (hasUnsupportedBlendMode)
      {
         AssetFails(asset, LOCTEXT("NaniteValidator_NaniteMaterial", "Nanite is enabled on a mesh using an unsupported blend mode."));
         return EDataValidationResult::Invalid;
      }

      const float vertToTriRatio = meshAsset->GetNumNaniteVertices() / (float)meshAsset->GetNumNaniteTriangles();
      if (vertToTriRatio >= MaxVertexToTriangleRatio && meshAsset->GetNumNaniteTriangles() >= MinTrianglesToWarnAboutVertexToTriangleRatio)
      {
         AssetWarning(asset, FText::Format(LOCTEXT("NaniteValidator_NaniteMaterial", "Vertex to triangle ratio of {0}:1 >= {1}. Nanite is not as good at simplifying meshes that have lots of faceted normals. (see 'Faceted and Hard-edged normals' in https://docs.unrealengine.com/5.2/en-US/nanite-virtualized-geometry-in-unreal-engine/#workingwithnanite-enabledcontent) If there is a good reason for this, add it to the exclusion list in DefaultEngine.ini"), vertToTriRatio, MaxVertexToTriangleRatio));
      }
      
      const FMeshNaniteSettings& naniteSettings = meshAsset->NaniteSettings;
      const ENaniteFallbackTarget fallbackTarget = naniteSettings.FallbackTarget;
      if (fallbackTarget == ENaniteFallbackTarget::PercentTriangles && naniteSettings.FallbackPercentTriangles == 1.f)
      {
         AssetWarning(asset, LOCTEXT("NaniteValidator_NaniteFallbackTriangles", "Mesh has nanite fallback with 100% of triangles. That probably isn't what was intended"));
      }
      else if (fallbackTarget == ENaniteFallbackTarget::RelativeError && naniteSettings.FallbackRelativeError == 0.f)
      {
         AssetWarning(asset, LOCTEXT("NaniteValidator_NaniteFallbackError", "Mesh has nanite fallback with zero relative error. That probably isn't what was intended"));
      }
   }
   else if(meshAsset->GetNumTriangles(0) >= MinTrianglesToWarnAboutNanite)
   {
      if ((!hasWpo || RequireNaniteWithWPO) && !hasUnsupportedBlendMode)
      {
         AssetFails(asset, LOCTEXT("NaniteValidator_NaniteNotEnabled", "Nanite not enabled on eligible mesh. Please edit exclude list (or validation) if it should not be"));
         return EDataValidationResult::Invalid;
      }
   }

   AssetPasses(asset);
   return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
