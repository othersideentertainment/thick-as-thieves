// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"

class UTATSceneAsset;
class UTATSceneSetAsset;
class UTATSceneVariantConfig;
struct FTATSceneVariantSelectionParams;


namespace TATSceneSelection
{
   void SelectVariants(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler);
}
