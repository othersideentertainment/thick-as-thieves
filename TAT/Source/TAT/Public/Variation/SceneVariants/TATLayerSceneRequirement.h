// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATSceneRequirement.h"

// ue
#include "CoreMinimal.h"

#include "TATLayerSceneRequirement.generated.h"

// A scene requirement for the actors in a Layer to be not-destroyed
USTRUCT()
struct FTATLayerSceneRequirement
{
   GENERATED_BODY()

   // the name of the layer in the level (not data layer)
   UPROPERTY(EditAnywhere)
   FName Layer;

   // The requirement for the actors assigned to this layer to be used
   UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
   FTATSceneRequirement Requirement;
};
