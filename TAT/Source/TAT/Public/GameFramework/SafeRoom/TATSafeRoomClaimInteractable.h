// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoomConnectedActor.h"
#include "Interactables/InteractableInterface.h"

#include "TATSafeRoomClaimInteractable.generated.h"

enum class ETATSafeRoomOwnerType : uint8;

// An interactable actor that, when used, can let players claim an unclaimed safe room
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS()
class TAT_API ATATSafeRoomClaimInteractable : public ATATSafeRoomConnectedActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   // from AActor
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType);

   UPROPERTY(EditDefaultsOnly, Category = "Safe Room Claiming")
   float HoldDuration = 3.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Safe Room Claiming")
   FText HoldPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "Safe Room Claiming")
   FText CannotClaimError;

   UPROPERTY(EditDefaultsOnly, Category = "Safe Room Claiming")
   FGameplayTag HoldAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "Safe Room Claiming")
   FOSEHeldActionCues HoldActionCues;

   /// IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;

private:
   UFUNCTION()
   void _OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType);
};
