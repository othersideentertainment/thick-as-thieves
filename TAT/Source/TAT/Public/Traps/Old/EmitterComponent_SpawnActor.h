// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Traps/Old/SimpleTrapEmitterComponent.h"


#include "EmitterComponent_SpawnActor.generated.h"

USTRUCT()
struct FSpawnActorEmitterEditorVis
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category = EditorVisualization)
   FColor Color = FColor::Red;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Arrow")
   bool bDrawArrow = false;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Arrow", Meta = (EditCondition = bDrawArrow))
   float ArrowLength = 0.0f;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Sphere")
   bool bDrawSphere = false;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Sphere", Meta = (EditCondition = bDrawSphere))
   FVector SphereOffset = FVector(ForceInit);

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Sphere", Meta = (EditCondition = bDrawSphere))
   float SphereRadius = 0.0f;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Box")
   bool bDrawBox = false;

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Box", Meta = (EditCondition = bDrawBox))
   FVector BoxOffset = FVector(ForceInit);

   UPROPERTY(EditDefaultsOnly, Category = "EditorVisualization|Box", Meta = (EditCondition = bDrawBox))
   FVector BoxExtents = FVector(ForceInit);
};

// A trap emitter component for spawning actors
// Does not actually spawn actors directly, but provides utilities for positioning the spawn location,
// and editor-only visualizations of it
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UEmitterComponent_SpawnActor_Old : public USimpleTrapEmitterComponent_Old
{
   GENERATED_BODY()

public:   
   // Sets default values for this component's properties
   UEmitterComponent_SpawnActor_Old();


   //virtual void OnTriggered_Implementation() override;

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector SpawnLocation;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator SpawnRotation;

   UFUNCTION(BlueprintPure)
   FTransform GetSpawnTransform() const;

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly, Category = EditorVisualization, meta=(ShowOnlyInnerProperties))
   FSpawnActorEmitterEditorVis EditorVisualizationConfig;
#endif
      
};
