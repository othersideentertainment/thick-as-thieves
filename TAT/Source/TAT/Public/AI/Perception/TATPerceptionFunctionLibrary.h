// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TATPerceptionFunctionLibrary.generated.h"

struct FTATHearingEventStimSettings;
struct FGameplayTag;

UCLASS()
class TAT_API UTATPerceptionFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static bool GetStimInfoByGameplayTag(const UDataTable* dataTable,
                                        const FGameplayTag& tag,
                                        FTATHearingEventStimSettings& stimSettings,
                                        FName& stimName);

   static bool CanActorBoundingBoxBeSeenFromLocation(const AActor* ownerActor,
                                                     const FBox& bounds,
                                                     const FVector& observerLocation,
                                                     FVector& outSeenLocation,
                                                     int32& numberOfLoSChecksPerformed,
                                                     float& outSightStrength,
                                                     const AActor* ignoreActor);
};
