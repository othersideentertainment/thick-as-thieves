// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueInfo.h"

#include "TATActorTestClue.generated.h"

// A clue type that just spawns an arbitrary actor
// CLUE-WIP: Probably rename if this is going to stick around. SimpleActorClue?
USTRUCT()
struct TAT_API FTATActorTestClue final : public FTATClueInfo
{
   GENERATED_BODY()
public:
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   virtual FString GetDebugDescription() const override { return TATClueInfoHelpers::FormatDebugDescription(TEXT("Actor"), ActorToSpawn.GetAssetName()); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
   
   UPROPERTY(EditAnywhere)
   TSoftClassPtr<AActor> ActorToSpawn;
};
