// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/AimAssist/OSEAimAssistComponent.h"

// ose
#include "Input/OSEInputFunctionLibrary.h"
#include "Player/OSEPlayerController.h"

// ue4
#include "GameFramework/HUD.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAimAssistComponent)

DEFINE_LOG_CATEGORY(LogAimAssist);

namespace AimAssistCVars
{
   static int AimAssistEnabled = 1;
   FAutoConsoleVariableRef CVarAimAssistEnabled(
      TEXT("OSE.AimAssist.Enabled"),
      AimAssistEnabled,
      TEXT("Is aim assist enabled?"),
      ECVF_Default);

   static int RequiresInput = 1;
   FAutoConsoleVariableRef CVarRequiresInput(
      TEXT("OSE.AimAssist.RequiresInput"),
      RequiresInput,
      TEXT("Does aim assist require some input to run?"),
      ECVF_Default);

   static int AllowMouseKeyboard = 0;
   FAutoConsoleVariableRef CVarAllowMouseKeyboard(
      TEXT("OSE.AimAssist.AllowMouseKeyboard"),
      AllowMouseKeyboard,
      TEXT("Is aim assist turned on for mouse/keyboard?"),
      ECVF_Default);

   static int DebugDraw = 0;
   FAutoConsoleVariableRef CVarDebugDraw(
      TEXT("OSE.AimAssist.DebugDraw"),
      DebugDraw,
      TEXT("Is aim assist debug drawing?"),
      ECVF_Default);
}

namespace AimAssistUtl
{
   FBox2D GetScreenSpaceBox(const AOSEPlayerController& pc, const FOrientedBox& oBox)
   {
      FVector outVertices[8];
      oBox.CalcVertices(outVertices);

      // Build 2D bounding box of actor in screen space
      FBox2D actorBox2D(EForceInit::ForceInitToZero);
      for (uint8 vertIdx = 0; vertIdx < 8; vertIdx++)
      {
         FVector2D screenLocation;

         // Project vert into screen space.
         pc.ProjectWorldLocationToScreen(outVertices[vertIdx], screenLocation);

         // Add to 2D bounding box
         actorBox2D += screenLocation;
      }

      return actorBox2D;
   }

   FBox2D GetScreenSpaceBoxWithOrientation(const AOSEPlayerController& pc, AActor& actor, float minSize)
   {
      FOrientedBox oBox;

      static const bool kNonColliding = false;
      static const bool kIncludeFromChildActors = false;
      const FBox actorBounds = actor.GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);

      // center
      FVector extents;
      actorBounds.GetCenterAndExtents(oBox.Center, extents);

      // extents
      oBox.ExtentX = FMath::Max(minSize, extents.X);
      oBox.ExtentY = FMath::Max(minSize, extents.Y);
      oBox.ExtentZ = FMath::Max(minSize, extents.Z);

      // rotation
      // Rotating an AABB with the local rotation is not a recipe for success
	  // TODO: eval if local-space bounds are worth the perf cost to calculate
      /*FTransform xfm = actor.GetActorTransform();
      oBox.AxisX = xfm.GetScaledAxis(EAxis::X);
      oBox.AxisY = xfm.GetScaledAxis(EAxis::Y);
      oBox.AxisZ = xfm.GetScaledAxis(EAxis::Z);*/

      return GetScreenSpaceBox(pc, oBox);
   }

   FBox2D GetScreenSpaceBoxWithOrientation(const AOSEPlayerController& pc, USceneComponent& component, float minSize)
   {
      FOrientedBox oBox;

      // center
      oBox.Center = component.Bounds.Origin;

      // extents
      FVector extents = component.Bounds.BoxExtent;
      oBox.ExtentX = FMath::Max(minSize, extents.X);
      oBox.ExtentY = FMath::Max(minSize, extents.Y);
      oBox.ExtentZ = FMath::Max(minSize, extents.Z);

      // rotation
      // Rotating an AABB with the local rotation is not a recipe for success
	  // TODO: eval if local-space bounds are worth the perf cost to calculate
      /*FTransform xfm = component.GetComponentTransform();
      oBox.AxisX = xfm.GetScaledAxis(EAxis::X);
      oBox.AxisY = xfm.GetScaledAxis(EAxis::Y);
      oBox.AxisZ = xfm.GetScaledAxis(EAxis::Z);*/

      return GetScreenSpaceBox(pc, oBox);
   }

   FBox2D GetScreenSpaceBoxWithOrientation(const AOSEPlayerController& pc, FVector center, FVector extents, float minSize)
   {
      FOrientedBox oBox;

      // center
      oBox.Center = center;

      // extents
      oBox.ExtentX = FMath::Max(minSize, extents.X);
      oBox.ExtentY = FMath::Max(minSize, extents.Y);
      oBox.ExtentZ = FMath::Max(minSize, extents.Z);

      return GetScreenSpaceBox(pc, oBox);
   }

   FBox2D ScaleBoxBy(const FBox2D& box, float multiplier)
   {
      // outer box is just a multiplier of the inner box
      FVector2D size = box.GetSize();
      FVector2D newSize = size * multiplier;
      FVector2D deltaSize = newSize - size;
      
      FBox2D newBox;
      newBox.Min = (box.Min - deltaSize);
      newBox.Max = (box.Max + deltaSize);
      newBox.bIsValid = true;
      return newBox;
   }

   FVector2D FindClosestPointOnLine(const FVector2D& lineStart, const FVector2D& lineEnd, const FVector2D& testPoint)
   {
      const FVector2D lineVector = lineEnd - lineStart;

      const float a = -FVector2D::DotProduct(lineStart - testPoint, lineVector);
      const float b = lineVector.SizeSquared();
      const float t = FMath::Clamp<float>(a / b, 0.0f, 1.0f);

      // Generate closest point
      return lineStart + (t * lineVector);
   }

   TArray<TPair<FVector2D, FVector2D>> GetLinesFromBox(const FBox2D& box)
   {
      FVector2D center;
      FVector2D extents;
      box.GetCenterAndExtents(center, extents);

      TArray<TPair<FVector2D, FVector2D>> lines =
      {
         TPair<FVector2D, FVector2D>(FVector2D(center.X - extents.X, center.Y + extents.Y), FVector2D(center.X - extents.X, center.Y - extents.Y)),
         TPair<FVector2D, FVector2D>(FVector2D(center.X - extents.X, center.Y + extents.Y), FVector2D(center.X + extents.X, center.Y + extents.Y)),
         TPair<FVector2D, FVector2D>(FVector2D(center.X + extents.X, center.Y - extents.Y), FVector2D(center.X + extents.X, center.Y + extents.Y)),
         TPair<FVector2D, FVector2D>(FVector2D(center.X + extents.X, center.Y - extents.Y), FVector2D(center.X - extents.X, center.Y - extents.Y))
      };
      return lines;
   }

   FVector2D GetNearestPointInPerimeter(const FBox2D& box, const FVector2D& point)
   {
      check(box.IsInside(point));
      
      float bestDistance = float(INDEX_NONE);
      FVector2D bestPoint = FVector2D::ZeroVector;

      TArray<TPair<FVector2D, FVector2D>> lines = GetLinesFromBox(box);
      for (const TPair<FVector2D, FVector2D>& line : lines)
      {
         const FVector2D& start = line.Key;
         const FVector2D& end = line.Value;
         const FVector2D closestPointOnLine = FindClosestPointOnLine(start, end, point);
         const float distance = FVector2D::Distance(point, closestPointOnLine);
         if (bestDistance == float(INDEX_NONE) || distance < bestDistance)
         {
            bestDistance = distance;
            bestPoint = closestPointOnLine;
         }
      }
      return bestPoint;
   }

#if !UE_BUILD_SHIPPING
   void DebugDrawBox(AHUD& hud, const FBox2D& box, const FLinearColor& color)
   {
      TArray<TPair<FVector2D, FVector2D>> lines = GetLinesFromBox(box);
      for(const TPair<FVector2D, FVector2D>& line : lines)
      {
         const FVector2D& start = line.Key;
         const FVector2D& end = line.Value;
         hud.DrawLine(start.X, start.Y, end.X, end.Y, color, 1.0f);
      }
   }
   
   void DebugDrawWeightText(AHUD& hud, const FBox2D& box, const FLinearColor& color, float weight)
   {
      TArray<TPair<FVector2D, FVector2D>> lines = GetLinesFromBox(box);
      static int kLineNum = 2;
      if (lines.Num() > 0)
      {
         const TPair<FVector2D, FVector2D>& line = lines[kLineNum];
         const FVector2D& start = line.Key;
         const FVector2D& end = line.Value;
         hud.DrawText(FString::Printf(TEXT("%.02f"), weight), color, start.X, start.Y, nullptr, 2.0f, false);
      }
      FVector2D center;
      FVector2D extents;
      box.GetCenterAndExtents(center, extents);
      
   }

   void DebugDrawPoint(AHUD& hud, const FVector2D& point, const FLinearColor& color, float size)
   {
      if (!point.IsZero())
         hud.DrawRect(color, point.X - (0.5f * size), point.Y - (0.5f * size), size, size);
   }
#endif
}

//---------------------------------------------------------------------------------------
// FAimAssistTarget
//---------------------------------------------------------------------------------------

FString FAimAssistTarget::GetDebugName() const
{
   if (BoundsComponent.IsValid())
   {
      return FString::Printf(TEXT("{%s : %s}"), *AActor::GetDebugName(BoundsComponent->GetOwner()), *BoundsComponent->GetName());
   }
   else if (BoundsActor.IsValid())
   {
      return FString::Printf(TEXT("{%s}"), *AActor::GetDebugName(BoundsActor.Get()));
   }
   else
   {
      return TEXT("{InvalidAimAssistTarget}");
   }
}

void FAimAssistTarget::_CacheData(const AOSEPlayerController& owningPC, float minBoxSize)
{
   FBox2D boundingBox(EForceInit::ForceInitToZero);

   if (UseRawCenterAndExtents)
   {
      _CacheCenterLoc(owningPC, RawCenter);

      boundingBox = AimAssistUtl::GetScreenSpaceBoxWithOrientation(owningPC, RawCenter, RawExtents, minBoxSize);
   }
   else if (BoundsComponent.IsValid())
   {
      // TODO: should we allow multiple bounds components and add them all together?
      USceneComponent* comp = BoundsComponent.Get();

      // start w/ the target loc focusing on the bounds component
      _CacheCenterLoc(owningPC , *comp);

      // unmodified box
      boundingBox = AimAssistUtl::GetScreenSpaceBoxWithOrientation(owningPC, *comp, minBoxSize);
   }
   else if (BoundsActor.IsValid())
   {
      AActor* actor = BoundsActor.Get();

      // start w/ the target loc focusing on the bounds actor
      _CacheCenterLoc(owningPC, *actor);

      // unmodified box
      boundingBox = AimAssistUtl::GetScreenSpaceBoxWithOrientation(owningPC, *actor, minBoxSize);
   }

   if (!boundingBox.GetExtent().IsZero())
   {
      // inner box
      _screenSpaceBoxInner = AimAssistUtl::ScaleBoxBy(boundingBox, InnerBoxSizeMultiplier);

      // outer box
      _screenSpaceBoxOuter = AimAssistUtl::ScaleBoxBy(boundingBox, OuterBoxSizeMultiplier);
   }

   if (CenterComponent.IsValid())
   {
      USceneComponent* comp = CenterComponent.Get();
      _CacheCenterLocOffset(owningPC, *comp, CenterOffset);
   }
}

void FAimAssistTarget::_CacheCenterLoc(const AOSEPlayerController& owningPC, USceneComponent& comp)
{
   _centerLoc = comp.GetComponentLocation();
   _CacheIsOnScreen(owningPC);
}

void FAimAssistTarget::_CacheCenterLoc(const AOSEPlayerController& owningPC, AActor& actor)
{
   _centerLoc = actor.GetActorLocation();
   _CacheIsOnScreen(owningPC);
}

void FAimAssistTarget::_CacheCenterLoc(const AOSEPlayerController& owningPC, FVector center)
{
   _centerLoc = center;
   _CacheIsOnScreen(owningPC);
}

void FAimAssistTarget::_CacheCenterLocOffset(const AOSEPlayerController& owningPC, USceneComponent& comp, const FVector& offset)
{
   _centerLoc = comp.GetComponentTransform().TransformPosition(offset);
   _CacheIsOnScreen(owningPC);
}

void FAimAssistTarget::_CacheIsOnScreen(const AOSEPlayerController& owningPC)
{
   if (owningPC.ProjectWorldLocationToScreen(_centerLoc, _screenSpaceCenterLoc))
   {
      int32 sizeX, sizeY;
      owningPC.GetViewportSize(sizeX, sizeY);

      const FVector2D viewportSize = FVector2D(sizeX, sizeY);
      const FVector2D viewportCenter = viewportSize / 2.0f;
      _isCenterProjectedOnScreen = _screenSpaceCenterLoc.X >= 0 && _screenSpaceCenterLoc.X <= viewportSize.X &&
                                   _screenSpaceCenterLoc.Y >= 0 && _screenSpaceCenterLoc.Y <= viewportSize.Y;
   }
}

//---------------------------------------------------------------------------------------
// UOSEAimAssistComponent
//---------------------------------------------------------------------------------------

UOSEAimAssistComponent::UOSEAimAssistComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UOSEAimAssistComponent::BeginPlay()
{
   Super::BeginPlay();
}

FRotator UOSEAimAssistComponent::ProcessAimAssist(const AOSEPlayerController& owningPC, const FRotator& inputRot, float deltaTime)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UOSEAimAssistComponent_ProcessAimAssist);

   APawn* owningPawn = owningPC.GetPawn();

   // check if we have valid (required) input or not
   const bool hasValidInput = !(AimAssistCVars::RequiresInput && inputRot.IsZero());

   const bool enabled =
      owningPawn && // wait for the controller to control a pawn
      (AimAssistCVars::AimAssistEnabled) &&  // cvar to disable the whole system
      (hasValidInput) && // only adjust aiming if we actually have some input to assist with, don't pull the cursor if there's no movement
      (owningPC.GetCurrentInputHardwareType() == EOSEInputHardwareType::KeyboardMouse ? AimAssistCVars::AllowMouseKeyboard : true); // only used for gamepads unless enabled for m/k explicitly

   FRotator newInputDelta = FRotator::ZeroRotator;

   if (enabled)
   {
      // pawn eyes
      FVector pawnEyesLoc;
      FRotator pawnEyesRot;
      owningPawn->GetActorEyesViewPoint(pawnEyesLoc, pawnEyesRot);

      // make sure the input is considered in the rotation
      pawnEyesRot = (pawnEyesRot + inputRot).GetNormalized();

      const FTransform pawnEyesXfm(pawnEyesRot, pawnEyesLoc);

      // start by filtering out aim assist targets that are invalid
      _FilterAimAssistTargets(*owningPawn, pawnEyesXfm);

      // given the new list of valid aim assist targets, if we have a valid target,
      // calculate new input rotation given our current input rotation + aim assist magnetism
      if (FAimAssistTarget* aimAssistTarget = _FindAimAssistTarget(owningPC, pawnEyesXfm))
      {
         // center location
         FVector centerLoc = aimAssistTarget->GetCenterLoc();

         // look-at rot from eyes to our target (minimize roll by using same up-axis)
         const FVector axisX = centerLoc - pawnEyesLoc;
         const FVector axisZ = pawnEyesRot.RotateVector(FVector::UpVector);
         FRotator lookAtRot = FRotationMatrix::MakeFromXZ(axisX, axisZ).Rotator();
         lookAtRot.Roll = pawnEyesRot.Roll; // make sure roll doesn't change
         
         // delta from where we are to where we'd like to be
         const FRotator deltaRot = (lookAtRot - pawnEyesRot).GetNormalized();

         // make sure our weight does not overshoot (or undershoot)
         const float strength = FMath::Clamp(deltaTime * MagnetStrength * aimAssistTarget->GetAimAssistStrength(), 0.f, 1.f);
         newInputDelta += deltaRot * strength;
      }
   }

   // clear our our aim assist targets
   _aimAssistTargets.Reset();

   return newInputDelta;
}

void UOSEAimAssistComponent::AddAimAssistTarget(const FAimAssistTarget& target)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UOSEAimAssistComponent_AddAimAssistTarget);
   FAimAssistTarget& arrTarget = _aimAssistTargets.Emplace_GetRef(target);
   arrTarget._CacheData(_GetOwningPlayerController(), MinimumBoxSizeWorld);
}

#if !UE_BUILD_SHIPPING
void UOSEAimAssistComponent::DebugDrawHUD(AHUD& hud)
{
   if (!AimAssistCVars::DebugDraw)
      return;

   // NOTE: This component ticks early in the frame and the hud ticks late, so all
   // the data should be setup here for the frame it was processed

   for(const FAimAssistTarget& target : _debugDrawAimTargets)
   {
      FLinearColor innerColor = target.IsTarget() ? FLinearColor::Green : FColor::Cyan;
      FLinearColor outerColor = target.IsTarget() ? FColor::Emerald : FLinearColor::Blue;
      AimAssistUtl::DebugDrawBox(hud, target.GetScreenSpaceBoxInner(), innerColor);
      AimAssistUtl::DebugDrawBox(hud, target.GetScreenSpaceBoxOuter(), outerColor);
      AimAssistUtl::DebugDrawPoint(hud, target.GetScreenSpaceCenterLoc(), innerColor, 15.0f);
      DrawDebugPoint(GetWorld(), target.GetCenterLoc(), 15.0f, FColor::Purple);
      
      if(target.IsTarget())
      {
         AimAssistUtl::DebugDrawWeightText(hud, target.GetScreenSpaceBoxOuter(), outerColor, target.GetAimAssistStrength());
         AimAssistUtl::DebugDrawPoint(hud, target.GetInnerClosestPoint(), FLinearColor::Red, 10.0f);
         AimAssistUtl::DebugDrawPoint(hud, target.GetOuterClosestPoint(), FLinearColor::Red, 10.0f);
      }
   }

   _debugDrawAimTargets.Reset();
}
#endif

void UOSEAimAssistComponent::_FilterAimAssistTargets(APawn& owningPawn, const FTransform& pawnEyesXfm)
{
   for(auto it = _aimAssistTargets.CreateIterator(); it; ++it)
   {
      const FAimAssistTarget& target = (*it);

      // center of our target must be on screen
      if (!target._isCenterProjectedOnScreen)
      {
         it.RemoveCurrent();
         continue;
      }

      const FVector& centerLoc = target.GetCenterLoc();

      // no known location?  throw it out
      {
         if (centerLoc.IsZero())
         {
            it.RemoveCurrent();
            continue;
         }
      }

      // TODO: max distance check for the whole system?

      // is this object even in front of our sight?
      {
         FVector pawnEyesFwd = pawnEyesXfm.GetRotation().Vector();
         FVector targetFwd = (centerLoc - pawnEyesXfm.GetLocation());
         targetFwd.Normalize();
         float dot = pawnEyesFwd | targetFwd;
         if (dot < 0.0f)
         {
            it.RemoveCurrent();
            continue;
         }
      }

      // visibility check
      if(!target.SkipVisibility)
      {
         static const float kTolerance = 0.1f;
         static const bool kTraceComplex = false;
         FCollisionQueryParams params(SCENE_QUERY_STAT(UOSEAimAssistComponent_FilterAimAssistTargets), kTraceComplex);
         params.bReturnPhysicalMaterial = false; // don't need this
         params.bIgnoreTouches = true; // no overlaps, only blocks
         params.AddIgnoredActor(&owningPawn);
         if (target.BoundsActor.IsValid())
         {
            AActor* actor = target.BoundsActor.Get();
            params.AddIgnoredActor(actor);
         }
         if (target.BoundsComponent.IsValid())
         {
            USceneComponent* boundsComp = target.BoundsComponent.Get();
            params.AddIgnoredActor(boundsComp->GetOwner());
            if (UPrimitiveComponent* primitive = Cast<UPrimitiveComponent>(boundsComp))
            {
               params.AddIgnoredComponent(primitive);
            }
         }
         if (target.CenterComponent.IsValid())
         {
            USceneComponent* centerComp = target.CenterComponent.Get();
            params.AddIgnoredActor(centerComp->GetOwner());
            if (UPrimitiveComponent* primitive = Cast<UPrimitiveComponent>(centerComp))
            {
               params.AddIgnoredComponent(primitive);
            }
         }

         FHitResult hitResult;
         GetWorld()->LineTraceSingleByChannel(hitResult, pawnEyesXfm.GetLocation(), centerLoc, ECollisionChannel::ECC_Visibility, params);

         FVector impactPoint = hitResult.ImpactPoint;
         if (hitResult.bBlockingHit && !(FMath::IsNearlyEqual(impactPoint.X, centerLoc.X, kTolerance) &&
            FMath::IsNearlyEqual(impactPoint.Y, centerLoc.Y, kTolerance) &&
            FMath::IsNearlyEqual(impactPoint.Z, centerLoc.Z, kTolerance)))
         {
            it.RemoveCurrent();
            continue;
         }
      }
   }
}

FAimAssistTarget* UOSEAimAssistComponent::_FindAimAssistTarget(const AOSEPlayerController& owningPC, const FTransform& pawnEyesXfm)
{
   // Potential TODOs:
   // - Priority / context sorting
   // - Frame-to-frame momentum bonus for a target

   FAimAssistTarget* bestTarget = nullptr;
   float bestWeight = float(INDEX_NONE);
   float bestDistance = float(INDEX_NONE);
   
   int32 sizeX, sizeY;
   owningPC.GetViewportSize(sizeX, sizeY);

   const FVector2D viewportSize = FVector2D(sizeX, sizeY);
   const FVector2D viewportCenter = viewportSize / 2.0f;

   for(FAimAssistTarget& target : _aimAssistTargets)
   {
      const FBox2D& innerBox = target.GetScreenSpaceBoxInner();
      const FBox2D& outerBox = target.GetScreenSpaceBoxOuter();
      float targetWeight = float(INDEX_NONE);

      if (innerBox.IsInside(viewportCenter))
      {
         targetWeight = 1.0f;
      }
      else if (outerBox.IsInside(viewportCenter))
      {
         const FVector2D innerBoxPt = innerBox.GetClosestPointTo(viewportCenter);
         const FVector2D outerBoxPt = AimAssistUtl::GetNearestPointInPerimeter(outerBox, viewportCenter);
         
         const float innerDistance = FVector2D::Distance(viewportCenter, innerBoxPt);
         const float outerDistance = FVector2D::Distance(viewportCenter, outerBoxPt);
         const float totalDistance = FVector2D::Distance(innerBoxPt, outerBoxPt);
         if (totalDistance > 0.0f)
         {
            targetWeight = 1.0f - (innerDistance / totalDistance);
         }
         
#if !UE_BUILD_SHIPPING
         // some additional caching for debug drawing purposes
         if (AimAssistCVars::DebugDraw)
         {
            target._innerClosestPoint = innerBoxPt;
            target._outerClosestPoint = outerBoxPt;
         }
#endif
      }

      bool isNewBestTarget = false;
      if (targetWeight > bestWeight)
      {
         isNewBestTarget = true;
      }
      else if (targetWeight != float(INDEX_NONE) && targetWeight == bestWeight)
      {
         check(bestTarget);

         // break ties via distance
         float targetDistance = FVector::Distance(pawnEyesXfm.GetLocation(), target.GetCenterLoc());
         if (targetDistance < bestDistance)
         {
            isNewBestTarget = true;
         }
      }

      // found a new one!
      if (isNewBestTarget)
      {
         bestTarget = &target;
         bestWeight = targetWeight;
         bestDistance = FVector::Distance(pawnEyesXfm.GetLocation(), bestTarget->GetCenterLoc());
         target._SetAimAssistStrength(targetWeight);
      }
   }

   if (bestTarget)
   {
      bestTarget->_isTarget = true;
      UE_LOG(LogAimAssist, VeryVerbose, TEXT("Best target is %s"), *bestTarget->GetDebugName());
   }

#if !UE_BUILD_SHIPPING
   if (AimAssistCVars::DebugDraw)
   {
      // when debug drawing keep a separate set of targets that aren't cleared after input processing, for debug drawing
      _debugDrawAimTargets = _aimAssistTargets;
   }
#endif

   return bestTarget;
}

const AOSEPlayerController& UOSEAimAssistComponent::_GetOwningPlayerController() const
{
   return *CastChecked<AOSEPlayerController>(GetOwner());
}

