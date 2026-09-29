// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATCompleteContractInteractable.generated.h"

enum class ETATContractOutroFlow : uint8;

UENUM()
enum class ETATContractInteractableCompletionTiming : uint8
{
   // The contract completes immediately on interaction
   Immediate,
   // The contract completes between the narrative text and the completion popup
   Middle,
   // The contract completes after the completion popup closes
   End
};

// A simple interactable to complete a pending quest that waiting for its outro
UCLASS()
class TAT_API ATATCompleteContractInteractable : public AActor, public IInteractableInterface
{
   GENERATED_BODY()
   
public:	
   // Sets default values for this actor's properties
   ATATCompleteContractInteractable();

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

protected:
   UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnCurrentContractChanged"))
   void BP_OnCurrentContractChanged(FGameplayTag oldContract, FGameplayTag newContract);
   
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnContractOutroStarted"))
   void BP_OnContractOutroStarted(FGameplayTag contractTag);
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnContractCompleted"))
   void BP_OnContractCompleted();

private:
   void _RefreshContract();
   void _CompleteContract();

private:
   UPROPERTY(EditDefaultsOnly, Category = "Interactable")
   FText _interactPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "Interactable")
   ETATContractInteractableCompletionTiming _completionTiming = ETATContractInteractableCompletionTiming::End;

   UPROPERTY(Transient)
   FGameplayTag _currentContract;
   
   // might change to be a tag or something later
   UPROPERTY(EditDefaultsOnly, Category = "Quests")
   ETATContractOutroFlow _flow;
};
