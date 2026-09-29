// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue5
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATSelfDestructRequirementComponent.generated.h"

/// A component that destroys its actor if it has a scene requirement that isn't met
///
/// Possibly relatively niche, but could be a useful-bolt-on when:
/// 1. It is fine/desirable to destroy the entire actor
/// 2. Loading and destroying the actor is not overly expensive)
/// 
/// (e.g. an ambient smart object without any geometry)
UCLASS( ClassGroup=(Custom), hideCategories=(Tags,Activation,Collision,Cooking), meta = (BlueprintSpawnableComponent))
class TAT_API UTATSelfDestructRequirementComponent : public UActorComponent
{
   GENERATED_BODY()

public:	
   UTATSelfDestructRequirementComponent();

protected:
   virtual void BeginPlay() override;

public:
#if WITH_EDITOR
   virtual void CheckForErrors() override final;

   // for vis adapter
   const FTATSceneRequirement* FindSceneRequirement() const;
#endif

private:
   UPROPERTY(EditInstanceOnly, Category=Requirement, meta=(ShowOnlyInnerProperties))
   FTATSceneRequirement _requirement;
};
