// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "EditorValidatorBase.h"
#include "EditorValidator_MeshCollisionAOE.generated.h"


/// Something
UCLASS(Config=Editor)
class TATEDITOR_API UEditorValidator_MeshCollisionAOE : public UEditorValidatorBase
{
   GENERATED_BODY()

public:
   UEditorValidator_MeshCollisionAOE();

protected:
   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   FName BlockAOEProfile;

   UPROPERTY(Config)
   FName AllowAOEProfile;

   UPROPERTY(Config)
   TArray<FString> PathsToExcludeBlockingAOE;
};
