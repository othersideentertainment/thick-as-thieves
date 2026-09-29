// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TATAuthMapSeedSubsystem.generated.h"

// Authority-only source of truth for the map seed
// Needed in cases where the game mode may not exist yet
//
// TATGameState remains the place for the replicated map seed
UCLASS()
class TAT_API UTATAuthMapSeedSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   
   int32 GetOrCreateSeed();

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

   int32 _ChooseSeed() const;

   int32 _seed = 0;
   bool _hasSeed = false;
};
