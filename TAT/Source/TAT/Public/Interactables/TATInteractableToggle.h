// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Breakables/TATBreakableActorFwd.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "Breakables/TATBreakableActorInfoInterface.h"
#include "Environment/TATInhibitableInterface.h"

// ose
#include "TATInteractionGateInterface.h"

#include "OSEVoiceLineTraitInterface.h"
#include "Interactables/OSEInteractableToggle.h"

#include "TATInteractableToggle.generated.h"

UCLASS(HideCategories=(Rendering, Input))
class TAT_API ATATInteractableToggle : public AOSEInteractableToggle
   , public IAbilitySystemInterface
   , public IGameplayTagAssetInterface // < Breakables
   , public ITATUtilityAITargetingGroupInterface
   , public IOSEVoiceLineTraitInterface
   , public ITATBreakableActorInfoInterface
   , public ITATInhibitableInterface
{
   GENERATED_BODY()
public:
   ATATInteractableToggle();

   // from AActor
   virtual void PostInitializeComponents() override;


   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;

   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
   
   // from IOSEVoiceLineTraitInterface
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const override;

   // from ITATInhibitableInterface
   virtual bool CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const override;
   virtual FGameplayTag GetInhibitableType_Implementation() const override { return InhibitableType; }
   virtual FTATInhibitorPlacementInfo GetInhibitorPlacementInfo_Implementation() const override;
   virtual void OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount) override;
   virtual void OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved) override;

   /// Allow inhibitors to work on this interactable toggle
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable")
   bool IsInhibitable = true;

   /// The type/category of this object in terms of things that can be inhibited in the world.
   /// Allows things that apply inhibitors to decide what they can inhibit (eg. a tool that can inhibit this actor only if it has the type "Inhibitable.Light")
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable", Meta = (EditCondition = "IsInhibitable", Categories = "Inhibitable"))
   FGameplayTag InhibitableType;

   BREAKABLE_ACTOR_DECLARATIONS()

private:
   UPROPERTY(EditDefaultsOnly, Category="AI|TAT", meta=(Categories="AI.Object.Broken"))
   FGameplayTag _tagForVoiceLinesWhenBroken;

protected:
   virtual bool _TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt);
   virtual void _AddNormalInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt);

   virtual bool _TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult);
   virtual FInteractStartResult _NormalStartInteract(ACharacter* interactingCharacter);
   
   // Breakable boilerplate
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAbilitySystemComponent> _abilitySystemComponent = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TObjectPtr<UTATBreakableComponent> _breakableComponent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction")
   bool _interactableWhenBroken = false;

   UPROPERTY(Transient)
   FTATInteractionGateCollection _interactionGates;
};
