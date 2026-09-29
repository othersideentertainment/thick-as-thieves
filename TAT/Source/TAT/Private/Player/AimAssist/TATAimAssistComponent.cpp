// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/AimAssist/TATAimAssistComponent.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"

// ose
#include "OSECoreCollision.h"
#include "Character/OSECharacterMovement.h"
#include "Player/OSEPlayerController.h"
#include "Interactables/InteractableInterface.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Traversal/Mantle/OSEMantleQuery.h"

// ue4
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAimAssistComponent)

//---------------------------------------------------------------------------------------
// UOSEAimAssistComponent
//---------------------------------------------------------------------------------------

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<int32> CVarTATAimAssistVrilwireMantleDebugVis(
   TEXT("TAT.AimAssist.VrilwireMantleDebugVis"),
   0,
   TEXT("If 1, shows debug visuals for the mantle-based vrilwire aim assist")
);

static TAutoConsoleVariable<int32> CVarTATAimAssistInteractableDebugVis(
   TEXT("TAT.AimAssist.InteractableDebugVis"),
   0,
   TEXT("If 1, shows debug visuals for the interactable aim assist")
);
#endif

static TAutoConsoleVariable<int32> CVarTATAimAssistEnableVrilwireMantleAimAssist(
   TEXT("TAT.AimAssist.EnableVrilwireMantleAimAssist"),
   1,
   TEXT("If 0, doesn't run the mantle-based vrilwire aim assist \n")
   TEXT("If 1 (default), enables it")
);

UTATAimAssistComponent::UTATAimAssistComponent()
   : Super()
{
}

void UTATAimAssistComponent::BeginPlay()
{
   Super::BeginPlay();
}

FRotator UTATAimAssistComponent::ProcessAimAssist(const AOSEPlayerController& owningPC, const FRotator& inputRot, float deltaTime)
{
   if (ACharacter* character = Cast<ACharacter>(owningPC.GetPawn()))
   {
      _DoInteractionAimAssist(character);

      // If we've enabled the mantle-based vrilwire aim assist
      if (CVarTATAimAssistEnableVrilwireMantleAimAssist.GetValueOnGameThread() != 0)
      {
         bool hasVrilwireEquipped = false;
         if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(owningPC.GetPawn()))
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
            {
               hasVrilwireEquipped = toolSetInterface->HasToolEquipped(VrilwireToolTag);
            }
         }

         // Check if we have the vrilwire equipped, and if so run the mantle-based aim assist
         if (hasVrilwireEquipped)
         {
            _DoMantleBasedVrilwireAimAssist(character, inputRot, deltaTime);
         }
      }
   }

   return Super::ProcessAimAssist(owningPC, inputRot, deltaTime);
}

void UTATAimAssistComponent::_DoInteractionAimAssist(ACharacter* character)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATAimAssistComponent::_DoInteractionAimAssist)
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATAimAssistComponent_InteractableAimAssist);
   // TODO: still too expensive

   TArray<const UObject*, TInlineAllocator<32>> testedObjects;
   TArray<AActor*, TInlineAllocator<16>> foundActors;
   
   FOverlapDatum overlapDatum;
   {
      // Find async overlaps from last frame
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATAimAssistComponent::QueryOverlapData)
      GetWorld()->QueryOverlapData(_interactTraceHandle, overlapDatum);
   }
   for (const FOverlapResult& overlap : overlapDatum.OutOverlaps)
   {
      UPrimitiveComponent* component = overlap.GetComponent();
      if (!UOSEInteractionHelpers::IsComponentTargetableForInteraction(component))
      {
         continue;
      }

      AActor* actor = overlap.GetActor();
      // NOTE: If there are significant numbers of non-pawns with separate AimAssistProviders,
      //       can check for AimAssistProvider first. As is, costs more to check, assuming pawns
      //       already excluded.

      if (actor && actor->Implements<UInteractableInterface>() && !testedObjects.Contains(actor))
      {
         testedObjects.Add(actor);
         if (IInteractableInterface::Execute_IsInteractable(actor, character))
         {
            foundActors.AddUnique(actor);
         }
      }
      else if (component && component->Implements<UInteractableInterface>() && !testedObjects.Contains(component))
      {
         // NOTE: This fallback logic does not precisely match the actual logic in actual interaction targeting,
         //       but it what UOSEInteractionHelpers::SphereOverlapInteractables was doing previously. Possibly
         //       something to fix.
         testedObjects.Add(component);
         if (IInteractableInterface::Execute_IsInteractable(component, character))
         {
            foundActors.AddUnique(actor);
         }
      }
   }

   for (AActor* actor : foundActors)
   {
      // This is a pretty loose heuristic for aim assisting against interactables... my assumption here is that we
      // want to aim assist on whatever the visual highlight is.  The problem with this behavior is that it's not a requirement
      // that every interactable has a highlightable object, so we could just fail to aim assist some things.
      // In those cases we could use the components we hit in this overlap check, but often those have huge collion spheres, which
      // are much too big for aim assist.

      TArray<UActorComponent*> interactTagComponents = actor->GetComponentsByTag(USceneComponent::StaticClass(), UTATInteractHighlightUtils::InteractHighlightTag_NAME);
      if (interactTagComponents.Num() > 0) // could maybe use more than one and combine the hitboxes...?
      {
         FAimAssistTarget target;
         target.BoundsActor = actor;

         // For now still only get the first component, until things with discontinuous interact areas can opt out
         // (like double doors)
         FBox box(ForceInit);
         for (UActorComponent* interactComponent : interactTagComponents)
         {
            const auto* prim = CastChecked<USceneComponent>(interactComponent);
            if (prim->GetCollisionResponseToChannel(COLLISION_INTERACT) == ECR_Block)
            {
               box += prim->Bounds.GetBox();
               break;
            }
         }

         if (!box.IsValid)
         {
            continue;
         }

         target.RawCenter = box.GetCenter();
         target.RawExtents = box.GetExtent();
         target.UseRawCenterAndExtents = true;

#if ENABLE_DRAW_DEBUG
         if (CVarTATAimAssistInteractableDebugVis.GetValueOnGameThread())
         {
            DrawDebugBox(GetWorld(), target.RawCenter, target.RawExtents, FQuat::Identity, FColor::Orange, false, -1.0f, 0U, 0);
         }
#endif

         target.InnerBoxSizeMultiplier = InteractableAimAssistInnerBoxMultiplier;
         target.OuterBoxSizeMultiplier = InteractableAimAssistOuterBoxMultiplier;
         AddAimAssistTarget(target);
      }
   }

   {
      // queue async overlaps for next frame
      const FCollisionShape sphere = FCollisionShape::MakeSphere(InteractableDetectionDistance);
      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(SphereOverlapInteractables));
      queryParams.AddIgnoredActor(character);

      // Ignore pawns, assuming they already have aim assist providers
      FCollisionResponseParams responseParams = FCollisionResponseParams::DefaultResponseParam;
      responseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

      _interactTraceHandle = GetWorld()->AsyncOverlapByChannel(character->GetActorLocation(), FQuat::Identity, COLLISION_INTERACT, sphere, queryParams, responseParams);
   }
}

void UTATAimAssistComponent::_DoMantleBasedVrilwireAimAssist(ACharacter* character, const FRotator& inputRot, float deltaTime)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATAimAssistComponent_VrilwireAimAssist);
   UOSECharacterMovement* characterMovement = character->GetCharacterMovement<UOSECharacterMovement>();

   if (!characterMovement)
   {
      return;
   }

   FOSEMantleSettings aimAssistMantleSettings = characterMovement->GetMantleSettings();
   aimAssistMantleSettings.LookAheadTime = VrilwireAimAssistOverreach;
   if (VrilwireOverrideMaxAngleFacingForMantleQuery)
   {
      aimAssistMantleSettings.MaxAngleFacing = VrilwireMaxAngleFacingForMantleQuery;
   }

   FVector eyesLoc;
   FRotator pawnEyesRot;
   character->GetActorEyesViewPoint(eyesLoc, pawnEyesRot);

   // Doing a fairly naive "vertical shotgun" of mantle traces
   // it attempts ones with aims above and below the current aim
   //
   // This seems to work fairly well, but some geometry is more prone to sampling error
   for(int i = 0; i < VrilwireSearchCount; ++i)
   {
      float pitch = i * VrilwirePitchSpread;
      {
         const FRotator searchRotation = (pawnEyesRot + inputRot + FRotator(pitch, 0, 0)).GetNormalized();
         if (_TryMantleSweep(searchRotation, eyesLoc, character, aimAssistMantleSettings) && VrilwireStopOnFirstTarget)
         {
            break;
         }
      }

      if(i > 0)
      {
         const FRotator searchRotation = (pawnEyesRot + inputRot + FRotator(-pitch, 0, 0)).GetNormalized();
         if (_TryMantleSweep(searchRotation, eyesLoc, character, aimAssistMantleSettings) && VrilwireStopOnFirstTarget)
         {
            break;
         }
      }
   }
}

bool UTATAimAssistComponent::_TryMantleSweep(const FRotator& searchRotation, const FVector& eyesLoc, ACharacter* character, const FOSEMantleSettings& aimAssistMantleSettings)
{
   const FVector spherecastEnd = eyesLoc + searchRotation.Vector() * MaxVrilwireDistance;

   UCapsuleComponent* capsule = character->GetCapsuleComponent();

   const ECollisionChannel collisionChannel = capsule->GetCollisionObjectType();
   FCollisionResponseParams collisionResponseParams;
   FCollisionQueryParams collisionQueryParams;
   capsule->InitSweepCollisionParams(collisionQueryParams, collisionResponseParams);

   collisionQueryParams.AddIgnoredActor(character);

   UWorld* world = GetWorld();

   // Find any in-range geometry that we're probably aiming at with the vrilwire
   FHitResult hitResult;
   world->SweepSingleByChannel(hitResult, eyesLoc, spherecastEnd, FQuat::Identity, collisionChannel, FCollisionShape::MakeSphere(VrilwireInitialTraceRadius), collisionQueryParams, collisionResponseParams);

   // If there's nothing in range, don't bother creating an aim assist target
   if (!hitResult.bBlockingHit)
   {
      return false;
   }

   const FVector searchEndPoint = hitResult.Location;

   FVector searchStartPoint = character->GetActorLocation();
   const FVector endPointToStartingPoint = searchStartPoint - searchEndPoint;

   // If we're too close to the ledge anyway, don't bother creating an aim assist target
   if (endPointToStartingPoint.Length() >= MinVrilwireDistance)
   {
      searchStartPoint = searchEndPoint + endPointToStartingPoint.GetSafeNormal() * VrilwireMantleQueryStartingDistance;

      // Clamp our starting point to the same plane as 
      searchStartPoint.Z = searchEndPoint.Z;

      const FVector velocity = (searchEndPoint - searchStartPoint) * VrilwireAimAssistOverreach;

#if ENABLE_DRAW_DEBUG
      if (CVarTATAimAssistVrilwireMantleDebugVis.GetValueOnGameThread() != 0)
      {
         DrawDebugBox(world, searchStartPoint, FVector(10.0f, 10.0f, 10.0f), FColor::Green, false, -1.0f, 0U, 2.0f);
         DrawDebugDirectionalArrow(world, searchStartPoint, searchEndPoint, 4.0f, FColor::Green, false, -1.0f, 0U, 2.0f);
         DrawDebugBox(world, searchEndPoint, FVector(10.0f, 10.0f, 10.0f), FColor::Blue, false, -1.0f, 0U, 2.0f);
      }
#endif

      FOSEMantleQueryResult mantleQueryResult = UOSEMantleQuery::TraceMantleCapsuleWithVelocityAndPosition(aimAssistMantleSettings, capsule, searchStartPoint, searchRotation, velocity);
      if (mantleQueryResult.IsValid())
      {
         const FVector mantleEndLocation = mantleQueryResult.FinalHitResult.Location;

         // Find the floor instead of where the center of the character's body ends up
         const FVector mantleEndFloor = mantleEndLocation - FVector(0.0f, 0.0f, capsule->GetScaledCapsuleHalfHeight()) + VrilwireEdgeTargetBias;

         // The extents are globally configured: we're mantling to a location so we don't have a generic idea of its extents
         const FVector Extents(VrilwireEdgeTargetExtents, VrilwireEdgeTargetExtents, VrilwireEdgeTargetExtents);

#if ENABLE_DRAW_DEBUG
         if (CVarTATAimAssistVrilwireMantleDebugVis.GetValueOnGameThread() != 0)
         {
            DrawDebugBox(world, mantleEndFloor, FVector(12.0f, 12.0f, 12.0f), FColor::White, false, -1.0f, 0U, 2.0f);
         }
#endif

         FAimAssistTarget target;
         target.UseRawCenterAndExtents = true;
         target.RawExtents = Extents;
         target.RawCenter = mantleEndFloor;
         target.InnerBoxSizeMultiplier = VrilwireAimAssistInnerBoxMultiplier;
         target.OuterBoxSizeMultiplier = VrilwireAimAssistOuterBoxMultiplier;
         target.SkipVisibility = true; // assume it is visible if the mantle query found it. This might not be 100% true, but using the actor hit at the start may find the wrong one
         AddAimAssistTarget(target);

         return true;
      }
   }

   return false;
}

