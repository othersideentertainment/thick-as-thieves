// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Patrol/AIPathInterface.h"
#include "AI/SmartObjects/TATAmbientSmartObject.h"

// ue4
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "PatrolPath.generated.h"

class UPatrolPathComponent;
class IInteractableInterface;

UENUM(BlueprintType)
enum class EPatrolPointType : uint8
{
   PatrolToLocation,
   WatchPath,
   Interact,
   SmartObjectInteraction,
};


USTRUCT(BlueprintType)
struct FWatchPoint
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MakeEditWidget = ""))
   FVector LookAtPosition = FVector(ForceInit);

   /// The number of seconds to wait at this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = 0))
   float WaitSeconds = 0.0f;

   /// Random deviation to add/subtract to WaitSeconds.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = 0))
   float WaitDeviation = 0.0f;
};

UCLASS()
class TAT_API AWatchPath : public AActor, public IAIPathInterface
{
   GENERATED_BODY()

public:
   AWatchPath();

#if WITH_EDITOR
   virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

   // From IAIPathInterface
   virtual int32 GetNumPoints() const override;
   virtual FVector GetPointLocationLocalSpace(int32 pointIndex) const override;
   virtual FVector GetPointLocationWorldSpace(int32 pointIndex) const override;
   
   UFUNCTION(BlueprintCallable)
   virtual FNextPointData GetNextPoint(int32 currentPoint, bool currentDirection) const override;
   
   virtual EPatrolLoopType GetLoopType() const override { return LoopType; }
#if WITH_EDITOR
   virtual FColor GetColorForPoint(int32 pointIndex) const override;
#endif

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TArray<FWatchPoint> Points;

   UPROPERTY(EditAnywhere)
   EPatrolLoopType LoopType = EPatrolLoopType::Cycle;

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   UPatrolPathComponent* PathComponent;
#endif
};

USTRUCT(BlueprintType)
struct FPatrolPoint
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EPatrolPointType  PatrolPointType = EPatrolPointType::PatrolToLocation;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MakeEditWidget = ""))
   FVector Position = FVector(ForceInit);

   /// The number of seconds to wait at this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = 0))
   float WaitSeconds = 0.0f;

   /// Random deviation to add/subtract to WaitSeconds.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = 0))
   float WaitDeviation = 0.0f;

   /// Optional path of points to watch while waiting at this point.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "PatrolPointType == EPatrolPointType::WatchPath", EditConditionHides))
   AWatchPath* WatchPath = nullptr;

   /// Optional interactable to interact with upon reaching this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "PatrolPointType == EPatrolPointType::Interact", EditConditionHides))
   TScriptInterface<IInteractableInterface> Interactable;
   
   /// Optional smartobject to interact with upon reaching this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "PatrolPointType == EPatrolPointType::SmartObjectInteraction", EditConditionHides))
   ATATAmbientSmartObject* SmartObjectComponent = nullptr;
   
   /// Optional smartobject to interact with upon reaching this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "PatrolPointType == EPatrolPointType::SmartObjectInteraction", EditConditionHides))
   int SlotIndex { 0 };

   /// Optional facing to point in when reaching this position.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "PatrolPointType == EPatrolPointType::PatrolToLocation", EditConditionHides))
   bool HasFacing = false;
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "PatrolPointType == EPatrolPointType::PatrolToLocation && HasFacing", EditConditionHides))
   FRotator Facing = FRotator(ForceInit);
};

/// An actor that represents static data about an AI patrol path in a level.
UCLASS()
class TAT_API APatrolPath : public AActor, public IAIPathInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   APatrolPath();

#if WITH_EDITOR
   virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

   // From IAIPathInterface
   virtual int32 GetNumPoints() const override;
   virtual FVector GetPointLocationLocalSpace(int32 pointIndex) const override;
   virtual FVector GetPointLocationWorldSpace(int32 pointIndex) const override;
   virtual EPatrolLoopType GetLoopType() const override { return LoopType; }

   UFUNCTION(BlueprintCallable)
   virtual FNextPointData GetNextPoint(int32 currentPoint, bool currentDirection) const override;
#if WITH_EDITOR
   virtual FColor GetColorForPoint(int32 pointIndex) const override;
#endif

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TArray<FPatrolPoint> Points;

   UPROPERTY(EditAnywhere)
   EPatrolLoopType LoopType = EPatrolLoopType::Cycle;
   
#if WITH_EDITORONLY_DATA
   UPROPERTY()
   UPatrolPathComponent* PathComponent = nullptr;
#endif
};

