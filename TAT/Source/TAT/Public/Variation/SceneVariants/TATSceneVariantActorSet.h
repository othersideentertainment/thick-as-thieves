// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATSceneRequirement.h"

// ue
#include "GameFramework/Actor.h"

#include "TATSceneVariantActorSet.generated.h"

// A nativized version of a BP_SceneVariant_MeshSet, which destroyed actors when its scene requirement is not met
UCLASS(Blueprintable)
class TAT_API ATATSceneVariantActorSet : public AActor
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATSceneVariantActorSet();

   virtual void BeginPlay() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default", meta = (ShowOnlyInnerProperties))
   FTATSceneRequirement SceneRequirement;

   /** Please add a variable description */
   UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category="Default")
   TArray<TObjectPtr<AActor>> Actors;

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<class UTATSceneVariantActorSetVisComponent> _visualizationComponent;

   UPROPERTY()
   TObjectPtr<class UTATSceneRequirementVisComponent> _requirementVisComponent;
#endif
};

UCLASS()
class TAT_API UTATSceneVariantActorSetVisComponent : public UActorComponent
{
   UTATSceneVariantActorSetVisComponent();

   GENERATED_BODY()
};
