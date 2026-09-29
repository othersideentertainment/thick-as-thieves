// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Items/ToolComponent.h"

#include "TATToolSelectionInteractable.generated.h"

/// An interactable that allows the player to pick up or remove tools from their toolset
/// It is intended for placing in player's safe rooms, to allow loadout customisation pending an actual pre-session UI
UCLASS()
class TAT_API ATATToolSelectionInteractable : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   ATATToolSelectionInteractable();

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;

   UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "TAT|Tool Select")
   TSubclassOf<UToolComponent> ToolClass;

   /// Do not let the player pick up another tool if they have this many tools or more
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   int32 MaxToolCount = 4;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   float InteractHoldDuration = 1.0f;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   FGameplayTag AddToolAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   FGameplayTag RemoveToolAnimationTag;

   /// Prompt to show the player when they will add the tool
   /// Use "{ToolName}" as part of the text and it will be formatted to the tool name
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   FText AddToolPrompt;

   /// Prompt to show the player when they will remove the tool
   /// Use "{ToolName}" as part of the text and it will be formatted to the tool name
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   FText RemoveToolPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool Select")
   FText NoMoreRoomPrompt;

private:
   const FText& _GetCachedFormattedRemoveToolPrompt();
   const FText& _GetCachedFormattedAddToolPrompt();
   void _MaybeCacheFormattedPrompts();

   /// Cache the formatted prompt text so we don't need to recreate it each time in GetInteractPrompt
   UPROPERTY(Transient)
   FText _cachedFormattedRemoveToolPrompt;

   UPROPERTY(Transient)
   FText _cachedFormattedAddToolPrompt;

   bool _cachedPromptsSet = false;

   bool _PlayerHasTool(ACharacter* character) const;
   bool _PlayerHasRoomForMoreTools(ACharacter* character) const;
};
