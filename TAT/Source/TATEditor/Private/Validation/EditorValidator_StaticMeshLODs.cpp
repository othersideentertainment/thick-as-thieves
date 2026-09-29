// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_StaticMeshLODs.h"

// ue5
#include "Engine/StaticMesh.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_StaticMeshLODs)


bool UEditorValidator_StaticMeshLODs::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   if (!asset->IsA<UStaticMesh>())
      return false;

   return true;
}

EDataValidationResult UEditorValidator_StaticMeshLODs::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   FString pathName = asset->GetPathName(nullptr);
   const UStaticMesh& mesh = *CastChecked<UStaticMesh>(asset);
   const FTATStaticMeshLODValidatorEntry* entry = FoldersToValidate.FindByPredicate([&pathName, &mesh](const FTATStaticMeshLODValidatorEntry& entry) {
      return pathName.Contains(entry.Folder) && (entry.IncludingNanite || !mesh.NaniteSettings.bEnabled) && (mesh.GetNumTriangles(0) >= entry.MinTrianglesToCheck);
   });
   if (entry == nullptr)
   {
      AssetPasses(asset);
      return EDataValidationResult::Valid;
   }


   if (mesh.GetNumLODs() < entry->MinLODs)
   {
      AssetFails(asset, FText::FormatOrdered(INVTEXT("Not enough LODs ({0} < {1}): {2}"), mesh.GetNumLODs(), entry->MinLODs, FText::FromString(entry->Message)));
      return EDataValidationResult::Invalid;
   }

   AssetPasses(asset);
   return EDataValidationResult::Valid;
}
