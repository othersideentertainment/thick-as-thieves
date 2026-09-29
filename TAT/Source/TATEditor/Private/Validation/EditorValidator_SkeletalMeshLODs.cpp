// (c) 2022-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_SkeletalMeshLODs.h"

// TAT
#include "CharacterCustomization/TATCharacterOutfits.h"
#include "Developer/TATOutfitSettings.h"

// ue5
#include "Engine/SkeletalMesh.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_SkeletalMeshLODs)

UEditorValidator_SkeletalMeshLODs::UEditorValidator_SkeletalMeshLODs()
{
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      const UTATOutfitSettings* outfitSettings = GetDefault<UTATOutfitSettings>();
      if (outfitSettings)
      {
         _outfitMetadataRequestId = outfitSettings->OutfitMetadataTable.ToSoftObjectPath().LoadAsync(
            FLoadSoftObjectPathAsyncDelegate::CreateWeakLambda(this, [this](const FSoftObjectPath&, UObject* loadedAsset)
            {
               _outfitMetadataTable = Cast<UDataTable>(loadedAsset);
               if (_outfitMetadataTable)
               {
                  _outfitMetadataTable->OnDataTableChanged().AddUObject(this, &ThisClass::OnOutfitMetadataTableChanged);
                  OnOutfitMetadataTableChanged();
               }

               _outfitMetadataRequestId = INDEX_NONE;
            }));
      }
   }
}

bool UEditorValidator_SkeletalMeshLODs::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   if (!asset->IsA<USkeletalMesh>())
   {
      return false;
   }

   if (_outfitMetadataRequestId != INDEX_NONE)
   {
      FlushAsyncLoading(_outfitMetadataRequestId);
   }

   if (_outfitMetadataExcludedAssets.Contains(assetData.GetSoftObjectPath()))
   {
      return false;
   }

   return true;
}

EDataValidationResult UEditorValidator_SkeletalMeshLODs::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   FString pathName = asset->GetPathName(nullptr);
   const FTATSkeletalMeshLODValidatorEntry* entry = FoldersToValidate.FindByPredicate([&pathName](const FTATSkeletalMeshLODValidatorEntry& entry)
   {
      return pathName.Contains(entry.Folder);
   });

   if (entry == nullptr)
   {
      AssetPasses(asset);
      return EDataValidationResult::Valid;
   }

   const USkeletalMesh& mesh = *CastChecked<USkeletalMesh>(asset);

   if (mesh.GetLODNum() < entry->MinLODs)
   {
      AssetFails(asset, FText::FormatOrdered(INVTEXT("Not enough LODs ({0} < {1}): {2}"), mesh.GetLODNum(), entry->MinLODs, FText::FromString(entry->Message)));
      return EDataValidationResult::Invalid;
   }

   AssetPasses(asset);
   return EDataValidationResult::Valid;
}

void UEditorValidator_SkeletalMeshLODs::OnOutfitMetadataTableChanged()
{
   _outfitMetadataExcludedAssets.Reset();

   // Exclude first person character outfit meshes from LOD validation
   _outfitMetadataTable->ForeachRow<FTATOutfitsMetadataTableRow>(TEXT("EditorValidator_SkeletalMeshLODs"), [this](const FName&, const FTATOutfitsMetadataTableRow& value)
   {
      if (!value.OutfitSkeletalMesh1PUpperBody.IsNull())
      {
         _outfitMetadataExcludedAssets.Add(value.OutfitSkeletalMesh1PUpperBody.ToSoftObjectPath());
      }

      if (!value.OutfitSkeletalMesh1PLowerBody.IsNull())
      {
         _outfitMetadataExcludedAssets.Add(value.OutfitSkeletalMesh1PLowerBody.ToSoftObjectPath());
      }
   });
}
