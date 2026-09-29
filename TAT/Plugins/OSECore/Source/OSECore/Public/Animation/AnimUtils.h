// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AnimUtils.generated.h"

class UAnimInstance;
class UCurveVector;

/// Animation utilities
UCLASS()
class OSECORE_API UAnimUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   /// Extracts root motion position curves from the specified animation sequence.
   /// Existing curve object must exist, and its data is reset.
   /// Returns true on success.
   UFUNCTION(BlueprintCallable, Category = "Animation|OSE")
   static bool RootMotionCurveExtract(UCurveVector* outRootCurve, const class UAnimSequence* inAnimSequence);

   /// Creates and extracts root motion position curves from the specified animation sequence.
   /// The curve is created and returned on success, otherwise null is returned.
   UFUNCTION(BlueprintCallable, Category = "Animation|OSE")
   static UCurveVector* RootMotionCurveCreate(const class UAnimSequence* inAnimSequence);

   /// Processes the anim graph links specified in the data asset, replacing them as needed.
   UFUNCTION(BlueprintCallable, Category = "Animation|OSE", meta = (DefaultToSelf = "AnimInstance"))
   static bool LinkAnimGraphAssets(UAnimInstance* AnimInstance, const class UOSEAnimGraphLinkAsset* animLinkAsset);
};
