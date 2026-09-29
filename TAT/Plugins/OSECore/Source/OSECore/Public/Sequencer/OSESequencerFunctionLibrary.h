// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSESequencerFunctionLibrary.generated.h"

class USkeletalMeshComponent;

UCLASS()
class OSECORE_API UOSESequencerFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category="Sequencer|OSE", meta = (WorldContext = "contextObj"))
   static bool SetSkeletalMeshForPlayer(const UObject* contextObj, USkeletalMeshComponent* skeletalMeshToSet, int playerIdx, bool hideOnFailure);
};
