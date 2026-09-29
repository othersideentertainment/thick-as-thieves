// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_SkeletalMeshLODs.generated.h"

USTRUCT()
struct FTATSkeletalMeshLODValidatorEntry
{
   GENERATED_BODY()

   UPROPERTY()
   FString Folder;

   UPROPERTY()
   int32 MinLODs = 0;

   UPROPERTY()
   FString Message;
};

// Checks LODs of skeletal meshes in configured folders
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_SkeletalMeshLODs : public UEditorValidatorBase
{
   GENERATED_BODY()

public:
   UEditorValidator_SkeletalMeshLODs();

   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   TArray<FTATSkeletalMeshLODValidatorEntry> FoldersToValidate;

private:
   void OnOutfitMetadataTableChanged();

   int32 _outfitMetadataRequestId = INDEX_NONE;

   UPROPERTY()
   TObjectPtr<UDataTable> _outfitMetadataTable;

   TSet<FSoftObjectPath> _outfitMetadataExcludedAssets;
};
