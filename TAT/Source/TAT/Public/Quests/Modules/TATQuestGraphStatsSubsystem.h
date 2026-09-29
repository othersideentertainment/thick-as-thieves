// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Subsystems/EngineSubsystem.h"

#if WITH_EDITOR

#include "TATQuestGraphStatsSubsystem.generated.h"

class UTATQuestGraphNode;

// Editor-only _engine_ subsystem to dump statistics from
// 1. Could have had it in the editor module, but then would have had to expose a hook in the game module that the editor module would implement
// 2. Mildly more convenient than a world subsystem, since being consumed in a context without a world
// 3. Not pushing stats directly onto objects, so can wipe fully each run
UCLASS()
class TAT_API UTATQuestGraphStatsSubsystem : public UEngineSubsystem
{
   GENERATED_BODY()

public:

   TOptional<float> GetLastSimulatedSpawnRate(TWeakObjectPtr<const UTATQuestGraphNode> node) const;
   void SetSpawnRates(const TMap<TWeakObjectPtr<const UTATQuestGraphNode>, int32>& spawnCounts, int simulationCount);

private:
   TMap<TWeakObjectPtr<const UTATQuestGraphNode>, float> _spawnRates;
};

#endif
