// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATToolFunctionLibrary.h"

// tat
#include "Developer/TATToolSettings.h"
#include "Tools/TATToolTags.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetComponent.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "AI/Perception/OSEAISightInterface.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogTATToolFunctionLibrary, Log, All)

static TAutoConsoleVariable<float> CVarTATWorldActorAdjustmentTraceHeightRatio(
   TEXT("TAT.WorldActorAdjustment.TraceHeightRatio"),
   0.9f,
   TEXT("At what fraction of the estimated height should we perform the traces to determine how much horizontal room we have")
);

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<int32> CVarTATWorldActorAdjustmentDebugDraw(
   TEXT("TAT.WorldActorAdjustment.DebugDraw"),
   0,
   TEXT("Whether to debug draw the traces/constraints for adjusting world actor placement")
);
#endif

namespace ToolHelpers
{
   struct FLineBoxIntersection
   {
      bool Hit = false;
      FVector Location = FVector::ZeroVector;
      FVector Normal = FVector::UpVector;
      float Time = 0.0f;

      FORCEINLINE explicit operator bool() const { return Hit; }
   };

   bool LineExtentBoxIntersection(FLineBoxIntersection& outIntersection, const FVector& lineA, const FVector& lineB, const FBox& box)
   {
      outIntersection.Hit = FMath::LineExtentBoxIntersection(box, lineA, lineB, FVector::ZeroVector, outIntersection.Location, outIntersection.Normal, outIntersection.Time);
      if (!outIntersection.Hit)
      {
         outIntersection = {};
         return false;
      }
      return outIntersection.Hit;
   }

   /// A bounding box represented by a transform and an extent. Supports being rotated.
   struct FCharBoundingBox
   {
      FTransform Transform = FTransform::Identity;
      FVector Extent = FVector::ZeroVector;

      FCharBoundingBox() = default;
      FCharBoundingBox(const FCharBoundingBox&) = default;
      FCharBoundingBox(const FVector& location, const FQuat& ori, const FVector& extent, const FVector& scale = FVector::One())
         : Transform(ori, location, scale)
         , Extent(extent)
      {
      }

      FORCEINLINE FVector GetLocation() const { return Transform.GetLocation(); }
      FORCEINLINE FVector GetExtents() const { return Extent; }
      FORCEINLINE float GetRadius() const { return FMath::Max3(Extent.X, Extent.Y, Extent.Z); }
      FORCEINLINE bool IsValid() const { return Extent.X > 0 && Extent.Y > 0 && Extent.Z > 0; }

      /// Finds the intersection between a line segment and this bounding box
      bool FindIntersectionWithLine(FLineBoxIntersection& outIntersection, const FVector& lineA, const FVector& lineB) const
      {
         // Transform the line to box-space so we can treat it as an AABB
         if (LineExtentBoxIntersection(outIntersection, Transform.InverseTransformPosition(lineA), Transform.InverseTransformPosition(lineB), FBox(-Extent, Extent)))
         {
            // Transform the intersection back to world space
            outIntersection.Location = Transform.TransformPosition(outIntersection.Location);
            outIntersection.Normal = Transform.TransformVector(outIntersection.Normal);
         }
         return outIntersection.Hit;
      }

      /// Returns a new bounding box with a rotation applied, optionally rotated around a pivot point
      FCharBoundingBox Rotate(const FQuat& ori, const FVector& relativePivot = FVector::ZeroVector) const
      {
         FCharBoundingBox result = *this;
         result.Transform.SetRotation(Transform.GetRotation() * ori);
         // Calculate the world space position of the pivot point before and after the rotation.
         const FVector oldPivotWorld = Transform.TransformPosition(relativePivot);
         const FVector newPivotWorld = result.Transform.TransformPosition(relativePivot);
         result.Transform.SetTranslation(Transform.GetTranslation() + (oldPivotWorld - newPivotWorld));
         return result;
      }

      void DrawDebug(UWorld* world, const FColor& color, float duration, bool showAABB = false, const FColor& aabbColor = FColor::White) const
      {
         DrawDebugBox(world, Transform.GetLocation(), Extent, Transform.GetRotation(), color, false, duration);
         if (showAABB)
         {
            const FBox aabb = GetAABB();
            DrawDebugBox(world, aabb.GetCenter(), aabb.GetExtent(), aabbColor, false, duration);
         }
      }

      /// Returns an axis-aligned bounding box that contains this one
      FBox GetAABB() const
      {
         FBox result{};
         constexpr int32 numVerts = 8;
         FVector verts[numVerts];
         FBox(-Extent, Extent).GetVertices(verts);
         for (int32 i = 0; i < numVerts; i++)
         {
            result += Transform.TransformPosition(verts[i]);
         }
         return result;
      }
   };

   void GetActorCharBoundingBoxes(const AActor* actor, TArray<FCharBoundingBox, TInlineAllocator<2>>& outBounds)
   {
      outBounds.Empty();
      if (actor == nullptr)
      {
         return;
      }

      auto getCapsuleComponent = [](const AActor* actor) -> UCapsuleComponent*
      {
         const ACharacter* character = Cast<ACharacter>(actor);
         return (character != nullptr) ? character->GetCapsuleComponent() : nullptr;
      };

      auto getCharBoundingBox = [getCapsuleComponent](const AActor* actor) -> ToolHelpers::FCharBoundingBox
      {
         check(actor != nullptr);
         const FQuat orientation = actor->GetActorRotation().Quaternion();
         if (const UCapsuleComponent* characterCapsule = getCapsuleComponent(actor))
         {
            return ToolHelpers::FCharBoundingBox{ characterCapsule->GetComponentLocation(), orientation, characterCapsule->Bounds.BoxExtent };
         }
         // Fallback
         constexpr bool collidingOnly = true;
         FVector loc, extent;
         actor->GetActorBounds(collidingOnly, loc, extent);
         return ToolHelpers::FCharBoundingBox{ loc, orientation, extent };
      };

      auto isUnconsciousOrLyingDown = [](const AActor* actor) -> bool
      {
         const AOSECharacterBase* oseCharacter = Cast<AOSECharacterBase>(actor);
         return (oseCharacter != nullptr) ? (oseCharacter->IsUnconscious() || oseCharacter->IsLyingDown()) : false;
      };

      const FCharBoundingBox targetStandingBounds = getCharBoundingBox(actor);
      const UCapsuleComponent* targetCharacterCapsule = getCapsuleComponent(actor);
      if (isUnconsciousOrLyingDown(actor) && ensure(targetCharacterCapsule != nullptr))
      {
         // If the character is unconscious or lying down, adjust the location to account for this.
         // This is a bit of a kludge, but we don't run animations on the server so we can't just check the anim state.
         // Note that we *don't* need to take crouching into account here because the capsule size shrinks when crouched.
         //TODO: We really need a better solution for this, because making assumptions about character animations is not great.

         const FQuat lyingDownFrontOrientation = FQuat(FRotator{ -90.0f, 0.0f, 0.0f });
         const FVector lyingDownFrontPivotPoint = FVector(targetCharacterCapsule->GetScaledCapsuleRadius(), 0, -targetCharacterCapsule->GetScaledCapsuleHalfHeight());
         outBounds.Add(targetStandingBounds.Rotate(lyingDownFrontOrientation, lyingDownFrontPivotPoint));

         const FQuat lyingDownBackOrientation = FQuat(FRotator{ 90.0f, 0.0f, 0.0f });
         const FVector lyingDownBackPivotPoint = FVector(-targetCharacterCapsule->GetScaledCapsuleRadius(), 0, -targetCharacterCapsule->GetScaledCapsuleHalfHeight());
         outBounds.Add(targetStandingBounds.Rotate(lyingDownBackOrientation, lyingDownBackPivotPoint));
      }
      else
      {
         outBounds.Add(targetStandingBounds);
      }
   }

   void DrawDebugLineTrace(UWorld* world, const FVector& traceStart, const FVector& traceEnd, const TOptional<FVector>& hitLocation, float lifeTime = -1.0f, const FColor& hitColor = FColor::Green, const FColor& missColor = FColor::Red, float thickness = 0.0f)
   {
      auto applyHSVDelta = [](const FColor& color, float hueDelta, float satDelta, float valDelta) -> FColor
      {
         FLinearColor hsv = FLinearColor(color).LinearRGBToHSV();
         hsv.R = FMath::Fmod(hsv.R + hueDelta, 360.0f);
         hsv.G = FMath::Fmod(hsv.G + satDelta, 1.0f);
         hsv.B = FMath::Fmod(hsv.B + valDelta, 1.0f);
         return hsv.HSVToLinearRGB().ToFColorSRGB();
      };

      if (hitLocation)
      {
         DrawDebugLine(world, traceStart, *hitLocation, applyHSVDelta(hitColor, 15.0f, 0, 0), false, lifeTime, 0, thickness);
         DrawDebugPoint(world, *hitLocation, FMath::GetMappedRangeValueClamped(FVector2f(0.0f, 5.0f), FVector2f(8.0f, 50.0f), thickness), hitColor, false, lifeTime);
         DrawDebugLine(world, *hitLocation, traceEnd, applyHSVDelta(hitColor, 60.0f, 0, 0), false, lifeTime, 0, thickness);
      }
      else
      {
         DrawDebugLine(world, traceStart, traceEnd, missColor, false, lifeTime, 0, thickness);
      }
   }

   bool LineCharBoundingBoxTrace(FLineBoxIntersection& outIntersection, const FCharBoundingBox& box, const FVector& traceStart, const FVector& traceEnd, UWorld* world = nullptr, bool drawDebug = false, float drawDebugDuration = -1.0f)
   {
      const bool hit = box.FindIntersectionWithLine(outIntersection, traceStart, traceEnd);
      if (drawDebug && ensure(world != nullptr))
      {
         box.DrawDebug(world, FColor::White, drawDebugDuration);
         DrawDebugLineTrace(world, traceStart, traceEnd, hit ? TOptional(outIntersection.Location) : NullOpt, drawDebugDuration, FColor::Cyan, FColor::Orange);
      }
      return hit;
   }
}

// static
UAnimMontage* UTATToolFunctionLibrary::ChooseRandomMontageForAttack(const FTATMeleeWeaponAttack& attackInfo, int32 poolIndex, int32& nextPoolIndex)
{
   if (attackInfo.SequentialAttackPools.IsValidIndex(poolIndex))
   {
      const FTATMeleeWeaponAttackAnimationPool& chosenAnimationPool = attackInfo.SequentialAttackPools[poolIndex];
      UAnimMontage* chosenMontage = nullptr;
      if (chosenAnimationPool.Animations.Num() == 0)
      {
         UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("ChooseRandomMontageForAttack: pool at index %d had zero animations"), poolIndex);
      }
      else
      {
         chosenMontage = chosenAnimationPool.Animations[FMath::RandHelper(chosenAnimationPool.Animations.Num())];
      }

      // Switch to the next pool in the set
      nextPoolIndex = (poolIndex + 1) % attackInfo.SequentialAttackPools.Num();

      return chosenMontage;
   }
   else
   {
      UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("ChooseRandomMontageForAttack: zero pools for melee attack"));
      return nullptr;
   }
}

// static
FGameplayTag UTATToolFunctionLibrary::ExtractToolUsageTypeFromContainer(const FGameplayTagContainer& tagContainer)
{
   // Find the first gameplay tag with a parent matching TAG_Tool_Usage
   for (const FGameplayTag tag : tagContainer)
   {
      if (tag.MatchesTag(TAG_Tool_Usage))
      {
         return tag;
      }
   }

   return FGameplayTag();
}

// static
void UTATToolFunctionLibrary::TryAdjustWorldActorBoxFillPlacement(const AActor* actor, const FTATWorldActorBoxFillExtentConstraints& extentConstraints, FTATWorldActorBoxFillAdjustedTransform& outNewExtents)
{
   const int32 kExpectedNumberAxes = 4;

   static const FVector kAxesToCheck[] =
   {
      // Cardinals (not including negations)
      FVector( 1.0f,  0.0f, 0.0f),
      FVector( 0.0f,  1.0f, 0.0f),

      // Diagonals (not including negations)
      FVector( 1.0f,  1.0f, 0.0f),
      FVector( 1.0f, -1.0f, 0.0f),
   };

   static_assert(UE_ARRAY_COUNT(kAxesToCheck) == kExpectedNumberAxes, "Check kAxesToCheck");

   UWorld* world = actor->GetWorld();
   check(world);

   const FVector actorLocation = actor->GetActorLocation();
   const FRotator actorRotation = actor->GetActorRotation();

   FCollisionQueryParams queryParams = FCollisionQueryParams::DefaultQueryParam;
   queryParams.AddIgnoredActor(actor);

   // Compute how much headroom we have above us
   {
      const FVector traceEnd = actorLocation + FVector::UpVector * extentConstraints.MaxHeight;

      FHitResult hit;
      world->LineTraceSingleByProfile(hit, actorLocation, traceEnd, extentConstraints.TraceProfile.Name, queryParams);

      if (hit.bBlockingHit)
      {
         outNewExtents.Height = hit.Distance;
      }
      else
      {
         outNewExtents.Height = extentConstraints.MaxHeight;
      }

#if ENABLE_DRAW_DEBUG
      if (CVarTATWorldActorAdjustmentDebugDraw.GetValueOnGameThread() != 0)
      {
         DrawDebugLine(world, actorLocation, hit.bBlockingHit ? hit.Location : traceEnd, hit.bBlockingHit ? FColor::Red : FColor::Green, false, 5.0f, 0, 2.0f);
      }
#endif
   }

   // NB: We do this at a Z level based on our max height: we don't want to do it too low, or we're likely to hit redundant objects, e.g. stairs, bumps
   // We also don't want to do it too high, for the same reason but on the ceiling
   // For now, the default is 90% but can be configured via CVar
   const FVector traceLocation = actorLocation + FVector::UpVector * outNewExtents.Height * CVarTATWorldActorAdjustmentTraceHeightRatio.GetValueOnGameThread();

   struct FTraceInfo
   {
      float positiveDistance = 0.0f;
      float negativeDistance = 0.0f;
   };

   // Compute distances in both directions for each axis
   FTraceInfo axisTraceInfo[kExpectedNumberAxes] = {};
   for (int32 i = 0; i < kExpectedNumberAxes; i++)
   {
      const FVector positiveAxis = kAxesToCheck[i];
      const FVector negativeAxis = -positiveAxis;

      const FVector positiveTraceEnd = traceLocation + positiveAxis * extentConstraints.MaxHalfExtents2D;
      const FVector negativeTraceEnd = traceLocation + negativeAxis * extentConstraints.MaxHalfExtents2D;

      FHitResult positiveHit;
      world->LineTraceSingleByProfile(positiveHit, traceLocation, positiveTraceEnd, extentConstraints.TraceProfile.Name, queryParams);

      FHitResult negativeHit;
      world->LineTraceSingleByProfile(negativeHit, traceLocation, negativeTraceEnd, extentConstraints.TraceProfile.Name, queryParams);

      axisTraceInfo[i].positiveDistance = positiveHit.bBlockingHit ? positiveHit.Distance : extentConstraints.MaxHalfExtents2D;
      axisTraceInfo[i].negativeDistance = negativeHit.bBlockingHit ? negativeHit.Distance : extentConstraints.MaxHalfExtents2D;

#if ENABLE_DRAW_DEBUG
      if (CVarTATWorldActorAdjustmentDebugDraw.GetValueOnGameThread() != 0)
      {
         DrawDebugLine(world, traceLocation, positiveHit.bBlockingHit ? positiveHit.Location : positiveTraceEnd, positiveHit.bBlockingHit ? FColor::Red : FColor::Green, false, 5.0f, 0, 2.0f);
         DrawDebugLine(world, traceLocation, negativeHit.bBlockingHit ? negativeHit.Location : negativeTraceEnd, negativeHit.bBlockingHit ? FColor::Red : FColor::Green, false, 5.0f, 0, 2.0f);
      }
#endif
   }

   // Find shortest axis: since this constraints us the most, we will align our bounding box to it and the vector perpendicular to it
   // That way, if we satisfy that constraint we can be reasonably confident that we aren't clipping into the other axes
   int32 shortestAxisIndex = 0;
   float shortestAxisLength = FLT_MAX;
   for (int32 i = 0; i < kExpectedNumberAxes; i++)
   {
      const float axisLength = axisTraceInfo[i].positiveDistance + axisTraceInfo[i].negativeDistance;

      if (axisLength < shortestAxisLength)
      {
         shortestAxisIndex = i;
         shortestAxisLength = axisLength;
      }
   }

   const FTraceInfo shortestAxisInfo = axisTraceInfo[shortestAxisIndex];

   // Orient to the shortest axis
   const FVector newForwardVector = kAxesToCheck[shortestAxisIndex];
   const FRotator alignedRotation = FRotationMatrix::MakeFromX(newForwardVector).Rotator();
   outNewExtents.WorldRotation = alignedRotation;

   // Determine position and extents based on those axes
   FVector newLocation = actorLocation;
   {
      // Find the center of the bounds for this axis, and half its total size
      // This will be our new position and extents in that axis
      const float forwardAxisCenter = (shortestAxisInfo.positiveDistance - shortestAxisInfo.negativeDistance) * 0.5f;
      const float forwardAxisHalfTotal = (shortestAxisInfo.positiveDistance + shortestAxisInfo.negativeDistance) * 0.5f;

      newLocation += kAxesToCheck[shortestAxisIndex] * forwardAxisCenter;
      outNewExtents.HalfExtents2D.X = forwardAxisHalfTotal;

      // axes 0 and 1 are the X and Y axes, so toggling the low bit will switch b/w them
      // axes 2 and 3 are the diagonals, so same for them
      const int32 perpendicularAxisIndex = shortestAxisIndex ^ 0x01;
      check(FVector::DotProduct(kAxesToCheck[shortestAxisIndex], kAxesToCheck[perpendicularAxisIndex]) < UE_SMALL_NUMBER);
      const FTraceInfo perpendicularAxisInfo = axisTraceInfo[perpendicularAxisIndex];

      // Do the same thing for our right-facing axis
      const float rightAxisCenter = (perpendicularAxisInfo.positiveDistance - perpendicularAxisInfo.negativeDistance) * 0.5f;
      const float rightAxisHalfTotal = (perpendicularAxisInfo.positiveDistance + perpendicularAxisInfo.negativeDistance) * 0.5f;

      newLocation += kAxesToCheck[perpendicularAxisIndex] * rightAxisCenter;
      outNewExtents.HalfExtents2D.Y = rightAxisHalfTotal;
   }

   outNewExtents.WorldPosition = newLocation;

#if ENABLE_DRAW_DEBUG
   if (CVarTATWorldActorAdjustmentDebugDraw.GetValueOnGameThread() != 0)
   {
      const FVector requestedBoxCenter = actorLocation + FVector::UpVector * extentConstraints.MaxHeight * 0.5f;
      const FVector requestedBoxExtents = FVector(extentConstraints.MaxHalfExtents2D, extentConstraints.MaxHalfExtents2D, extentConstraints.MaxHeight * 0.5f);
      DrawDebugBox(world, requestedBoxCenter, requestedBoxExtents, FQuat(actorRotation), FColor::Yellow, false, 5.0f, 0, 2.0f);

      const FVector newBoxCenter = newLocation + FVector::UpVector * outNewExtents.Height * 0.5f;
      const FVector newBoxExtents = FVector(outNewExtents.HalfExtents2D, outNewExtents.Height * 0.5f);
      DrawDebugBox(world, newBoxCenter, newBoxExtents, FQuat(alignedRotation), FColor::Green, false, 5.0f, 0, 2.0f);
   }
#endif
}

// static
int32 UTATToolFunctionLibrary::GetTargetCharacterBounds(AActor* actor, TArray<FBox, TInlineAllocator<2>>& outBounds)
{
   outBounds.Empty();
   if (actor != nullptr)
   {
      TArray<ToolHelpers::FCharBoundingBox, TInlineAllocator<2>> targetBounds;
      ToolHelpers::GetActorCharBoundingBoxes(actor, targetBounds);
      check(targetBounds.Num() == 1 || targetBounds.Num() == 2);
      outBounds.SetNum(targetBounds.Num());
      for (int32 i = 0; i < targetBounds.Num(); i++)
      {
         outBounds[i] = targetBounds[i].GetAABB();
      }
   }
   return outBounds.Num();
}

// static
USceneComponent* UTATToolFunctionLibrary::FindFirstInteractableComponentInActor(AActor* actor)
{
   if (IsValid(actor))
   {
      for (UActorComponent* component : actor->GetComponents())
      {
         USceneComponent* sceneComp = Cast<USceneComponent>(component);
         if (sceneComp != nullptr && UOSEInteractionHelpers::IsComponentTargetableForInteraction(sceneComp))
         {
            return sceneComp;
         }
      }
   }
   return nullptr;
}

// static
bool UTATToolFunctionLibrary::PerformLineOfSightTrace(FHitResult& outHitResult, AActor* sourceActor, AActor* targetActor, const FTATLineOfSightTraceParams& params, bool debug, float debugDrawDuration)
{
   if (sourceActor == nullptr || targetActor == nullptr)
   {
      outHitResult = FHitResult{};
      return false;
   }

   UWorld* world = sourceActor->GetWorld();
   check(world != nullptr);

   FVector sourceLocation;
   FRotator sourceRotation;
   if (!UOSEAbilityFunctionLibrary::OffsetCameraAimToAvatarAim(sourceActor, FGameplayAbilityTargetingLocationInfo(), sourceLocation, sourceRotation))
   {
      // fallback
      sourceActor->GetActorEyesViewPoint(sourceLocation, sourceRotation);
   }

   // If targetActor is a non-character interactable, this is the specific component on that actor we're aiming for
   USceneComponent* targetComponent = nullptr;

   if (!targetActor->IsA<ACharacter>() && targetActor->Implements<UInteractableInterface>())
   {
      if (!params.AllowNonCharacterInteractableTarget)
      {
         // Target is interactable, but we specifically want to ignore those
         return false;
      }

      targetComponent = FindFirstInteractableComponentInActor(targetActor);
   }

   const FVector targetLocation = targetComponent
      ? targetComponent->GetComponentLocation()
      : targetActor->GetActorLocation();

   // Init the hit result with a reasonable default
   outHitResult.Init(sourceLocation, targetLocation);

   if (params.RequireSourceActorConscious)
   {
      AOSECharacterBase* sourceCharacter = Cast<AOSECharacterBase>(sourceActor);
      if (sourceCharacter != nullptr && sourceCharacter->IsUnconscious())
      {
         return false;
      }
   }

   if (params.RequireSourceTag.IsValid())
   {
      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(sourceActor);
      if (asc != nullptr && !asc->HasMatchingGameplayTag(params.RequireSourceTag))
      {
         return false;
      }
   }

   if (params.RequireTargetTag.IsValid())
   {
      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(targetActor);
      if (asc != nullptr && !asc->HasMatchingGameplayTag(params.RequireTargetTag))
      {
         return false;
      }
   }

   // Get all possible bounding boxes for the target.
   // This is normally one, but if the target is knocked out we could get two possibilities.
   TArray<ToolHelpers::FCharBoundingBox, TInlineAllocator<2>> targetBounds;
   if (targetComponent != nullptr)
   {
      // Just get the component bounds if we're looking for a specific component
      targetBounds.Add(ToolHelpers::FCharBoundingBox{ targetComponent->Bounds.Origin, FQuat::Identity, targetComponent->Bounds.BoxExtent });
   }
   else
   {
      ToolHelpers::GetActorCharBoundingBoxes(targetActor, targetBounds);
   }
   check(targetBounds.Num() == 1 || targetBounds.Num() == 2);

   // If the source actor implements the AI sight interface, use that to limit the max distance they can see
   float maxDistance = params.MaxDistance;
   if (maxDistance > 0 && params.UseAISightToLimitMaxDistance && sourceActor->Implements<UOSEAISightInterface>())
   {
      IOSEAISightInterface* aiSightInterface = CastChecked<IOSEAISightInterface>(sourceActor);
      if (!aiSightInterface->IsAllowedToSeeActor(targetActor))
      {
         return false;
      }

      // At the moment (8/8/2024), implementations of ModifySightRangeForSpecificActor simply do an in-place multiplication of the distance value.
      // Clamp the result to keep the value sane.
      aiSightInterface->ModifySightRangeForSpecificActor(targetActor, maxDistance);
      maxDistance = FMath::Clamp(maxDistance, 1.0f, params.MaxDistance);
   }

   constexpr bool debugDrawPersistent = false;
   constexpr int32 numDebugConeSides = 16;

   auto isTargetWithinRange = [world, debug, debugDrawDuration](const FVector& srcLocation, const FVector& tgtLocation, float maxDist)
   {
      const bool isInRange = FVector::DistSquared(srcLocation, tgtLocation) <= FMath::Square(maxDist);

      if (debug)
      {
         const FVector targetDir = (tgtLocation - srcLocation).GetSafeNormal();
         const FVector searchEndpoint = srcLocation + (targetDir * maxDist);
         constexpr float coneAngleRad = FMath::DegreesToRadians(35.0f);
         constexpr float coneLength = 20.0f;
         if (isInRange)
         {
            DrawDebugLine(world, srcLocation, tgtLocation, FColor::Green, debugDrawPersistent, debugDrawDuration);
            DrawDebugCone(world, (tgtLocation + srcLocation) * 0.5f, -targetDir, coneLength, coneAngleRad, coneAngleRad, numDebugConeSides, FColor::Emerald, debugDrawPersistent, debugDrawDuration);
         }
         else
         {
            DrawDebugLine(world, srcLocation, searchEndpoint, FColor::Cyan, debugDrawPersistent, debugDrawDuration);
            DrawDebugCone(world, searchEndpoint, -targetDir, coneLength, coneAngleRad, coneAngleRad, numDebugConeSides, FColor::Red, debugDrawPersistent, debugDrawDuration);
         }
      }

      return isInRange;
   };

   // distance check
   if (maxDistance > 0
      && !isTargetWithinRange(sourceLocation, targetLocation, maxDistance)
      && !isTargetWithinRange(sourceLocation, targetBounds[0].GetLocation(), maxDistance)
      && (!targetBounds.IsValidIndex(1) || !isTargetWithinRange(sourceLocation, targetBounds[1].GetLocation(), maxDistance)))
   {
      return false;
   }

   auto isTargetInView = [world, &params, debug, debugDrawDuration](const FVector& srcLocation, const FRotator& srcRotation, const ToolHelpers::FCharBoundingBox& tgtBounds, bool tgtIsNPC, float maxDist) -> bool
   {
      // Not configured to check for view angle
      if (params.MaxAngleDeg <= 0)
      {
         return true;
      }

      // Check if the source is looking directly at the target
      ToolHelpers::FLineBoxIntersection boundsIntersection{};
      if (ToolHelpers::LineCharBoundingBoxTrace(boundsIntersection, tgtBounds, srcLocation, srcLocation + (srcRotation.RotateVector(FVector::ForwardVector) * maxDist), world, debug, debugDrawDuration))
      {
         return true;
      }

      // check if the target is in the source's field of view by doing a frustum check
      // When at close range, just do a dot product check (frustums get finicky at close range)
      if (params.UseFrustumCheckForViewAngle
         && (params.MinDistanceForFrustumCheck <= 0 || FVector::DistSquared(srcLocation, tgtBounds.GetLocation()) >= FMath::Square(params.MinDistanceForFrustumCheck)))
      {
         constexpr float nearClip = 1.0f;
         const float farClip = maxDist * 1.1f;

         // The aspect ratio, used to make the frustum wider or taller. 1.0 is square
         constexpr float frustumAspectRatio = 2.2f;

         // For some reason, NPCs need a few degrees added to their frustum pitch, but player characters do not
         const float frustumPitch = tgtIsNPC ? -15.0f : 0.0f;

         const FBox tgtAABB = tgtBounds.GetAABB();
         const bool isTargetInFrustum = OSEAIPerceptionHelpers::CheckIsTargetInFrustum(world, srcLocation, srcRotation, tgtAABB.GetCenter(), tgtAABB.GetExtent(),
            params.MaxAngleDeg * 0.5f, nearClip, farClip, frustumPitch, frustumAspectRatio);

         if (debug)
         {
            OSEAIPerceptionHelpers::DrawDebugTargetFrustum(world, srcLocation, srcRotation, params.MaxAngleDeg * 0.5f, nearClip, farClip, frustumPitch,
               frustumAspectRatio, isTargetInFrustum ? FColor::Green : FColor::Orange, debugDrawDuration);
         }

         return isTargetInFrustum;
      }

      // Just check the view angle from the source to the target
      const FVector targetDir = (tgtBounds.GetLocation() - srcLocation).GetSafeNormal();
      const FVector sourceFwd = srcRotation.Vector().GetSafeNormal();
      const float angleDeg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(sourceFwd, targetDir)));
      const bool isTargetInViewAngle = angleDeg <= params.MaxAngleDeg;
      if (debug)
      {
         const float maxAngleRad = FMath::DegreesToRadians(params.MaxAngleDeg);
         DrawDebugCone(world, srcLocation, sourceFwd, maxDist, maxAngleRad, maxAngleRad, numDebugConeSides,
            isTargetInViewAngle ? FColor::Green : FColor::Orange, debugDrawPersistent, debugDrawDuration);
      }
      return isTargetInViewAngle;
   };

   // Check if the target is within the specified viewing angle (if configured)
   const bool targetIsNPC = targetActor->IsA<ATATCharacterAIBase>();
   if (!isTargetInView(sourceLocation, sourceRotation, targetBounds[0], targetIsNPC, maxDistance)
      && (!targetBounds.IsValidIndex(1) || !isTargetInView(sourceLocation, sourceRotation, targetBounds[1], targetIsNPC, maxDistance)))
   {
      return false;
   }

   auto haveLineOfSightToTarget = [world, &params, debug, debugDrawDuration](FHitResult& hitResult, const FVector& traceStart, const ToolHelpers::FCharBoundingBox& tgtBounds, TConstArrayView<AActor*> ignoreActors) -> bool
   {
      // Not configured to do a trace, so just assume there was nothing there
      if (params.TraceProfile.Name == NAME_None)
      {
         return true;
      }

      // Check if this trace would intersect with the target's bounding box before we do the actual trace
      ToolHelpers::FLineBoxIntersection boundsIntersection{};
      if (!ToolHelpers::LineCharBoundingBoxTrace(boundsIntersection, tgtBounds, traceStart, tgtBounds.GetLocation(), world, debug, debugDrawDuration))
      {
         return false;
      }

      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(UTATToolFunctionLibrary_PerformLineOfSightTrace), params.TraceComplex);
      for (AActor* actor : ignoreActors)
      {
         queryParams.AddIgnoredActor(actor);
      }

      // Even if we hit something, we only have line of sight if we hit the target.
      // Only trace up to the target's bounding box - we don't care about hitting the target, we just want to see if there's anything in between the source and the target.
      const FVector reverseTraceDir = (traceStart - boundsIntersection.Location).GetSafeNormal();
      const FVector traceEnd = boundsIntersection.Location + (reverseTraceDir * 0.1f); // backtrack very slightly towards the source location
      const bool hitSomething = world->LineTraceSingleByProfile(hitResult, traceStart, traceEnd, params.TraceProfile.Name, queryParams);
      if (debug)
      {
         ToolHelpers::DrawDebugLineTrace(world, traceStart, traceEnd, hitSomething ? TOptional<FVector>(hitResult.Location) : NullOpt, debugDrawDuration, FColor::Red, FColor::Green);
      }
      // We have line of sight if the line trace hit the target
      return !hitSomething;
   };

   // The only thing left to check is if there's anything in the way
   const TArray<AActor*, TInlineAllocator<2>> ignoreActors = { targetActor, sourceActor };
   if (!haveLineOfSightToTarget(outHitResult, sourceLocation, targetBounds[0], ignoreActors)
      && (!targetBounds.IsValidIndex(1) || !haveLineOfSightToTarget(outHitResult, sourceLocation, targetBounds[1], ignoreActors)))
   {
      return false;
   }

   // If we didn't have a trace profile and we got this far, assume we have line of sight.
   if (!outHitResult.bBlockingHit)
   {
      // If we never ended up initializing the hit result, make a fake one.
      outHitResult.Init(sourceLocation, targetLocation);
      outHitResult.bBlockingHit = true;
      outHitResult.HitObjectHandle = targetActor;
   }
   return true;
}

UToolComponent* UTATToolFunctionLibrary::GetEquippedToolComponentFromActor(AActor* actor)
{
   if (TScriptInterface<IToolSetInterface> toolsetInterface = IToolSetInterface::GetToolSetFromActor(actor))
   {
      return toolsetInterface.GetInterface()->GetCurrentTool();
   }
   return nullptr;
}

UTATMeleeWeaponToolComponent* UTATToolFunctionLibrary::GetEquippedMeleeWeaponToolFromActor(AActor* actor)
{
   if (!actor)
   {
      UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("Invalid toolOwner passed to GetEquippedMeleeWeaponTool()!"));
      return nullptr;
   }
   
   TScriptInterface<IToolSetInterface> toolsetInterface = IToolSetInterface::GetToolSetFromActor(actor);
   if (!toolsetInterface)
   {
      UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("GetEquippedMeleeWeaponTool() | Failed to retrieve IToolSetInterface from actor %s!"), *actor->GetName());
      return nullptr;
   }
   return Cast<UTATMeleeWeaponToolComponent>(toolsetInterface.GetInterface()->GetCurrentTool());
}

UToolSetComponent* UTATToolFunctionLibrary::GetToolSetComponentFromActor(AActor* actor)
{
   if (!actor)
   {
      UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("GetToolSetComponentFromActor() called with invalid actor!"));
      return nullptr;
   }
   TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actor);
   if (!toolSetInterface)
   {
      UE_LOG(LogTATToolFunctionLibrary, Error, TEXT("GetToolSetComponentFromActor() called with actor %s that doesn't implement IToolSetInterface!"), *actor->GetName());
      return nullptr;
   }

   return Cast<UToolSetComponent>(toolSetInterface.GetObject());
}
