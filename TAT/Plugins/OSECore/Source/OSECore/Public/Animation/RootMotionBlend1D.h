// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RootMotionBlend1D.generated.h"


// @TODO: I originally wanted this to be a subclass of UBlendSpace1D. The animation sequences in the blend space
// would automatically have their root motion extracted and stored, and properly blended based on weights.
// Unfortunately, as of 4.22 at least, it is not possible to subclass UBlendSpace1D without engine modifications.

UCLASS()
class URootMotionBlend1D : public UDataAsset
{
   GENERATED_BODY()

protected:

   /// The source blend space asset
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   TSubclassOf< class UBlendSpace1D > BlendSpace;

   UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = RootMotion)
   TArray< TSubclassOf< class UCurveVector > > RootMotionCurves;
};

