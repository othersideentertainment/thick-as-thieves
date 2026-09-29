// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_StaticActorBlueprints.generated.h"

// Checks that eligible meshes are nanite-enabled, and non-eligible ones are not
// Fairly conservative given exceptions, so not fully symmetric
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_StaticActorBlueprints : public UEditorValidatorBase
{
   GENERATED_BODY()

public:

   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   TArray<FString> PathsToValidate;

   // There are exceptions that can be contextual
   UPROPERTY(Config)
   TArray<FString> PathsToSkipValidation;

   UPROPERTY(Config)
   TArray<TSoftClassPtr<AActor>> BaseClassesToSkip;
};
