// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TATTrapDetectorComponent.generated.h"


UCLASS(ClassGroup=(Custom), HideCategories=(Tags,Activation,Cooking,AssetUserData,Navigation), meta=(BlueprintSpawnableComponent))
class TAT_API UTATTrapDetectorComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/TAT.TATTrapActionInterface"), Category="Trap")
   TArray<TObjectPtr<AActor>> TrapActionsToTrigger;

   UFUNCTION(BlueprintCallable)
   void TriggerTrapActions(AActor* optionalTarget);

   UFUNCTION(BlueprintCallable)
   void TEMP_ResetActions() { TrapActionsToTrigger.Reset(); }

   UFUNCTION(BlueprintCallable)
   void TEMP_MigrateFrom(UTATTrapDetectorComponent* other);

   UFUNCTION(BlueprintCallable)
   void TEMP_MigrateFromLockdown(class UTATSecurityLockdownComponent* lockdown);
};
