// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATSceneRequirementVisComponent.generated.h"

struct FTATSceneRequirement;

// A component for putting on actor's with SceneRequirement properties, so that the requirements will be visualized in editor when selected
UCLASS( ClassGroup=(Custom), meta=(DisplayName="SceneRequirementVisualizer", BlueprintSpawnableComponent))
class TAT_API UTATSceneRequirementVisComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATSceneRequirementVisComponent();

   // Whether it should separately validate scene requirements
   // Only set from code, since the assumption is that only code would do its own validation
   bool ShouldValidate = true;

#if WITH_EDITOR
   // for vis adapter
   const FTATSceneRequirement* FindSceneRequirement() const;

   virtual void CheckForErrors() override;
#endif
};
