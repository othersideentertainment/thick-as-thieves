// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Interactables/TATInteractionGateInterface.h"

// ue
#include "Components/ActorComponent.h"

#include "TATPowerInteractRequirementComponent.generated.h"


class ATATPowerSource;

UENUM()
enum class ETATPowerInteractRequireMode : uint8
{
   None,
   RequirePowered,
   RequireUnpowered
};

// A component that gates interaction on having or not-having power
//
// * Only affects interactable that opt-in to interaction gates, like doors and toggles
//   (but this is most of them)
//
// * This requires a separate TATElectricalDevice component to be on the same actor
//    I went back and forth on this, since UTATElectricalDeviceComponent doesn't do much.
//    The main motivation, is if this component ends up getting BP subclasses, those can
//    be more easily swapped out without breaking instances references in maps. (since this
//    would probably not be added in C++, where that is easy)
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATPowerInteractRequirementComponent : public UActorComponent, public ITATInteractionGateInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATPowerInteractRequirementComponent();

   // ITATInteractionGateInterface
   virtual bool CanEverBlockInteraction() const override { return _mode != ETATPowerInteractRequireMode::None; }
   virtual bool IsInteractionBlocked() const override;
   virtual bool ShowMessageWhenBlocked() const override { return _showMessageWhenUnusable; }
   virtual void AddToPrompt(FInteractPrompt& prompt) const override;

protected:
   virtual void BeginPlay() override;

   UFUNCTION()
   void _OnAuthorityPoweredChanged(bool hasPower);

private:
   UPROPERTY(Transient)
   TObjectPtr<ATATPowerSource> _powerSource;

   UPROPERTY(EditDefaultsOnly, Category = Power)
   ETATPowerInteractRequireMode _mode = ETATPowerInteractRequireMode::RequireUnpowered;
   
   UPROPERTY(EditDefaultsOnly, Category = Power)
   bool _showMessageWhenUnusable = false;

   UPROPERTY(EditDefaultsOnly, Category = Power, meta=(EditCondition = "_showMessageWhenUnusable"))
   FText _unusableMessage;

   // Whether to close or turn off the actor once interaction is blocked
   UPROPERTY(EditDefaultsOnly, Category = Power)
   bool _closeWhenUnusable = false;
};
