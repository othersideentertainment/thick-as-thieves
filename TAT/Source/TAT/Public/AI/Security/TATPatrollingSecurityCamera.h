// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Interactables/TATVisionPerceptionDevice.h"
#include "TATPatrollingSecurityCamera.generated.h"

class UTATFlyingPatrolFollowingMovementComponent;
UCLASS(BlueprintType, Blueprintable)
class TAT_API ATATPatrollingSecurityCamera : public ATATVisionPerceptionDevice
                                 , public IInteractableInterface
{
public:
   GENERATED_BODY()
   ATATPatrollingSecurityCamera();

   virtual void BeginPlay() override;
protected:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TAT|Movement")
   float DelayBeforeMovingAfterDetectingPlayer {0.f};
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="TAT|Movement")
   UTATFlyingPatrolFollowingMovementComponent* _movementComponent { nullptr };

   virtual void SetOn(bool newOn) override;
   virtual void _OnDeviceStateChanged_Implementation(ETATVisionPerceptionDeviceState deviceState, ETATVisionPerceptionDeviceState oldDeviceState) override;

   // Begin IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* InteractingCharacter) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   // End IInteractableInterface
   
   virtual void PostNetReceiveLocationAndRotation() override;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction|Animation")
   FGameplayTag TurnOnAnimationTag;
   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction|Animation")
   FGameplayTag TurnOffAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction")
   FText TurnOnPrompt;
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction")
   FText TurnOffPrompt;
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Visuals")
   USceneComponent* _interpolatedVisualsComponent;
   
   UPROPERTY(EditDefaultsOnly)
   USphereComponent* _colliderComponent;
   
private:
   UFUNCTION()
   void _OnMovementResetTimerExpired() const;
   
   FTimerHandle _movementResetHandle;
};
