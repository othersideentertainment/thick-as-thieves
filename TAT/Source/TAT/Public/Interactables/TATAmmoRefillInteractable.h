// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATAmmoRefillInteractable.generated.h"

UCLASS()
class TAT_API ATATAmmoRefillInteractable : public AActor, public IInteractableInterface
{
   GENERATED_BODY()
   
public:	
   // Sets default values for this actor's properties
   ATATAmmoRefillInteractable();

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

private:
   UPROPERTY(EditDefaultsOnly, Category = "Interactable")
   FText _interactPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "Interactable")
   float _interactHoldDuration;

   UPROPERTY(EditDefaultsOnly, Category = "Interactable", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag _interactAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "Interactable")
   FOSEHeldActionCues _holdCues;
};
