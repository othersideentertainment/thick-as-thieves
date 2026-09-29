// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATTutorialRunnerComponent.h"

// ue
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "TATTutorialRespawnManager.generated.h"

enum class ERespawnDuration : uint8;

// An actor in the tutorial level that resets the tutorial and world state
// when the player is KOed and respawns
UCLASS(Blueprintable)
class TAT_API ATATTutorialRespawnManager : public AActor
{
   GENERATED_BODY()

public:
   ATATTutorialRespawnManager();

protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
   virtual void Tick(float deltaTime) override;

private:
   void _OnUnconsciousTagChanged(FGameplayTag gameplayTag, int count);
   void _ResetWorld();
   void _DestroyTransientActors();

   void _SuspendTutorial();
   void _ResumeTutorial();

   void _OverrideRespawnDuration();
   void _RestoreRespawnDuration();

   // Base classes of actors to be destroyed when the player respawns
   UPROPERTY(EditDefaultsOnly, Category="Tutorial")
   TArray<TSoftClassPtr<AActor>> _baseClassesToDestroy;

   FTATTutorialResumeSnapshot _tutorialSnapshot;
   TWeakObjectPtr<UTATTutorialRunnerComponent> _tutorialRunner;

   // Overrides the respawn duration in match settings
   UPROPERTY(EditDefaultsOnly, Category="Tutorial")
   ERespawnDuration _respawnDurationOverride = static_cast<ERespawnDuration>(0);
   ERespawnDuration _previousRespawnDuration = static_cast<ERespawnDuration>(0);;
};
