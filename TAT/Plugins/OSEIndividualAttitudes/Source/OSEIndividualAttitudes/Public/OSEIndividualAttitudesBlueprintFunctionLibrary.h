// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "IndividualAttitudeTypes.h"

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// self
#include "OSEIndividualAttitudesBlueprintFunctionLibrary.generated.h"

UCLASS()
class OSEINDIVIDUALATTITUDES_API UOSEIndividualAttitudesBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   static UOSEIndividualAttitudeComponent* GetIndividualAttitudeComponent(const AActor* actor);
   
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Individual Attitudes")
   static void SetIndividualAttitude(AActor* sourceActor,
                                     AActor* targetActor,
                                     EOSEIndividualAttitude attitude,
                                     bool shouldExpire = false,
                                     float maxAge = -1.f);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Individual Attitudes")
   static void ClearIndividualAttitude(const AActor* sourceActor,
                                       AActor* targetActor);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Individual Attitudes")
   static void ShareAllIndividualAttitudesWithTarget(
      AActor* sourceActor,
      AActor* targetActor,
      EOSEAttitudeCopyRules attitudeCopyRules = EOSEAttitudeCopyRules::Overwrite,
      EOSEExpirationTimeCopyRules expirationTimeCopyRules = EOSEExpirationTimeCopyRules::Overwrite
      );
};
