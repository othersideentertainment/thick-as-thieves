// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// UE4
#include "Math/Range.h"

// OSE
#include "OSEMantleSettings.generated.h"


/// Animation configuration for a specific mantle animation
USTRUCT(BlueprintType)
struct OSECORE_API FOSEMantleAnimConfig
{
   GENERATED_BODY()

public:

   /// The root motion montage to play for this animation
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   class UAnimMontage* Montage = nullptr;

   /// Height must be >= the specified value for this animation
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 400, UIMax = 400))
   int32 Height = 0;

   /// Horizontal speed must be >= the specified value for this animation
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float Speed = 0;
};


/// Mantle configuration settings
USTRUCT(BlueprintType)
struct OSECORE_API FOSEMantleSettings
{
   GENERATED_BODY()

public:

   /// Maximum angle in degrees for mantling surfaces facing the player
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 1, UIMin = 1, ClampMax = 45, UIMax = 45))
   float MaxAngleFacing = 40.0f;

   /// Minimum mantle height
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 400, UIMax = 400))
   float MinHeight = 50.0f;

   /// Maximum mantle height
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 400, UIMax = 400))
   float MaxHeight = 300.0f;

   /// Time in the future for predictive velocity
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float LookAheadTime = 0.1f;

};
