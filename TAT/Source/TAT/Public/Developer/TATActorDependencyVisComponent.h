// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATActorDependencyVisComponent.generated.h"


// a visualization component just to visualize connections registered with the
// EditActorDependencySubsystem
UCLASS()
class TAT_API UTATActorDependencyVisComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATActorDependencyVisComponent();

#if WITH_EDITORONLY_DATA
   FName GroupKey;
   FColor Color = FColor::White;
   bool IsReversed = false;
   bool DrawForeground = true;
#endif
};
