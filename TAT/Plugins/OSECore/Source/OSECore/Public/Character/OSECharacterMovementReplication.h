// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// OSE
#include "Traversal/OSEDistanceConstraint.h"

// UE4
#include "GameFramework/CharacterMovementReplication.h"


/// FCharacterNetworkMoveData encapsulates a client move that is sent to the server for UCharacterMovementComponent networking.
///
/// Adding custom data to the network move is accomplished by deriving from this struct, adding new data members, implementing ClientFillNetworkMoveData(), implementing Serialize(),
/// and setting up the UCharacterMovementComponent to use an instance of a custom FCharacterNetworkMoveDataContainer (see that struct for more details).
///
/// @see FCharacterNetworkMoveDataContainer
struct OSECORE_API FOSECharacterNetworkMoveData : public FCharacterNetworkMoveData
{
public:
   using Super = FCharacterNetworkMoveData;

   FOSECharacterNetworkMoveData()
   {
   }

   /// Given a FSavedMove_Character from UCharacterMovementComponent, fill in data in this struct with relevant movement data.
   /// Note that the instance of the FSavedMove_Character is likely a custom struct of a derived struct of your own, if you have added your own saved move data.
   /// @see UCharacterMovementComponent::AllocateNewMove()
   virtual void ClientFillNetworkMoveData(const FSavedMove_Character& clientMove, ENetworkMoveType moveType) override;

   /// Serialize the data in this struct to or from the given FArchive. This packs or unpacks the data in to a variable-sized data stream that is sent over the
   /// network from client to server.
   /// @see UCharacterMovementComponent::CallServerMovePacked
   virtual bool Serialize(UCharacterMovementComponent& characterMovement, FArchive& ar, UPackageMap* packageMap, ENetworkMoveType moveType) override;

public:
   FOSEDistanceConstraint DistanceConstraint;
   EDistanceConstraintDelta DistanceConstraintDelta;

   float MaxSpeedMultiplier;
   float ExtraGravityScale;

   uint8 bExtendingDistanceConstraint : 1;
   uint8 bWantsToContractDistanceConstraint : 1;

   uint8 bWantsToWallClimb : 1;
};

struct OSECORE_API FOSECharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:

   FOSECharacterNetworkMoveDataContainer()
   {
      NewMoveData = &_defaultMoveData[0];
      PendingMoveData = &_defaultMoveData[1];
      OldMoveData = &_defaultMoveData[2];
   }

private:
   FOSECharacterNetworkMoveData _defaultMoveData[3];
};

 /// Response from the server to the client about a move that is being acknowledged.
 /// Internally it mainly copies the FClientAdjustment from the UCharacterMovementComponent indicating the response, as well as
 /// setting a few relevant flags about the response and serializing the response to and from an FArchive for handling the variable-size
 /// payload over the network.
struct OSECORE_API FOSECharacterMoveResponseDataContainer : FCharacterMoveResponseDataContainer
{
public:
   using Super = FCharacterMoveResponseDataContainer;

   FOSECharacterMoveResponseDataContainer()
      : bDistanceConstraintCorrection(false)
      , DistanceConstraintPosition(ForceInitToZero)
      , DistanceConstraintDistance(0)
   {
   }


   virtual void ServerFillResponseData(const UCharacterMovementComponent& characterMovement, const FClientAdjustment& pendingAdjustment) override;
   virtual bool Serialize(UCharacterMovementComponent& characterMovement, FArchive& ar, UPackageMap* packageMap) override;

   bool bDistanceConstraintCorrection;
   FVector DistanceConstraintPosition; // is it worth using a net-quantized version of this?
   float DistanceConstraintDistance;
};
