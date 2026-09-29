// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Traversal/OSEDistanceConstraint.h"
#include "Traversal/Mantle/OSEMantleLocation.h"

#include "TraversalInterface.generated.h"


//--------------------------------------------------------------------------------------------------
/// Broad ground speed categories calculated from movement mode and the various speed properties.
//--------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EOSEGroundSpeed : uint8
{
   Invalid,  ///< Not moving on the ground
   Stopped,  ///< Not moving at all, nearly zero speed
   Slow,     ///< Less than "walk", which happens when the player starts walking, or is crouched
   Walk,     ///< At walk speed
   Sprint,   ///< At sprint speed
   Full      ///< At sprint speed + forward sprint speed boost
};


//--------------------------------------------------------------------------------------------------
/// Combined state that can be queried in a single call, populated via the traversal interface.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSETraversalState
{
   GENERATED_BODY()

public:

   FOSETraversalState()
      : GroundSpeed(EOSEGroundSpeed::Invalid)
      , IsCrouching(false)
      , IsSliding(false)
      , IsScrambling(false)
      , IsSprinting(false)
      , IsMantling(false)
      , IsOnLedge(false)
      , IsClimbing(false)
   { }

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EOSEGroundSpeed GroundSpeed;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsCrouching : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsSliding : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsScrambling : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsSprinting : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsMantling : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsOnLedge : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsClimbing : 1;
};


//---------------------------------------------------------------------------------------------------
/// Traversal interface
//---------------------------------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Category = Traversal, meta = (CannotImplementInterfaceInBlueprint))
class UTraversalInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API ITraversalInterface
{
   GENERATED_BODY()

public:

   /// Returns true if we are currently crouching. For convenience and consistency; implementations
   /// are expected to pass through to the movement component IsCrouching() method.
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsCrouching() const = 0;

public:

   /// Returns true if this character can slide
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanSlide() const = 0;

   /// Returns true if we are currently sliding
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsSliding() const = 0;

public:

   /// Returns true if this character can climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanScramble() const = 0;

   /// Returns true if we are currently climbing
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsScrambling() const = 0;

   /// Indicates that we want to BE ABLE TO climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StartScrambling() = 0;

   /// Indicates that we no longer want to BE ABLE TO climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StopScrambling() = 0;

public:

   /// Returns true if this character can sprint
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanSprint() const = 0;

   /// Returns true if we are currently sprinting
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsSprinting() const = 0;

   /// Requests sprinting
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void SprintRequest() = 0;

   /// Cancels sprinting request
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void SprintCancel() = 0;

public:

   /// Returns true if this character can crouch
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanCrouch() const = 0;

   /// Requests crouching
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void CrouchRequest() = 0;

   /// Cancels crouching request
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void CrouchCancel() = 0;


public:

   /// Returns true if this character can mantle
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanMantle() const = 0;

   /// Returns true if we are currently mantling
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsMantling() const = 0;

   /// Indicates that we WANT to start mantling; not guaranteed to actually mantle
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StartMantleAttempt() = 0;

   /// Indicates that we no longer WANT to mantle; may not interrupt an in-progress mantle
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StopMantleAttempt() = 0;


public:

   /// Returns true if this character can mantle
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanUseLedges() const = 0;

   /// Returns true if we are currently mantling
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsOnLedge() const = 0;

   /// Indicates that we want to stop hanging on a ledge
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StartReleaseLedge() = 0;

   /// Indicates that we want to stop hanging on a ledge
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StopReleaseLedge() = 0;

public:
   /// Returns true if this character can wall climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool CanWallClimb() const = 0;

   /// Returns true if we are currently wall climbing
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual bool IsWallClimbing() const = 0;

   /// Indicates that we want to BE ABLE TO climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StartWallClimbing() = 0;

   /// Indicates that we no longer want to BE ABLE TO climb
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual void StopWallClimbing() = 0;

public:

   /// Broad ground speed categories calculated from current movement mode + the various speed properties
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual EOSEGroundSpeed GetGroundSpeedThreshold(float speedValue) const = 0;

   /// Broad ground speed categories calculated from current movement mode + the various speed properties
   UFUNCTION(BlueprintCallable, Category = Traversal)
   virtual EOSEGroundSpeed GetGroundSpeed() const = 0;

public:

   /// Native-only method to return the current combined traversal state
   FORCEINLINE FOSETraversalState GetTraversalState() const
   {
      FOSETraversalState state;
      state.GroundSpeed = GetGroundSpeed();
      state.IsCrouching = IsCrouching();
      state.IsSliding   = IsSliding();
      state.IsScrambling  = IsScrambling();
      state.IsSprinting = IsSprinting();
      state.IsMantling  = IsMantling();
      state.IsOnLedge = IsOnLedge();
      state.IsClimbing = IsWallClimbing();
      return state;
   }
};
