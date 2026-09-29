// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "Variation/TATExternalSpawnerDependencyInterface.h"
#include "Variation/TATSpawnerlikeWrapperActor.h"
#include "Variation/Clues/TATClueLocationInterface.h"

// ue
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATQuestActorSpawner.generated.h"

struct FTATQuestObjectiveInfo;
struct FInstancedStruct;

UCLASS(hideCategories=(ComponentTick), meta=(BlueprintSpawnableComponent))
class TAT_API UTATQuestActorSpawnerComponent : public USceneComponent, public ITATClueLocationInterface
{
   GENERATED_BODY()

public:
   UTATQuestActorSpawnerComponent();

   bool IsEnabled() const { return _isEnabled; }
   const FGameplayTag& GetQuestLocationTag() const { return _questLocationTag; }
   const FTATSceneRequirement& GetSceneRequirement() const { return _sceneRequirement; }

   void ExecuteSpawn(const TSoftClassPtr<AActor>& actorClass);

   // ITATClueLocationInterface
   virtual const FText& GetClueLocationName() const override { return _clueLocationText; }
   virtual const FGameplayTag& GetClueLocationTag() const override final { return _clueLocationTag; }
   // ITATClueLocationInterface end

#if WITH_EDITOR
   virtual void CheckForErrors() override final;

   // for vis adapter
   const FTATSceneRequirement* FindSceneRequirement() const { return &_sceneRequirement; }
#endif

   TMulticastDelegate<void(AActor*)> OnActorSpawned;

protected:
   // from UActorComponent
   virtual void InitializeComponent() override;

private:

   UPROPERTY(EditAnywhere, DisplayName="Is Enabled", Category=QuestSpawner)
   bool _isEnabled = true;

   // The category of quest location this spawner represents (e.g. office)
   UPROPERTY(EditAnywhere, DisplayName="Quest Location Tag", Category=QuestSpawner, meta = (EditCondition = "_isEnabled", Categories="QuestLocation"))
   FGameplayTag _questLocationTag;

   // Tag that represents this location, so that there can be clue sets specific to it
   UPROPERTY(EditAnywhere, DisplayName = "Clue Location Tag", Category=QuestSpawner, meta = (Categories="ClueLocation"))
   FGameplayTag _clueLocationTag;
   
   // Player-visible location name to be injected into quest objective text
   UPROPERTY(EditInstanceOnly, DisplayName="Clue Location Text", Category=QuestSpawner)
   FText _clueLocationText;

   UPROPERTY(EditAnywhere, Category=QuestSpawner, meta = (EditCondition = "_isEnabled", ShowOnlyInnerProperties))
   FTATSceneRequirement _sceneRequirement;

   // Actions that will be run on the spawned actor
   UPROPERTY(EditAnywhere, DisplayName="Spawn Actions", Category="QuestSpawner", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestActorSpawnAction"))
   TArray<FInstancedStruct> _spawnActions;
};

UCLASS(Blueprintable, ConversionRoot)
class TAT_API ATATQuestActorSpawner : public ATATSpawnerlikeWrapperActor, public ITATExternalSpawnerDependencyInterface
{
	GENERATED_BODY()

public:
   ATATQuestActorSpawner(const FObjectInitializer& objectInitializer);

   UTATQuestActorSpawnerComponent* GetQuestSpawner() const { return _questSpawner; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuestSpawner, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTATQuestActorSpawnerComponent> _questSpawner;
};
