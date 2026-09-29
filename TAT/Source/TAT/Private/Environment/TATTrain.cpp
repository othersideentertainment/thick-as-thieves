// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATTrain.h"

// ue
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTrain)

ATATTrain::ATATTrain()
{
   PrimaryActorTick.bCanEverTick = true;
   
   _rootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
   _rootComponent->Mobility = EComponentMobility::Movable;
   RootComponent = _rootComponent;
   
   _cabinISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISMCabin"));
   _cabinISMComponent->SetupAttachment(_rootComponent);
   _cabinISMComponent->bNavigationRelevant = false;
   _cabinISMComponent->bDisableCollision = true;
   _cabinISMComponent->Mobility = EComponentMobility::Movable;
   
   _cabinGlassISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISMCabinGlass"));
   _cabinGlassISMComponent->SetupAttachment(_rootComponent);
   _cabinGlassISMComponent->bNavigationRelevant = false;
   _cabinGlassISMComponent->bDisableCollision = true;
   _cabinGlassISMComponent->Mobility = EComponentMobility::Movable;
}

void ATATTrain::OnConstruction(const FTransform& transform)
{
   _cabinISMComponent->ClearInstances();
   _cabinGlassISMComponent->ClearInstances();
   Super::OnConstruction(transform);
   
   for (int i = 1; i <= _totalNumberOfCars; ++i)
   {
      _SetupCabin(i);
   }
}

void ATATTrain::BeginPlay()
{
   Super::BeginPlay();
   _HandleTrainReset();
}

FTransform ATATTrain::_CalculateTransformForSplinePosition(const float distanceOffset) const
{
   const FTransform transform = _splineComponent->GetTransformAtDistanceAlongSpline(_trackDistance + distanceOffset, ESplineCoordinateSpace::World);
 
   FVector calculatedOffset = _railOffset;
   calculatedOffset.Y = _reverseDirection ? calculatedOffset.Y * -1.f : calculatedOffset.Y;
   const FVector finalPosition = transform.GetLocation() + calculatedOffset;

   FRotator reversedRotation = transform.GetRotation().Rotator();
   reversedRotation.Yaw += 180.f;
   reversedRotation.Pitch = _reverseDirection ? reversedRotation.Pitch * -1.f : reversedRotation.Pitch;

   const FRotator finalRotation = _reverseDirection ? reversedRotation : transform.GetRotation().Rotator();
   
   return FTransform(finalRotation, finalPosition);
}

float ATATTrain::_GetOffsetFromIndex(const int index) const
{
   if (_reverseDirection)
   {
      return _initialOffset * -1.f - (index * _carLength * -1.f);
   }
   return _initialOffset - (index * _carLength);
}

void ATATTrain::_SetupCabin(const int index) const
{
   FVector calculatedPosition = FVector::ZeroVector;
   calculatedPosition.X = _GetOffsetFromIndex(index);
   _cabinISMComponent->AddInstance(FTransform(FRotator::ZeroRotator, calculatedPosition));
   _cabinGlassISMComponent->AddInstance(FTransform(FRotator::ZeroRotator, calculatedPosition));
}

void ATATTrain::_HandleTrainMovement(const float deltaSeconds)
{
   const float speed = (_reverseDirection ? _trainSpeed * -1.f : _trainSpeed) * deltaSeconds;
   _trackDistance += speed;
   
   if (_isTrackLooping)
   {
      const float splineLength = _splineComponent->GetSplineLength();
      if (_reverseDirection)
      {
         if (_trackDistance < 0.f)
         {
            _trackDistance = _trackDistance + splineLength;
         }
      }
      else
      {
         if (_trackDistance > splineLength)
         {
            _trackDistance = _trackDistance - splineLength;
         }
      }
   }
}

void ATATTrain::_HandleEnginePosition()
{
   const FTransform engineTransform = _CalculateTransformForSplinePosition(0.f);
   SetActorLocationAndRotation(engineTransform.GetLocation(), engineTransform.GetRotation());
}

void ATATTrain::_HandleCabinPositions() const
{
   TArray<FTransform> cabinTransforms;
   for (int i = 1; i <= _totalNumberOfCars; ++i)
   {
      cabinTransforms.Add(_CalculateTransformForSplinePosition(_GetOffsetFromIndex(i)));
   }
   if (cabinTransforms.Num() > 0)
   {
      _cabinISMComponent->BatchUpdateInstancesTransforms(
         0,
         MakeArrayView(cabinTransforms),
         true);
      _cabinGlassISMComponent->BatchUpdateInstancesTransforms(
         0,
         MakeArrayView(cabinTransforms),
         true);
   }
}

void ATATTrain::_HandleTrainReset()
{
   if (_reverseDirection)
   {
      _trackDistance = _splineComponent->GetSplineLength();
   }
   else
   {
      _trackDistance = 0.f;
   }
   SetActorTickEnabled(true);
}

void ATATTrain::_SetupResetTimer()
{
   const float delayTime = (_splineComponent->GetSplineLength() / _trainSpeed) * _delayMultiplier;
   GetWorld()->GetTimerManager().SetTimer(_trainResetTimerHandle,
                                          FTimerDelegate::CreateUObject(this, &ThisClass::_HandleTrainReset),
                                          delayTime,
                                          false);
}

void ATATTrain::Tick(const float deltaSeconds)
{
   Super::Tick(deltaSeconds);
   
   if (_splineComponent == nullptr)
      return;

   _HandleTrainMovement(deltaSeconds);
   _HandleEnginePosition();
   _HandleCabinPositions();
   
   if (_isTrackLooping == false)
   {
      const float carTotalLength = _carLength * _totalNumberOfCars;
      if (_reverseDirection)
      {
         const float remainingDistance = _trackDistance + carTotalLength;
         if (remainingDistance <= 0.f)
         {
            SetActorTickEnabled(false);
            _SetupResetTimer();
         }
      }
      else
      {
         const float remainingDistance = _trackDistance - carTotalLength;
         if (remainingDistance >= _splineComponent->GetSplineLength())
         {
            SetActorTickEnabled(false);
            _SetupResetTimer();
         }
      }
   }
}
