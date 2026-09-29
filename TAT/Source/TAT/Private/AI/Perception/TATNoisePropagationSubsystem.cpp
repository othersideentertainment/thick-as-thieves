// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/TATNoisePropagationSubsystem.h"

// tat
#include "AI/TATAISettings.h"
#include "AI/Perception/TATHearingTypes.h"
#include "Audio/TATAudioRoomComponent.h"

// ose
#include "OSECoreCollision.h"

// wwise
#include "AkAcousticPortal.h"
#include "AkAudioDevice.h"
#include "AkRoomComponent.h"

// ue5
#include "DrawDebugHelpers.h"
#include "Components/BrushComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNoisePropagationSubsystem)

namespace TATHearingCVars
{
   static int DrawNoisePropagation = 0;
   FAutoConsoleVariableRef CVarDebugDrawDrawNoisePropagation(
      TEXT("TAT.Perception.NoisePropagation.DebugDraw"),
      DrawNoisePropagation,
      TEXT("(default 0) Whether to enable debug drawing for noise propagation"),
      ECVF_Default);

   static TAutoConsoleVariable<float> CVarDebugDrawNoisePropagationLifetime(
      TEXT("TAT.Perception.NoisePropagation.DebugDrawLifetime"),
      1.0f,
      TEXT("(default 1.0f) How long the debug draw visuals for a noise stim propagation should stick around")
   );

   static TAutoConsoleVariable<int32> CVarForceNoisePropagationMethod(
      TEXT("TAT.Perception.NoisePropagation.ForceMethod"),
      -1,
      TEXT("What method, if any, to force the propagation system.\n")
      TEXT("-1 (default) - Don't force a method, use the configured propagation method for the stim\n")
      TEXT(" 0 - Force to RequireDirectLOS\n")
      TEXT(" 1 - Force to CheckOnlyDistance\n")
      TEXT(" 2 - Force to PropogateThroughOpenPortals\n")
   );

   static TAutoConsoleVariable<int32> CVarRoomPortalDebugDraw(
      TEXT("TAT.Perception.NoisePropagation.DebugDrawRoomsPortal"),
      0,
      TEXT("If enabled, debug draw some visuals to show rooms, portals, and their connections\n")
      TEXT("Rooms are shown in red. The room the player is currently in is drawn in the foreground,\n")
      TEXT("along with its portals. The portals are cyan when open, and orange when closed")
   );

   static TAutoConsoleVariable<int32> CVarRoomPortalDebugDrawThickness(
      TEXT("TAT.Perception.NoisePropagation.DebugDrawRoomsPortalThickness"),
      2.0f,
      TEXT("(default 2.0) The thickness of lines when debug drawing room/portal connections")
   );
}

DECLARE_STATS_GROUP(TEXT("TATNoisePropagationSubsystem"), STATGROUP_TATNoisePropagationSubsystem, STATCAT_Advanced);

DEFINE_LOG_CATEGORY_STATIC(LogTATNoisePropagationSubsystem, Log, All);

namespace TATNoisePropagationHelpers
{

#if DO_CHECK
static FString GetOwnerNameSafe(const UActorComponent* comp)
{
   return GetNameSafe(comp ? comp->GetOwner() : nullptr);
}
#endif

}

ETickableTickType UTATNoisePropagationSubsystem::GetTickableTickType() const
{
#if ENABLE_DRAW_DEBUG
   return Super::GetTickableTickType();
#else
   // In builds w/o debug drawing, don't ever tick
   return ETickableTickType::Never;
#endif
}

#if ENABLE_DRAW_DEBUG
static void _DrawConvexElement(UWorld* world, AActor* owningActor, const FKConvexElem& convexElement, FTransform elemTM, const FColor color, ESceneDepthPriorityGroup depthPriority, float thickness)
{
   // NOTE: For now, we're assuming that these are always
   // We can handle other cases, (see `FKConvexElem::DrawElemWire`), but for now assume a bounding box,
   // and log a warning if that's not the case
   // One thing to note is that drawing the arbitrary mesh using triangles would lead to diagonals on the bounding box,
   // which can add more visual noise. Hence why it's worth special-casing here
   if (convexElement.VertexData.Num() != 8)
   {
      UE_LOG(LogTATNoisePropagationSubsystem, Warning,
         TEXT("Debug drawing an AkRoom's convex element on actor '%s' with %d vertices: we expected a box, ")
         TEXT("so a full convex mesh should be handled separately"),
         *owningActor->GetName(),
         convexElement.VertexData.Num());
      return;
   }

   const FQuat rotation = elemTM.GetRotation();
   const FVector boxCenter = elemTM.TransformPosition(convexElement.ElemBox.GetCenter());
   const FVector boxExtents = elemTM.GetScale3D() * convexElement.ElemBox.GetExtent();

   DrawDebugBox(world, boxCenter, boxExtents, rotation, color, false, 0.0f, depthPriority, thickness);
}

static void _DebugDrawLocalBoundsForComponent(UWorld* world, const USceneComponent* component, FColor color, ESceneDepthPriorityGroup depthPriority, float thickness)
{
   // NB: We could just get the bounds from the component, however that would be an AABB. If the portal/room has a non-axis-aligned rotation
   // then the debug visuals may not be as clear. Instead, we compute a local bounds that doesn't take rotation into account, meaning the AABB will be
   // aligned to the portal's local axes. Then, we re-apply the rotation when rendering it in the debug draw call, so that it will give the appearance of an OBB
   FTransform componentTransform = component->GetComponentTransform();
   componentTransform.SetRotation(FQuat::Identity);

   // NB: This has to be done on the attach parent because the portal component/room doesn't override CalcBounds, and so just gives the default
   // zero-sized bounds. It is expected to be attached to a box or other primitive component that does have a proper bounds,
   // and since the proal just inherits its parent component's transform we can use it
   const FBoxSphereBounds localBounds = component->GetAttachParent()->CalcBounds(componentTransform);

   const FVector boundsCenter = localBounds.Origin;
   const FVector boundsExtents = localBounds.BoxExtent;
   const FQuat componentRotation = component->GetComponentRotation().Quaternion();

   DrawDebugBox(world, boundsCenter, boundsExtents, componentRotation, color, false, -1.0f, depthPriority, thickness);
}

static void _DebugDrawRoomComponent(UWorld* world, const UAkRoomComponent* roomComponent, FColor color, ESceneDepthPriorityGroup depthPriority, float thickness)
{
   bool convexElementsDrawn = false;
   if (UBrushComponent* brush = Cast<UBrushComponent>(roomComponent->GetAttachParent()))
   {
      if (UBodySetup* brushBodySetup = brush->BrushBodySetup)
      {
         const FKAggregateGeom& aggGeom = brushBodySetup->AggGeom;

         // NB: If we select "fit to geometry," even using a box, it still generates a convex shape for the Brush
         // We can potentially handle other scenarios, but until then focus on this code path
         if (aggGeom.GetElementCount() != aggGeom.GetElementCount(EAggCollisionShape::Convex))
         {
            UE_LOG(LogTATNoisePropagationSubsystem, Warning,
               TEXT("Debug drawing AkRoom on actor '%s', which has non-convex aggregate collision shapes. ")
               TEXT("This can be supported, but we assume it doesn't happen for now"),
               *roomComponent->GetOwner()->GetName());
         }

         for (const FKConvexElem& convexElem : aggGeom.ConvexElems)
         {
            convexElementsDrawn = true;
            _DrawConvexElement(world, roomComponent->GetOwner(), convexElem, brush->GetComponentTransform(), color, depthPriority, thickness);
         }
      }
   }

   // If we don't have any convex elements, then we likely are not fitting the room to geometry
   // Instead, just draw the bounds to give some visual feedback
   if (!convexElementsDrawn)
   {
      _DebugDrawLocalBoundsForComponent(world, roomComponent, color, depthPriority, thickness);
   }
}

void UTATNoisePropagationSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   if (TATHearingCVars::CVarRoomPortalDebugDraw.GetValueOnGameThread() == 0)
   {
      return;
   }

   if (const APlayerController* pc = GetWorld()->GetFirstPlayerController())
   {
      FVector playerLocation;
      FRotator playerRotation;

      pc->GetActorEyesViewPoint(playerLocation, playerRotation);

      const UAkRoomComponent* currentPlayerRoom =  _FindRoomForLocation(playerLocation);

      const float thickness = TATHearingCVars::CVarRoomPortalDebugDrawThickness.GetValueOnGameThread();

      for (const TPair<const UAkRoomComponent*, FTATPortalsForRoom>& roomAndPortals : _roomToPortals)
      {
         const UAkRoomComponent* roomComponent = roomAndPortals.Key;

         if (IsValid(roomComponent))
         {
            // Enunciate the room the player actor is currently in by drawing it brighter and in the foreground
            if (currentPlayerRoom == roomComponent)
            {
               _DebugDrawRoomComponent(GetWorld(), roomComponent, FColor::Red, SDPG_Foreground, thickness);

               for (const FTATPortalConnection& portal : roomAndPortals.Value.Portals)
               {
                  const UAkPortalComponent* portalComponent = portal.Portal;
                  const FColor portalColor = (portalComponent->GetCurrentState() == AkAcousticPortalState::Open) ? FColor::Cyan : FColor::Orange;
                  _DebugDrawLocalBoundsForComponent(GetWorld(), portalComponent, portalColor, SDPG_Foreground, thickness);
               }
            }
            else
            {
               _DebugDrawRoomComponent(GetWorld(), roomComponent, FColor(100, 0, 0), SDPG_World, thickness);
            }
         }
      }
   }
}
#endif

TStatId UTATNoisePropagationSubsystem::GetStatId() const
{
   RETURN_QUICK_DECLARE_CYCLE_STAT(UTATNoisePropagationSubsystem, STATGROUP_Tickables);
}

bool UTATNoisePropagationSubsystem::IsNoiseEventHeardByListener(
   const FVector& noiseLocation,
   const AActor* listenerActor,
   const FVector& listenerLocation,
   const FTATHearingEventStimSettings& stimSettings,
   float maxHearingRange,
   FVector& outPerceivedStimLocation) const
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Is Noise Heard by Listener"), STAT_TATNoisePropagationSubsystem_IsNoiseEventHeardByListener, STATGROUP_TATNoisePropagationSubsystem);

   // By default, stims are heard at their true location
   outPerceivedStimLocation = noiseLocation;

   // they require a body
   if (!listenerActor)
   {
      UE_LOG(LogTATNoisePropagationSubsystem, Warning, TEXT("IsNoiseEventHeardByListener passed a null listener actor for noise stim of tag '%s'"),
         *stimSettings.Tag.ToString());
      return true;
   }

   ETATHearingEventPropagationMethod propagationMethod = stimSettings.VolumeLevel;

#if !UE_BUILD_SHIPPING
   const int32 forcePropagationMethod = TATHearingCVars::CVarForceNoisePropagationMethod.GetValueOnGameThread();
   if (forcePropagationMethod != -1)
   {
      constexpr int32 maxValue = static_cast<int32>(ETATHearingEventPropagationMethod::Count) - 1;
      propagationMethod = static_cast<ETATHearingEventPropagationMethod>(FMath::Clamp(forcePropagationMethod, 0, maxValue));
   }
#endif

   UE_LOG(LogTATNoisePropagationSubsystem, Verbose,
      TEXT("Checking if listener '%s' heard stim of tag '%s' (propagation method '%s')"),
      *listenerActor->GetName(),
      *stimSettings.Tag.ToString(),
      *UEnum::GetValueAsString(propagationMethod));
   switch(propagationMethod)
   {
   case ETATHearingEventPropagationMethod::CheckOnlyDistance:
      return true;
   case ETATHearingEventPropagationMethod::RequireDirectLOS:
      return _DoesListenerHaveLOSToLocation(noiseLocation, listenerActor, listenerLocation, nullptr);
   case ETATHearingEventPropagationMethod::PropogateThroughOpenPortals:
      return _HandleAudioPropagation(noiseLocation, listenerActor, listenerLocation, maxHearingRange, outPerceivedStimLocation);
   default:
      checkNoEntry();
      return false;
   }
}
   

void UTATNoisePropagationSubsystem::RegisterPortalComponent(UAkPortalComponent* portal)
{
   check(portal);

   FTATRoomsForPortal newRoomsForPortal = _ComputeCurrentRoomsForPortal(portal);
   check(!_portalToRooms.Contains(portal));
   _portalToRooms.Add(portal, newRoomsForPortal);

   _AddRoomPortalConnection(portal, newRoomsForPortal.FrontRoom);
   _AddRoomPortalConnection(portal, newRoomsForPortal.BackRoom);
}

void UTATNoisePropagationSubsystem::UnregisterPortalComponent(UAkPortalComponent* portal)
{
   check(_portalToRooms.Contains(portal));

   FTATRoomsForPortal roomsForPortal = _GetRoomsForPortal(portal);
   _RemoveRoomPortalConnection(portal, roomsForPortal.FrontRoom);
   _RemoveRoomPortalConnection(portal, roomsForPortal.BackRoom);

   _portalToRooms.Remove(portal);
}

// Based on FAkEnvironmentIndex::Add
void UTATNoisePropagationSubsystem::RegisterRoomComponent(UAkRoomComponent* room)
{
   FAkEnvironmentOctreeElement element(room);
   _roomOctree.AddElement(element);
}

// Based on FAkEnvironmentIndex::Remove
void UTATNoisePropagationSubsystem::UnregisterRoomComponent(UAkRoomComponent* room)
{
   AK_OCTREE_ELEMENT_ID* id = _roomOctree.ObjectToOctreeId.Find(room->GetUniqueID());
   if (id != nullptr && _roomOctree.IsValidElementId(*id))
   {
      _roomOctree.RemoveElement(*id);
   }

   _roomOctree.ObjectToOctreeId.Remove(room->GetUniqueID());
}

void UTATNoisePropagationSubsystem::OnPortalComponentRoomConnectionsUpdated(UAkPortalComponent* portal)
{
   UnregisterPortalComponent(portal);
   RegisterPortalComponent(portal);
}

void UTATNoisePropagationSubsystem::_AddRoomPortalConnection(UAkPortalComponent* portal, const UAkRoomComponent* room)
{
   check(portal);

   if (room)
   {
      FTATPortalsForRoom& portalsForRoom = _roomToPortals.FindOrAdd(room);
      portalsForRoom.AddPortal(portal);
   }
}

void UTATNoisePropagationSubsystem::_RemoveRoomPortalConnection(UAkPortalComponent* portal, const UAkRoomComponent* room)
{
   check(portal);
   
   if (room)
   {
      FTATPortalsForRoom* portalsForRoom = _roomToPortals.Find(room);
      checkf(portalsForRoom, TEXT("Trying to remove portal '%s' from room '%s' but could not find list"),
         *portal->GetName(),
         // NB: UAkRoomComponent defines its own GetName() which hides the UObject one, and gives the name of its parent component
         *room->UObject::GetName());

      portalsForRoom->RemovePortal(portal);
   }
}

#if DO_CHECK

void UTATNoisePropagationSubsystem::_EnsurePortalRoomsMatchWWise(const UAkPortalComponent* portal, const UAkRoomComponent* expectedRoom, const UAkRoomComponent* actualRoom, const FVector& portalLocation)
{
   check(portal);

   // If we are actually running with sound, check what wwise thinks the portals are
   if (FAkAudioDevice::Get() != nullptr)
   {
      if (expectedRoom != actualRoom)
      {
         // The following is checking for the situation where we have a portal that is actually in 2 rooms at once, with equal priority
         // That's not ideal, but it doesn't indicate an issue with how we calculate the room/portal connections,
         // it's instead just that which room the portal is actually in is ambiguous and depends on octree iteration order
         // Still worth logging a warning though
         if (actualRoom != nullptr
            && expectedRoom != nullptr
            && actualRoom->HasEffectOnLocation(portalLocation)
            && expectedRoom->Priority == actualRoom->Priority)
         {
            UE_LOG(LogTATNoisePropagationSubsystem, Warning,
               TEXT("The portal on '%s' has either its front or back inside two rooms at once of equal priority: '%s' and '%s'. ")
               TEXT("Their priorities should be different to indicate which one takes precedence, or they should not be overlapping"),
               *TATNoisePropagationHelpers::GetOwnerNameSafe(portal),
               *TATNoisePropagationHelpers::GetOwnerNameSafe(expectedRoom),
               *TATNoisePropagationHelpers::GetOwnerNameSafe(actualRoom));
         }
         // Secondly, we may have a discrepancy due to rooms having UseForNoisePropagation set to false
         // In that case, wwise may connect the portal to a room that we don't index, and so wouldn't use
         // For that, check that we aren't computing a totally wrong portal, but otherwise allow discrepancies
         else if (expectedRoom && !CastChecked<UTATAudioRoomComponent>(expectedRoom)->UseForNoiseStimPropagation)
         {
            // If we do have a portal, at least ensure that it should be used for the portal
            // Ideally, we'd try to check our result against what wwise would get if they discounted any rooms with UseForNoisePropagation set to false,
            // but that may prove more complicated and require more dependencies on the internals of wwise
            if (actualRoom != nullptr)
            {
               ensureMsgf(
                  actualRoom->HasEffectOnLocation(portalLocation),
                  TEXT("Discrepancy of room for portal '%s': we computed '%s' which should not have an effect on the portal's location %s"),
                  *TATNoisePropagationHelpers::GetOwnerNameSafe(portal),
                  *TATNoisePropagationHelpers::GetOwnerNameSafe(actualRoom),
                  *portalLocation.ToString());
            }
         }
         else
         {
            // NB: Disabling this ensure with the Wwise 2023.1.1 upgrade to suppress a failed ensure when a portal is registered before its 
            // connecting rooms are assigned, likely due to _OnUpdateConnectedRooms() being called unconditionally.
            // Going forward, we can likely rely on Wwise's newly-introduced room -> portal mapping (see UAkRoomComponent member ConnectedPortals)
            // 
            // If the above conditions don't hold, then we have an issue where we are not computing the rooms for portals the same way
            // wwise does. That should bubble up as an ensure since we probably need to check the code
            //ensureMsgf(false,
            //   TEXT("Discrepancy of room for portal '%s': wwise says '%s', we computed '%s'"),
            //   *TATNoisePropagationHelpers::GetOwnerNameSafe(portal),
            //   *TATNoisePropagationHelpers::GetOwnerNameSafe(expectedRoom),
            //   *TATNoisePropagationHelpers::GetOwnerNameSafe(actualRoom));
         }
      }
   }
}
#endif

FTATRoomsForPortal UTATNoisePropagationSubsystem::_ComputeCurrentRoomsForPortal(const UAkPortalComponent* portal) const
{
   FVector frontPoint = FVector::ZeroVector;
   FVector backPoint = FVector::ZeroVector;
   _GetPortalFrontAndBackLocations(portal, frontPoint, backPoint);

   FTATRoomsForPortal roomsForPortal;
   roomsForPortal.FrontRoom = _FindRoomForLocation(frontPoint);
   roomsForPortal.BackRoom = _FindRoomForLocation(backPoint);

#if DO_CHECK
   // Check that we agree with wwise on what rooms the portal is connected to
   _EnsurePortalRoomsMatchWWise(portal, portal->GetFrontRoomComponent().Get(), roomsForPortal.FrontRoom, frontPoint);
   _EnsurePortalRoomsMatchWWise(portal, portal->GetBackRoomComponent().Get(), roomsForPortal.BackRoom, backPoint);
#endif

   return roomsForPortal;
}

FTATRoomsForPortal UTATNoisePropagationSubsystem::_GetRoomsForPortal(const UAkPortalComponent* portal) const
{
   if (const FTATRoomsForPortal* rooms = _portalToRooms.Find(portal))
   {
      return *rooms;
   }

   return FTATRoomsForPortal();
}

UAkPortalComponent* UTATNoisePropagationSubsystem::_FindClosestOpenPortalForRoom(const UAkRoomComponent* room, const FVector& listenerPos) const
{
   UAkPortalComponent* closestOpenPortal = nullptr;

   if (const FTATPortalsForRoom* portalsForRoom = _roomToPortals.Find(room))
   {
      float closestDistSqr = FLT_MAX;
      for (const FTATPortalConnection& portalConnection : portalsForRoom->Portals)
      {
         check(portalConnection.NumConnections > 0);
         UAkPortalComponent* portal = portalConnection.Portal;

         const float multiplier = portal->GetCurrentState() == AkAcousticPortalState::Open ? 1.f : UTATAISettings::Get().DistanceMultiplierWhenPropagatingThroughClosedPortal;
         const float distSqr = FVector::DistSquared(portal->GetComponentLocation(), listenerPos) * multiplier;
         if (distSqr < closestDistSqr)
         {
            closestOpenPortal = portal;
            closestDistSqr = distSqr;
         }
      }
   }

   return closestOpenPortal;
}

// Based on the logic in FAkEnvironmentIndex::Query
// However, using the octree type directly lets us avoid an allocation, and keep things const
UAkRoomComponent* UTATNoisePropagationSubsystem::_FindRoomForLocation(const FVector& location) const
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Find Room for Location"), STAT_TATNoisePropagationSubsystem_FindRoomForLocation, STATGROUP_TATNoisePropagationSubsystem);

   FBoxCenterAndExtent searchBounds(location, FVector::ZeroVector);

   UAkRoomComponent* foundRoom = nullptr;
   float highestRoomPriority = -FLT_MAX;

   _roomOctree.FindElementsWithBoundsTest(searchBounds, [&](const FAkEnvironmentOctreeElement& Element)
   {
      if (UAkRoomComponent* room = Cast<UAkRoomComponent>(Element.Component))
      {
         if (room->bEnable && room->HasEffectOnLocation(location))
         {
            if (room->Priority > highestRoomPriority)
            {
               highestRoomPriority = room->Priority;
               foundRoom = room;
            }
         }
      }
   });

   return foundRoom;
}

// See UAkPortalComponent::FindConnectedComponents for the logic this is based on
void UTATNoisePropagationSubsystem::_GetPortalFrontAndBackLocations(const UAkPortalComponent* portal, FVector& frontLocation, FVector& backLocation)
{
   const float x = portal->GetExtent().X;
   const FVector frontVector(x, 0.f, 0.f);

   FTransform toWorld = portal->GetAttachParent()->GetComponentTransform();
   toWorld.SetScale3D(FVector(1.0f));

   frontLocation = toWorld.TransformPosition(frontVector);
   backLocation = toWorld.TransformPosition(-1 * frontVector);
}

bool UTATNoisePropagationSubsystem::_DoesListenerHaveLOSToLocation(const FVector& location, const AActor* listenerActor, const FVector& listenerLocation, const AActor* optionalPortalOwner) const
{
   // Quick-and-dirty occlusion check doing a line-of-sight check from the origin using the AOE trace channel
   // and only blocking WorldStatic. This combination should mean that walls will block, but things like doors
   // and windows (and props) will not.
   FHitResult hit;
   FCollisionQueryParams queryParams(FName("DoesListenerHaveLOSToLocation"), SCENE_QUERY_STAT_ONLY(DoesListenerHaveLOSToLocation));
   queryParams.AddIgnoredActor(listenerActor);
   if(optionalPortalOwner)
   {
      queryParams.AddIgnoredActor(optionalPortalOwner);
   }

   const ECollisionChannel traceChannel = UTATAISettings::Get().NoiseStimTraceChannel;
   GetWorld()->LineTraceSingleByChannel(hit, listenerLocation, location, traceChannel, queryParams);

#if ENABLE_DRAW_DEBUG
   if (TATHearingCVars::DrawNoisePropagation)
   {
      const float lifetime = TATHearingCVars::CVarDebugDrawNoisePropagationLifetime.GetValueOnGameThread();
      if (hit.bBlockingHit)
      {
         DrawDebugLine(GetWorld(), hit.TraceStart, hit.ImpactPoint, FColor::Yellow, false, lifetime, SDPG_World, 0);
         DrawDebugLine(GetWorld(), hit.ImpactPoint, hit.TraceEnd, FColor::Red, false, lifetime, SDPG_World, 0);
      }
      else
      {
         DrawDebugLine(GetWorld(), hit.TraceStart, hit.TraceEnd, FColor::Green, false, lifetime, SDPG_World, 0);
      }
   }
#endif

   const bool hasLoS = !hit.bBlockingHit;
   UE_LOG(LogTATNoisePropagationSubsystem, Verbose, TEXT("LoS check returns %d"), hasLoS ? 1 : 0);

   return hasLoS;
}

bool UTATNoisePropagationSubsystem::_IsLocationInHearingRangeForRoom(
   const UAkRoomComponent* room,
   const FVector& locationOutsideRoom,
   const FVector& locationInsideRoom,
   const AActor* listenerActor,
   float maxHearingRange,
   FVector& outPortalLocation) const
{
   if (const UAkPortalComponent* closestPortal = _FindClosestOpenPortalForRoom(room, locationOutsideRoom))
   {
      const FVector portalLocation = closestPortal->GetComponentLocation();
      const float multiplier = closestPortal->GetCurrentState() == AkAcousticPortalState::Open ? 1.f : UTATAISettings::Get().DistanceMultiplierWhenPropagatingThroughClosedPortal;
      const float combinedDistance = FVector::Dist(locationInsideRoom, portalLocation) + (FVector::Dist(portalLocation, locationOutsideRoom) * multiplier);

      auto debugDrawPortalWithColor = [&](const FColor& color)
      {
#if ENABLE_DRAW_DEBUG
         if (TATHearingCVars::DrawNoisePropagation)
         {
            const float lifetime = TATHearingCVars::CVarDebugDrawNoisePropagationLifetime.GetValueOnGameThread();
            DrawDebugSphere(GetWorld(), portalLocation, 16.0f, 8, color, false, lifetime, SDPG_World);
            DrawDebugLine(GetWorld(), locationInsideRoom, portalLocation, color, false, lifetime, SDPG_World);
            DrawDebugLine(GetWorld(), portalLocation, locationOutsideRoom, color, false, lifetime, SDPG_World);
         }
#endif
      };

      // Check that the combined distance is still within range
      if (combinedDistance > maxHearingRange)
      {
         UE_LOG(LogTATNoisePropagationSubsystem, Verbose, TEXT("Combined distance for noise - portal - listener is out of range (%f vs %f)"), combinedDistance, maxHearingRange);
         debugDrawPortalWithColor(FColor::Red);
         return false;
      }

      // Check that there is LoS from the location outside the room to the portal
      if (_DoesListenerHaveLOSToLocation(portalLocation, listenerActor, locationOutsideRoom, closestPortal->GetOwner()))
      {
         debugDrawPortalWithColor(FColor::Green);
         return true;
      }
      else
      {
         debugDrawPortalWithColor(FColor::Orange);
         return false;
      }
   }
   else
   {
      // There's no open portal, so noises can't propagate in/out
      UE_LOG(LogTATNoisePropagationSubsystem, Verbose, TEXT("No open portal for the room"));
      return false;
   }
}

void FTATPortalsForRoom::AddPortal(UAkPortalComponent* portal)
{
   FTATPortalConnection* existingConnection = Portals.FindByPredicate([&](const FTATPortalConnection& portalConnection)
   {
      return portalConnection.Portal == portal;
   });

   if (existingConnection != nullptr)
   {
      existingConnection->NumConnections++;
   }
   else
   {
      FTATPortalConnection newConnection;
      newConnection.Portal = portal;
      newConnection.NumConnections = 1;
      Portals.Add(newConnection);
   }
}

void FTATPortalsForRoom::RemovePortal(UAkPortalComponent* portal)
{
   int32 connectionIndex = Portals.IndexOfByPredicate([&](const FTATPortalConnection& portalConnection)
   {
      return portalConnection.Portal == portal;
   });

   check(Portals.IsValidIndex(connectionIndex));

   check(Portals[connectionIndex].NumConnections > 0);
   Portals[connectionIndex].NumConnections--;

   if (Portals[connectionIndex].NumConnections == 0)
   {
      Portals.RemoveAtSwap(connectionIndex);
   }
}

bool UTATNoisePropagationSubsystem::_HandleAudioPropagation(
   const FVector& noiseLocation,
   const AActor* listenerActor,
   const FVector& listenerLocation,
   const float maxHearingRange,
   FVector& outPerceivedStimLocation) const
{
   // First, check if we have direct LoS to the noise stim, in which case we don't need to adjust its perceived location
   // This is potentially an extra trace, but it avoids the situation where a guard is right on the other side of an open door, but only
   // perceives the sound as happening at the door itself, rather than its actual location
   if (_DoesListenerHaveLOSToLocation(noiseLocation, listenerActor, listenerLocation, nullptr))
   {
      UE_LOG(LogTATNoisePropagationSubsystem, Verbose, TEXT("Direct LoS to noise stim, listener can hear it no matter what room(s) they're in"));
      return true;
   }
   UE_LOG(LogTATNoisePropagationSubsystem, Verbose, TEXT("Listener does have direct LoS to noise stim, checking rooms to see if it propagates"));

   const UAkRoomComponent* noiseRoom = _FindRoomForLocation(noiseLocation);
   const UAkRoomComponent* listenerRoom = _FindRoomForLocation(listenerLocation);

   UE_LOG(LogTATNoisePropagationSubsystem, Verbose,
      TEXT("Room for noise: '%s', Room for listener: '%s'"),
      *GetNameSafe(noiseRoom),
      *GetNameSafe(listenerRoom));

   if (noiseRoom != nullptr)
   {
      // The noise and listener are in the same room, so they can hear the stim
      if (noiseRoom == listenerRoom)
      {
         return true;
      }

      // Check if the stim can travel from within its room outside to the listener
      return _IsLocationInHearingRangeForRoom(
         noiseRoom,
         listenerLocation,
         noiseLocation,
         listenerActor,
         maxHearingRange,
         outPerceivedStimLocation
      );
   }
   
   // Check if the stim can travel from outside into the listener's room
   if (listenerRoom != nullptr)
   {
      return _IsLocationInHearingRangeForRoom(
         listenerRoom,
         noiseLocation,
         listenerLocation,
         listenerActor,
         maxHearingRange,
         outPerceivedStimLocation
      );
   }

   // If the noise is outside a room, then just rely on the distance check.
   return true;      
}
