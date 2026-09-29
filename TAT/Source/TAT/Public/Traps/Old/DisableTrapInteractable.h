// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue4
#include "GameFramework/Actor.h"

#include "DisableTrapInteractable.generated.h"

class ATrapTriggerBase_Old;

UCLASS()
class TAT_API ADisableTrapInteractable_Old : public AActor, public IInteractableInterface
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ADisableTrapInteractable_Old();

   // InteractableInterface start
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   // InteractableInterface end

protected:
   // referring to class rather than interface for 2 reasons (but there could be reasons to change it):
   // 1) It seemed unlikely that these would disable a trap with a different base class (like a trapped lock)
   // 2) This may need to have its visual state reflect state changes of the trap itself
   UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Trap)
   ATrapTriggerBase_Old* _trapToDisable;

   UPROPERTY(EditAnywhere, Category = Trap)
   float _trapDisableDuration;

private:
   UPROPERTY(EditDefaultsOnly, Category = Trap, AdvancedDisplay)
   FText _promptText;
};
