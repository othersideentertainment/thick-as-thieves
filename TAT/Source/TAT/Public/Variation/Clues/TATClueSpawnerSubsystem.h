// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATClueSpawnerSubsystem.generated.h"

class UTATClueSpawnerComponent;

// Authority-only subsystem that keeps track of clue spawners
UCLASS()
class TAT_API UTATClueSpawnerSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;

   void RegisterSpawner(UTATClueSpawnerComponent* spawner);
   void UnregisterSpawner(UTATClueSpawnerComponent* spawner);

   const TArray<TObjectPtr<UTATClueSpawnerComponent>>& GetClueSpawners() const { return _clueSpawners; }

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   UPROPERTY(Transient)
   TArray<TObjectPtr<UTATClueSpawnerComponent>> _clueSpawners;
};
