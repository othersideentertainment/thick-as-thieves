// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OSEFallbackPingTargetStrategy.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOSEFallbackPingTargetStrategy : public UInterface
{
   GENERATED_BODY()
};


// An interface that
class OSECORE_API IOSEFallbackPingTargetStrategy
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual bool FindFallbackPingTarget(const FVector& startPos, const FVector& endPos, AActor*& outPingable, FHitResult& outHitResult) const = 0;
};


UCLASS()
class OSECORE_API UOSEPingFallbackFunctions : public UObject
{
   GENERATED_BODY()

   // Dispatches to interface implementations on the source actor
   UFUNCTION(BlueprintCallable, Category="Ping System")
   static bool FindFallbackPingTarget(const AActor* sourceActor, const FVector& startPos, const FVector& endPos, AActor*& outPingable, FHitResult& outHitResult);
};
