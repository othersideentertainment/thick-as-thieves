// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OSEAITargetSightInterface.generated.h"


UENUM(BlueprintType)
enum EAISightBucket : uint8
{
   High,
   Medium,
   Low,
   NonQualityControlledBucket UMETA(Hidden), // Used for the "old" queries to keep the code cleaner instead of using duplicate properties, this will be deprecated if needed later.
   Count UMETA(Hidden) // DO NOT USE
};


UINTERFACE(BlueprintType)
class UOSEAITargetSightInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEAITargetSightInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent)
   EAISightBucket GetBucketForTarget() const;
   virtual EAISightBucket GetBucketForTarget_Implementation() const { return EAISightBucket::Low; }
};
