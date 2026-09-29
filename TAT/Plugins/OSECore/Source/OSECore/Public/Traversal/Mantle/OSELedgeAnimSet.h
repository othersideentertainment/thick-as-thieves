// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// OSE
#include "Character/OSECharacterMovementTypes.h"
#include "OSELedgeAnimSet.generated.h"


/// Animation configuration for a specific Ledge animation
USTRUCT(BlueprintType, Category = Traversal, meta = (DisplayName = "Ledge Animation"))
struct OSECORE_API FOSELedgeAnim
{
   GENERATED_BODY()

public:

   bool IsValid() const { return ((Montage != nullptr) && (HeightOffset > 0)); }

   /// The offset from the environment anchor point that the anim starts at
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float HeightOffset = 0;

   /// The range of character offset that the 
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   FFloatInterval HeightRange = FFloatInterval(0.f, 0.f);

   /// should the animation auto eject from the ledge state
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   bool AutoEject = false;

   /// should the animation auto eject from the ledge state
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   FVector RootTargetOffset = FVector::ZeroVector;


   /// The root motion montage to play for this animation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   class UAnimMontage* Montage = nullptr;
};


/// Supported movement modes for an animation
UENUM(BlueprintType)
enum class EOSELedgeAnimMovement : uint8
{
   Any,              //< Any mode
   MovingOnGround,   //< Walking; NavWalking
   Falling,          //< Falling
   Scrambling,       //< Scrambling
   Ledge,            //< Ledge mode
   WallClimb,        //< Wall Climb mode
};


/// Ledge animation sets
UCLASS(Blueprintable, Category = Traversal, meta = (DisplayName = "Ledge Animation Set"))
class OSECORE_API UOSELedgeAnimSet : public UDataAsset
{
   GENERATED_BODY()

public:

   bool FindAnimation(FOSELedgeAnim& outAnim,
      EMovementMode inMovement, ECustomMovementType inCustomType,
      float inSpeed, float ledgeHeight, bool allowEject) const;

protected:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   EOSELedgeAnimMovement RequiredMode = EOSELedgeAnimMovement::Any;

   /// Horizontal speed must be >= the specified value for this animation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float RequiredSpeed = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   TArray<FOSELedgeAnim> Animations;
};
