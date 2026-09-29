// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueFact.h"
#include "UI/TATScreenWidget.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATReadableClueActor.generated.h"

class UTATUIQueueAction;
class UAkAudioEvent;

UCLASS(Abstract, meta = (DisableNativeTick))
class UTATReadableClueWidget : public UTATScreenWidget
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void SetClueText(const FText& noteText);

   static UTATUIQueueAction* CreateAction(const TSoftClassPtr<UTATReadableClueWidget>& widgetClass, const FText& noteText);
};

// replicated payload
// separate struct for the heck of it
USTRUCT()
struct FTATReadableClueData
{
   GENERATED_BODY()

   UPROPERTY()
   FText ClueText;

   FTATSharedClueFactThunk Facts;

   // replicated fact tags, so clients can use them
   UPROPERTY()
   FGameplayTagContainer FactTags;

   // replicated fact namespace for above tags
   UPROPERTY()
   FTATClueFactNamespace FactNamespace;
};

// An interactable actor for a readable clue
//
// NOTE: For now, the clue data is replicated up-front, but the cost/leakage of this is mitigated by reducing the
//       relevancy range
UCLASS(Abstract)
class TAT_API ATATReadableClueActor : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   ATATReadableClueActor();

   void InitClueData(const FTATReadableClueData& clueData);

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endplayReason) override;
   virtual void BeginReplication() override;
   virtual bool IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const override;

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnFactsKnownByLocalPlayer();

   UFUNCTION(BlueprintPure)
   bool AreFactsKnownByLocalPlayer() const { return _allFactsKnownByLocalPlayer; }

private:
   FGameplayTag _AuthorityGetSourceTag() const;
   void _UpdateFactsKnown();
   
   // The UI widget class used to display the clue
   // TODO: should this be a soft ref? I don't think textures in UMG are streamed
   //       Or have the visual info be a separate load?
   UPROPERTY(EditDefaultsOnly, Category=ReadableClue)
   TSoftClassPtr<UTATReadableClueWidget> _readableWidget;

   // Interact prompt to read the clue
   UPROPERTY(EditDefaultsOnly, Category=ReadableClue)
   FText _readCluePrompt;

   // the sound played for the local player when reading the clue
   UPROPERTY(EditDefaultsOnly, Category=ReadableClue)
   TObjectPtr<UAkAudioEvent> _localInteractSound;

   // the sound played for the local player when reading clue if already read
   UPROPERTY(EditDefaultsOnly, Category=ReadableClue)
   TObjectPtr<UAkAudioEvent> _localAlreadyReadInteractSound;

   UPROPERTY(Transient, Replicated)
   FTATReadableClueData _clueData;

   // Just for local-only cosmetic stuff
   bool _readByLocalPlayer = false;
   bool _allFactsKnownByLocalPlayer = false;
};
