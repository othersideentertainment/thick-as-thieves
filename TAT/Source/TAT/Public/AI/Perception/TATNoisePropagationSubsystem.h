// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// wwise
#include "AkEnvironmentIndex.h"

// ue5
#include "Subsystems/WorldSubsystem.h"

#include "TATNoisePropagationSubsystem.generated.h"

struct FTATHearingEventStimSettings;

class UAkPortalComponent;
class UAkRoomComponent;

/// Used to track what portals are connected to a given room, with the number of connections tracked
/// In theory, a portal could have the same front and back room, which would mean we'd have NumConnections = 2,
/// and would need to consider them separately to know if that portal was later still connected to the room
USTRUCT()
struct TAT_API FTATPortalConnection
{
   GENERATED_BODY()
public:
   UPROPERTY()
   UAkPortalComponent* Portal = nullptr;

   UPROPERTY()
   int32 NumConnections = 0;
};

USTRUCT()
struct TAT_API FTATPortalsForRoom
{
   GENERATED_BODY()
public:
   UPROPERTY()
   TArray<FTATPortalConnection> Portals;

   void AddPortal(UAkPortalComponent* portal);
   void RemovePortal(UAkPortalComponent* portal);
};

/// Tracks the inverse: which rooms a given portal is connected to
/// WWise will handle this on most builds, but not on dedicated servers:
/// in that case, we need to determine the front/back rooms ourselves, so we use this
/// to track the current state
USTRUCT()
struct TAT_API FTATRoomsForPortal
{
   GENERATED_BODY()
public:
   UPROPERTY()
   const UAkRoomComponent* FrontRoom = nullptr;

   UPROPERTY()
   const UAkRoomComponent* BackRoom = nullptr;
};

/// Used to determine if a given stim should be heard by a given listener/AI
UCLASS()
class TAT_API UTATNoisePropagationSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()
public:
   // from UTickableWorldSubsystem
   virtual ETickableTickType GetTickableTickType() const override;
   virtual TStatId GetStatId() const override;
   virtual bool IsTickableInEditor() const override { return false; }
#if ENABLE_DRAW_DEBUG
   // Only tick in builds with debug drawing, since that's all it does
   virtual void Tick(float deltaTime) override;
#endif

   bool IsNoiseEventHeardByListener(
      const FVector& noiseLocation,
      const AActor* listenerActor,
      const FVector& listenerLocation,
      const FTATHearingEventStimSettings& stimSettings,
      float maxHearingRange,
      FVector& outPerceivedStimLocation) const;

   void RegisterPortalComponent(UAkPortalComponent* portal);
   void UnregisterPortalComponent(UAkPortalComponent* portal);

   void RegisterRoomComponent(UAkRoomComponent* room);
   void UnregisterRoomComponent(UAkRoomComponent* room);

   // Called when a portal has potentailly updated its room connections, so we should recompute them on our end
   void OnPortalComponentRoomConnectionsUpdated(UAkPortalComponent* portal);

private:
   void _AddRoomPortalConnection(UAkPortalComponent* portal, const UAkRoomComponent* room);
   void _RemoveRoomPortalConnection(UAkPortalComponent* portal, const UAkRoomComponent* room);

#if DO_CHECK
   static void _EnsurePortalRoomsMatchWWise(const UAkPortalComponent* portal, const UAkRoomComponent* expectedRoom, const UAkRoomComponent* actualRoom, const FVector& portalLocation);
#endif

   FTATRoomsForPortal _ComputeCurrentRoomsForPortal(const UAkPortalComponent* portal) const;
   FTATRoomsForPortal _GetRoomsForPortal(const UAkPortalComponent* portal) const;
   UAkPortalComponent* _FindClosestOpenPortalForRoom(const UAkRoomComponent* room, const FVector& listenerPos) const;
   UAkRoomComponent* _FindRoomForLocation(const FVector& location) const;
   static void _GetPortalFrontAndBackLocations(const UAkPortalComponent* portal, FVector& frontLocation, FVector& backLocation);

   bool _IsLocationInHearingRangeForRoom(
      const UAkRoomComponent* room,
      const FVector& locationOutsideRoom,
      const FVector& locationInsideRoom,
      const AActor* listenerActor,
      float maxHearingRange,
      FVector& outPortalLocation) const;
   bool _HandleAudioPropagation(const FVector& noiseLocation, const AActor* listenerActor, const FVector& listenerLocation, const float maxHearingRange, FVector
                                & outPerceivedStimLocation) const;
   bool _DoesListenerHaveLOSToLocation(const FVector& location, const AActor* listenerActor, const FVector& listenerLocation, const AActor*
                                       optionalPortalOwner) const;

   UPROPERTY(Transient)
   TMap<const UAkRoomComponent*, FTATPortalsForRoom> _roomToPortals;

   UPROPERTY(Transient)
   TMap<const UAkPortalComponent*, FTATRoomsForPortal> _portalToRooms;

   // NB: Despite the U prefix, this is not a UObject-based type, but instead based on TOctree
   UAkEnvironmentOctree _roomOctree;
};
