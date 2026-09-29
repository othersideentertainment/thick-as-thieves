// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Compass/TATGenericIndicator.h"
#include "Variation/Clues/TATClueFact.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "TATQuestActorSpawnAction.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATQuestSpawnIndicator.generated.h"

class UTATMapVariationMgrComponent;
class UTATClueHighlightComponent;
class UTATMapActorComponent;
class ATATQuestActorSpawner;



// A map/compass indicator that can be enabled when a player gets a clue
// for a spawn location, and hidden when the loot is taken
UCLASS(Abstract)
class TAT_API ATATQuestSpawnIndicator : public ATATGenericIndicator
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATQuestSpawnIndicator();

#if WITH_EDITOR
   void CheckForErrors() override;
#endif

   void AuthorityInitFactTag(const FGameplayTag& factTag) { _clueFactTag = factTag; }
   void AuthorityInit(AActor* spawnedActor, UTATQuestActorSpawnerComponent* spawner);

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   void _AuthorityInitNamespace(UTATMapVariationMgrComponent* variationMgr);
   void _OnFactKnown();
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnFactKnown();
   
   UFUNCTION()
   void _OnSpawnedActorDestroyed(AActor* destroyedActor);

   UFUNCTION()
   void _OnRep_FactNamespace();
protected:
   
   // Target Spawner
   UPROPERTY(EditInstanceOnly, Category = "SpawnIndicator")
   TObjectPtr<ATATQuestActorSpawner> _questSpawner;

   // Which fact triggers the highlight when learned
   // Would use ClueHighlightComponent, but may need to start delayed
   UPROPERTY(Replicated, EditAnywhere, Category=SpawnIndicator, meta=(Categories="ClueFact"))
   FGameplayTag _clueFactTag;

   // Set to true if it might be something like a set collection
   UPROPERTY(EditAnywhere, Category=SpawnIndicator)
   bool _locationWillRandomize = false;

   // The scene requirement that must be met before this is used
   UPROPERTY(EditAnywhere, Category=SpawnIndicator)
   FTATSceneRequirement _sceneRequirement;
   
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   TObjectPtr<UTATMapActorComponent> _mapComponent;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_FactNamespace)
   FTATClueFactNamespace _factNamespace;
};

USTRUCT(DisplayName="Indicator")
struct TAT_API FTATQuestActorSpawnAction_Indicator : public FTATQuestActorSpawnAction
{
   GENERATED_BODY()

   virtual void OnSpawnedQuestActor(AActor* spawnedActor, UTATQuestActorSpawnerComponent* spawner) const override;

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const override;
#endif

   // The fact tag that will reveal the indicator once learned
   UPROPERTY(EditAnywhere, Category=SpawnIndicator, meta=(Categories="ClueFact"))
   FGameplayTag ClueFactTag;

   // Indicator to spawn
   UPROPERTY(EditAnywhere, Category=SpawnIndicator)
   TSubclassOf<ATATQuestSpawnIndicator> SpawnIndicatorClass;

   // Optional actor to use as the location of the indicator
   // If null, uses location of spawned actor
   UPROPERTY(EditAnywhere, Category=SpawnIndicator)
   TSoftObjectPtr<AActor> OptionalLocationProxy;
};
