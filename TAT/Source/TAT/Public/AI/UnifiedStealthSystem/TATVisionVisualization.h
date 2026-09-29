// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TATVisionVisualization.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class TAT_API UTATVisionVisualization : public UStaticMeshComponent
{
   GENERATED_BODY()

public:
   UTATVisionVisualization(const FObjectInitializer& objectInitializer);
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
private:
   UPROPERTY(EditAnywhere)
   float _guardVisionRange { 3000.f };
   UPROPERTY(EditAnywhere)
   float _minGuardVisionRange { 250.f };
   
   UPROPERTY(EditAnywhere)
   TObjectPtr<UMaterial> _visualizationMaterial { nullptr };

   UPROPERTY(Transient)
   TObjectPtr<UMaterialInstanceDynamic> _visualizationMaterialInstance { nullptr };
};
