// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_StaticMeshLODs.generated.h"

USTRUCT()
struct FTATStaticMeshLODValidatorEntry
{
   GENERATED_BODY()
   
   UPROPERTY()
   FString Folder;

   UPROPERTY()
   bool IncludingNanite = false;

   UPROPERTY()
   int MinLODs = 0;

   UPROPERTY()
   int MinTrianglesToCheck = 0;

   UPROPERTY()
   FString Message;
};

// Checks LODs of skeletal meshes in configured folders
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_StaticMeshLODs : public UEditorValidatorBase
{
   GENERATED_BODY()

public:

   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   TArray<FTATStaticMeshLODValidatorEntry> FoldersToValidate;
};
