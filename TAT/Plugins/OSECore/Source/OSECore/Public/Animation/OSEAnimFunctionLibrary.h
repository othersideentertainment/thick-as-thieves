// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEAnimFunctionLibrary.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

UCLASS(Const, meta = (BlueprintThreadSafe))
class OSECORE_API UOSEAnimFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "OSE|Animation")
   static bool GetMontageSectionStartAndEndTime(const UAnimMontage* montage, FName sectionName, float& outStartTime, float& outEndTime);

   // Useful in cases where a resulting socket location must be queried immediately after a pose change due to animation switch.
   // Use with care.
   UFUNCTION(BlueprintCallable, Category = "OSE|Animation")
   static void ForcePoseUpdate(USkeletalMeshComponent* skeletalMesh);
};
