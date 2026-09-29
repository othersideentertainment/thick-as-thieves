// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATSimpleDamageableInterface.generated.h"

struct FTATDamageWithType;

// A simple context struct for the source of the damage
USTRUCT(BlueprintType)
struct FTATSimpleDamageSource
{
   GENERATED_BODY()
   
   FTATSimpleDamageSource(AActor* sourceActor, const FVector& origin, const FHitResult& hitResult)
   : SourceActor(sourceActor), Origin(origin), HitResult(hitResult)
   {}
   FTATSimpleDamageSource() = default;

   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<AActor> SourceActor;
   UPROPERTY(BlueprintReadOnly)
   FVector Origin = FVector::ZeroVector;
   UPROPERTY(BlueprintReadOnly)
   FHitResult HitResult;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATSimpleDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/// An interface for actors that respond to damage in simple ways, but don't need/want a full ASC like characters/breakables.
///
/// Not meant to be used for super-fancy stuff
class TAT_API ITATSimpleDamageableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   // Since being damageable opts into slightly more expensive codepaths, allow actors to opt out in case
   // (for example) the interface implementation is in a base class
   UFUNCTION(BlueprintNativeEvent, Category = "Damage")
   bool CanHandleDamage() const;
   virtual bool CanHandleDamage_Implementation() const { return true; }

   UFUNCTION(BlueprintNativeEvent, Category = "Damage")
   void AuthorityHandleDamage(const FTATDamageWithType& damage, const FTATSimpleDamageSource& damageSource);
};
