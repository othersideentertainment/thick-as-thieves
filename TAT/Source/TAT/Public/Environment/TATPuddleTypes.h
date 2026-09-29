// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Abilities/TATGameplayEffectSetByCallerParam.h"

// ue
#include "CoreMinimal.h"

#include "TATPuddleTypes.generated.h"


/// The canonical puddle data - replicated to clients.
USTRUCT(BlueprintType)
struct TAT_API FTATPuddle
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   int32 PuddleId = INDEX_NONE;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   FVector Location = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   FVector Extent = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   FRotator Rotation = FRotator::ZeroRotator;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   float Health = 0.0f;

   /// The server time after which this puddle will be removed (to give it time to transition out)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Puddle")
   float ServerRemoveWorldTime = 0.0f;

   FORCEINLINE bool operator==(const FTATPuddle& rhs) const
   {
      return PuddleId == rhs.PuddleId
         && Location.Equals(rhs.Location)
         && Extent.Equals(rhs.Extent)
         && Rotation.EqualsOrientation(rhs.Rotation)
         && FMath::IsNearlyEqual(Health, rhs.Health, 0.0001f)
         && ServerRemoveWorldTime == rhs.ServerRemoveWorldTime;
   }

   bool CanApplyDamage(float damageAmount) const;
   bool ApplyDamage(float damageAmount, double serverWorldTimeSeconds, float lifetimeAfterRemove);
   FVector GetPositionAroundPuddle(float angleDeg, float extentMultiplier = 0.8f) const;
   FORCEINLINE bool IsPendingRemove() const { return ServerRemoveWorldTime > 0.0f; }
   FORCEINLINE float GetRemainingLifeSpan(double serverWorldTimeSeconds) const { return FMath::Max(0.0f, ServerRemoveWorldTime - serverWorldTimeSeconds); }
   FORCEINLINE FSphere GetBoundingSphere() const { return FSphere(Location, FMath::Max3(Extent.X, Extent.Y, Extent.Z)); }
};


UENUM(BlueprintType)
enum class ETATPuddleSurfaceAngle : uint8
{
   Floor = 0  UMETA(ToolTip = "Close to world up. Shows as a cyan arrow in the puddle debug visualizer (tat.puddle.debug 2)"),
   Ceiling    UMETA(ToolTip = "Close to world down. Shows as a magenta arrow in the puddle debug visualizer (tat.puddle.debug 2)"),
   Wall       UMETA(ToolTip = "On a vertical surface. Shows as a green arrow in the puddle debug visualizer (tat.puddle.debug 2)"),
   Angled     UMETA(ToolTip = "Any non-floor/ceiling/wall angles (eg. a 45 degree slope). Shows as a red arrow in the puddle debug visualizer (tat.puddle.debug 2)"),
   MAX        UMETA(Hidden)
};


/// Simple puddle transform, used to simplify computing multiple puddles and passing them around before spawning them
USTRUCT(BlueprintType)
struct TAT_API FTATPuddleTransform
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Location = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Extent = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FRotator Rotation = FRotator::ZeroRotator;

   FORCEINLINE FSphere GetBoundingSphere() const { return FSphere(Location, FMath::Max3(Extent.X, Extent.Y, Extent.Z)); }
};

UENUM(BlueprintType)
enum class ETATPuddleTargetFilter : uint8
{
   AllCharacters = 0      UMETA(DisplayName = "All Characters"),
   PlayerCharactersOnly   UMETA(DisplayName = "Player Characters Only"),
   NPCsOnly               UMETA(DisplayName = "NPCs Only"),
};

USTRUCT(BlueprintType)
struct TAT_API FTATPuddleGameplayEffect
{
   GENERATED_BODY()

   /// What targets should this gameplay effect be applied to?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Gameplay Effect")
   ETATPuddleTargetFilter TargetFilter = ETATPuddleTargetFilter::AllCharacters;

   /// Gameplay effect to apply to targets in a puddle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Gameplay Effect")
   TSubclassOf<UGameplayEffect> GameplayEffect;

   /// Should this gameplay effect be automatically removed from targets who leave the puddle area?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Gameplay Effect")
   bool AutoRemoveOnLeave = false;

   /// The level that the gameplay effect is applied at
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Gameplay Effect")
   float EffectLevel = 1.0f;

   /// SetByCaller parameters to pass to the gameplay effect
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Gameplay Effect")
   TArray<FTATGameplayEffectSetByCallerParam> SetByCallerParams;

   bool CanApplyToTarget(UAbilitySystemComponent* asc, const AActor* owner) const;
};
