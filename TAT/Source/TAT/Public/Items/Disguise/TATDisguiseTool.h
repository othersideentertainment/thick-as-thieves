// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATDisguiseTool.generated.h"

class ACharacter;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct FDisguiseSnapshot
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   TSubclassOf<ACharacter> ActorClass = nullptr;

   UPROPERTY(BlueprintReadOnly)
   uint8 Team = 0;

   UPROPERTY(Transient)
   FGameplayTagContainer AllowedPrivateZones;
};

UCLASS()
class TAT_API UTATDisguiseFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "Tools|TAT|Disguise")
   static FDisguiseSnapshot CreateDisguiseSnapshotFromTargetActor(ACharacter* character);

   UFUNCTION(BlueprintCallable, Category = "Tools|TAT|Disguise")
   static bool DoesCharacterHaveActiveDisguise(ACharacter* character);
};
