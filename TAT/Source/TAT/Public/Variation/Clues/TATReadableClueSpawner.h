// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueActorSpawner.h"

// ue
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "TATReadableClueSpawner.generated.h"

class ATATReadableClueActor;

// Types of readable clues, so that spawners can be specific
UENUM()
enum class ETATReadableClueType : uint8
{
   // Something that would be placed horizontally, like a note
   Horizontal,
   // Something that would be placed vertically, like a poster?
   Vertical
};


// A visual configuration for a readable clue, with subtype included
UCLASS()
class TAT_API UTATReadableClueVisuals : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere)
   TSoftClassPtr<ATATReadableClueActor> ReadableActorClass;

   UPROPERTY(EditAnywhere)
   ETATReadableClueType ReadableType;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
#endif
};

UCLASS()
class TAT_API UTATReadableClueSpawner : public UTATClueActorSpawnerComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATReadableClueSpawner();

   virtual FTATClueBucketKey GetClueBucket() const override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

protected:
   UPROPERTY(EditDefaultsOnly, Category=Clues, meta=(DisplayPriority=1))
   ETATReadableClueType ReadableType;

   // A tag that is matched with a placement tag on the clue spawner (That is, the spawner for the readable).
   //
   // For example, if categorizing by rarity, a readable clue marked CluePlacement.Common would only go to a
   // readable spawner configured with CluePlacement.Common.
   UPROPERTY(EditAnywhere, Category=Clues, meta=(DisplayPriority=1, Categories="CluePlacement"))
   FGameplayTag PlacementTag;
};
