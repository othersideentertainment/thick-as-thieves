// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "UObject/Interface.h"

// TAT
#include "AIPathInterface.generated.h"

UENUM(BlueprintType)
enum class EPatrolLoopType : uint8
{
   Once,
   PingPong,
   Cycle
};

USTRUCT(BlueprintType)
struct FNextPointData
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   bool forwardMovementDirection { false };

   UPROPERTY(BlueprintReadOnly)
   int nextIndexID { 0 }; 
};

UINTERFACE()
class TAT_API UAIPathInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API IAIPathInterface
{
   GENERATED_BODY()

public:
   virtual int32 GetNumPoints() const = 0;
   virtual FVector GetPointLocationLocalSpace(int32 pointIndex) const = 0;
   virtual FVector GetPointLocationWorldSpace(int32 pointIndex) const = 0;
   virtual EPatrolLoopType GetLoopType() const = 0;
   virtual FNextPointData GetNextPoint(int32 currentPoint, bool currentDirection) const = 0;
#if WITH_EDITOR
   virtual FColor GetColorForPoint(int32 pointIndex) const = 0;
#endif
};
