// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// OSE
#include "Character/OSECharacterMovementTypes.h"
#include "OSEMantleAnimSet.generated.h"


/// Animation configuration for a specific mantle animation
USTRUCT(BlueprintType, Category = Traversal, meta = (DisplayName = "Mantle Animation"))
struct OSECORE_API FOSEMantleAnim
{
   GENERATED_BODY()

public:

   bool IsValid() const { return ((Montage != nullptr) && (Height > 0)); }

   /// Height must be >= the specified value for this animation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float Height = 0;

   /// The root motion montage to play for this animation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   class UAnimMontage* Montage = nullptr;
};


/// Supported movement modes for an animation
UENUM(BlueprintType)
enum class EOSEMantleAnimMovement : uint8
{
   Any,              //< Any mode
   MovingOnGround,   //< Walking; NavWalking
   Falling,          //< Falling
   Scrambling,         //< Scrambling
   WallClimb,        //< Wall Climb mode
   Crouching,        //< Crouching
};

struct FOSEMantleAnimSearchParams
{
   EMovementMode Movement = MOVE_None;
   ECustomMovementType CustomType = ECustomMovementType::None;
   bool IsCrouching;
   float Speed;
   float Height;
};


/// Mantle animation sets
UCLASS(Blueprintable, Category = Traversal, meta = (DisplayName = "Mantle Animation Set"))
class OSECORE_API UOSEMantleAnimSet : public UDataAsset
{
   GENERATED_BODY()

public:

   bool FindAnimation(FOSEMantleAnim& outAnim, const FOSEMantleAnimSearchParams& params) const;

protected:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   EOSEMantleAnimMovement RequiredMode = EOSEMantleAnimMovement::Any;

   /// Horizontal speed must be >= the specified value for this animation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float RequiredSpeed = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Traversal)
   TArray<FOSEMantleAnim> Animations;
};
