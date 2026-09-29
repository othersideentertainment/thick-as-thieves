// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueType.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue
#include "Components/ActorComponent.h"

#include "TATClueSpawner.generated.h"

enum class ETATClueType : uint8;

// Component that handles the spawning of a clue in a specific location
// Will have subclasses that can handle a specific clue type
UCLASS(Abstract)
class TAT_API UTATClueSpawnerComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATClueSpawnerComponent();

   // using BeginPlay here, as I want to pair with end play and slightly care about the reason
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason);

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

   
   virtual FTATClueBucketKey GetClueBucket() const;

   FGameplayTag GetRequiredSourceLocation() const { return _isLocationSpecific ? _requiredSourceLocation : FGameplayTag();}
   const FTATSceneRequirement& GetSceneRequirement() const { return _sceneRequirement; }

   // Run after all clues have been applied to the spawner
   // Not guaranteed to be called if there are no clues for this spawner
   // CLUE-WIP: I don't really like this name
   virtual void OnAllCluesApplied() {}

   // Just for unit tests
   void SetClueType_TEST(ETATClueType clueType) { _clueType = clueType; }

protected:
   // If set, only clues that originate from a place with a matching tag can use this
   // Partial matches are allowed (e.g. ClueLocation.NiftyHouse will match something from ClueLocation.NiftyHouse.CreepyBasement)
   UPROPERTY(EditInstanceOnly, Category = "Clues", meta = (DisplayName = "Required Source Location", Categories = "ClueLocation", EditCondition = "_isLocationSpecific"))
   FGameplayTag _requiredSourceLocation;

   UPROPERTY(EditInstanceOnly, Category = "Clues", meta = (InlineEditConditionToggle))
   bool _isLocationSpecific = false;
   
   // explicitly _not_ a property, as only expected to be set in code, for default implementation
   // worth keeping?
   ETATClueType _clueType = static_cast<ETATClueType>(0);

   UPROPERTY(EditAnywhere, Category = "Clue Scene Requirement", meta = (ShowOnlyInnerProperties))
   FTATSceneRequirement _sceneRequirement;
};
