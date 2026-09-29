// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class UTATSceneVariantConfig;
struct FTATPendingClueSource;

namespace TATSceneVariantClueSource
{
   void TAT_API AddClueSourcesFromVariants(TConstArrayView<const UTATSceneVariantConfig*> variants, TArray<FTATPendingClueSource>& outSources);
}
