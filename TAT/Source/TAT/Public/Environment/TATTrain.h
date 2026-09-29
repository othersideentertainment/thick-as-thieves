// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"

#include "TATTrain.generated.h"

class USplineComponent;

UCLASS()
class TAT_API ATATTrain : public AActor
{
   GENERATED_BODY()

public:
   ATATTrain();

   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;
protected:
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TObjectPtr<USceneComponent> _rootComponent { nullptr };
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TObjectPtr<UInstancedStaticMeshComponent> _cabinISMComponent { nullptr };
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TObjectPtr<UInstancedStaticMeshComponent> _cabinGlassISMComponent { nullptr };
   
   UPROPERTY(Transient, BlueprintReadWrite)
   TObjectPtr<USplineComponent> _splineComponent { nullptr };
   
   UPROPERTY(EditAnywhere)
   FVector _railOffset { FVector(0, 225.f, -125.f) };
   
   UPROPERTY(EditAnywhere)
   bool _reverseDirection { false };

   UPROPERTY(EditAnywhere)
   float _carLength { 625.f };
   
   UPROPERTY(EditAnywhere)
   float _initialOffset { 125.f };
   
   float _trackDistance { 0.f };

   UPROPERTY(EditAnywhere)
   float _trainSpeed { 1000.f };
   UPROPERTY(EditAnywhere)
   bool _isTrackLooping { false };
   UPROPERTY(EditAnywhere)
   float _delayMultiplier { 2.f };
   UPROPERTY(EditAnywhere)
   int _totalNumberOfCars { 3 };
   
   FTimerHandle _trainResetTimerHandle;

   UFUNCTION(BlueprintCallable)
   void _SetupCabin(int index) const;
   void _HandleTrainMovement(float deltaSeconds);
   void _HandleEnginePosition();
   void _HandleCabinPositions() const;
   UFUNCTION()
   void _HandleTrainReset();
   void _SetupResetTimer();
   
   FTransform _CalculateTransformForSplinePosition(float distanceOffset) const;
   float _GetOffsetFromIndex(int index) const;
};
