// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueSpawner.h"

// ue
#include "CoreMinimal.h"
#include "Variation/TATSpawnerlikeWrapperActor.h"

#include "TATClueActorSpawner.generated.h"

// a clue-spawner component that spawns an actor with a provided initialization thunk
UCLASS()
class TAT_API UTATClueActorSpawnerComponent : public UTATClueSpawnerComponent
{
   GENERATED_BODY()

public:
   UTATClueActorSpawnerComponent();
   
   template<typename ActorT, typename FuncT>
   void SpawnClueActor(const TSoftClassPtr<ActorT>& actorClass, FuncT&& initialize) const
   {
      _SpawnClueActor(actorClass, [init = MoveTemp(initialize)](AActor* actor) { init(CastChecked<ActorT>(actor));});
   }

private:
   void _SpawnClueActor(TSoftClassPtr<AActor> actorClass, TFunction<void (AActor*)>&& initialize) const;
};

// A convenience wrapper actor for UTATClueActorSpawnerComponent
// CLUE-WIP: Should I just make a single wrapper, and let actors (BPs?) override the component?
UCLASS()
class TAT_API ATATClueActorSpawner : public ATATSpawnerlikeWrapperActor
{
   GENERATED_BODY()

public:
   ATATClueActorSpawner();

private:
   UPROPERTY(VisibleAnywhere)
   TObjectPtr<UTATClueActorSpawnerComponent> _spawner = nullptr;
};
