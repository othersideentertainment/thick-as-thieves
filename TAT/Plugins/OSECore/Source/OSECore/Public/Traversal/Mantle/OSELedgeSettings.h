// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// UE4
#include "Math/Range.h"

// OSE
#include "OSELedgeSettings.generated.h"


/// Ledge configuration settings
USTRUCT(BlueprintType)
struct OSECORE_API FOSELedgeSettings
{
   GENERATED_BODY()

public:

   /// Maximum angle in degrees for mantling surfaces facing the player
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 1, UIMin = 1, ClampMax = 45, UIMax = 45))
   float MaxAngleFacing = 40.0f;

   /// Time in the future for predictive velocity when entering a ledge
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float LookAheadTime = 0.1f;

   /// Min and max ledge heights to test when entering a ledge from another state
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 400, UIMax = 400))
   FFloatInterval EntranceHeight = FFloatInterval (50.0f, 300.0f);

   /// Min and max ledge heights to test when climbing from ledge to ledge
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 400, UIMax = 400))
   FFloatInterval ClimbHeight = FFloatInterval(25.0f, 150.0f);

   /// Min and max horizontal offset during ledge climb
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = -400, UIMin = -400, ClampMax = 400, UIMax = 400))
   FFloatInterval ClimbOffset = FFloatInterval(-25.0f, 75.0f);

   /// Minimum distance to attempt a shimmy
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1000, UIMax = 1000))
   FFloatInterval ShimmyDistance = FFloatInterval(60.0f, 100.0f);

   /// Minimum distance to attempt a shimmy
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
   float ShimmyEase = 15.0f;
};
