// (c) 2202 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ue4
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateCombinations.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"

#include "OSEFootstepSimulatorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimulatedFootstepDelegate, const FGameplayTag&, StrideTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimNotifyFootstepDelegate, const FGameplayTag&, StrideTag);


USTRUCT(BlueprintType)
struct FFootstepSimulatorStride
{
   GENERATED_BODY()

   UPROPERTY(Transient)
   float TravelDistance = 0;

   UPROPERTY(Transient)
   float LastStep = 0;

   UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (Categories = "SimulatedFootstep"))
   FGameplayTag StrideTag;

   // Whether this stride should emit an event when the character stops moving and their feet "close"
   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   bool StepOnClose = true;

   // How far a character must move to result in a footstep event.
   UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = "0.01"))
   float StrideLength = 50;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OSECORE_API UOSEFootstepSimulatorComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UOSEFootstepSimulatorComponent();

   virtual void BeginPlay();

   void ReceiveAnimationFootstep(const FGameplayTag& strideTag);

   /// Emit footstep events even if the X and Y distance is very small
   UPROPERTY(EditAnywhere)
   bool EmitFootstepsForVerticalOnlyMovement = false;

   UPROPERTY(EditAnywhere)
   TArray<FFootstepSimulatorStride> Strides;

   UFUNCTION(BlueprintCallable, meta = (Categories = "SimulatedFootstep"))
   void SetStrideLength(const FGameplayTag& strideTag, float length);

   UPROPERTY(BlueprintAssignable)
   FOnSimulatedFootstepDelegate OnStep;
   
   UPROPERTY(BlueprintAssignable)
   FOnAnimNotifyFootstepDelegate OnAnimNotifyFootstep;

protected:
   UFUNCTION()
   void _OnMovementUpdated(float deltaSeconds, FVector oldLocation, FVector oldVelocity);

private:
   UPROPERTY(Transient)
   class UCharacterMovementComponent* _characterMovement = nullptr;

   UPROPERTY(EditDefaultsOnly)
   bool _ShouldIgnoreAnimNotifies { false };

   void _StoppedMoving();
};
