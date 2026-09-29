// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

#include "PatrolPathComponent.generated.h"

class APatrolPath;
class AWatchPath;
class IAIPathInterface;

// A mostly-editor-only component for editor visualizations of a patrol path via:
// * SceneProxy for visualizing component when not selected
// * ComponentVisualizer for visualizing component when selected (and maybe some other bits)
UCLASS()
class TAT_API UPatrolPathComponent : public UPrimitiveComponent
{
   GENERATED_BODY()

public:
   UPatrolPathComponent();

#if WITH_EDITOR
   virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

   virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
#endif

protected:

   IAIPathInterface* _GetPath() const;
};

