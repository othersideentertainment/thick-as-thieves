// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_CircularBlueprintDependencies.generated.h"

// Checks that an asset does not indirectly have an indirect asset dependency on one of its child classes
// This is technically supported by Unreal, but can often lead to bugs during serialization
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_CircularBlueprintDependencies : public UEditorValidatorBase
{
   GENERATED_BODY()
public:
   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;
};
