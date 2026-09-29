// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_NaniteMeshes.generated.h"

// Checks that eligible meshes are nanite-enabled, and non-eligible ones are not
// Fairly conservative given exceptions, so not fully symmetric
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_NaniteMeshes : public UEditorValidatorBase
{
   GENERATED_BODY()

public:

   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   int32 MinTrianglesToWarnAboutNanite = 5000;

   // if the triangles don't share vertices, nanite is less able to optimize them
   UPROPERTY(Config)
   float MaxVertexToTriangleRatio = 2;

   UPROPERTY(Config)
   int32 MinTrianglesToWarnAboutVertexToTriangleRatio = 100;

   UPROPERTY(Config)
   TArray<FString> PathsToValidateNanite;

   // There are exceptions that can be contextual (e.g. interactables, due to (for now) lack of custom depth support for interaction highlight)
   UPROPERTY(Config)
   TArray<FString> PathsToSkipValidation;

   UPROPERTY(Config)
   bool AllowNaniteWithWPO = false;

   // whether to warn about non-nanite meshes with WPO
   UPROPERTY(Config)
   bool RequireNaniteWithWPO = false;
};
