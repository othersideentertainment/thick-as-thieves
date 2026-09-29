// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATDefaultDifficultySubsystem.generated.h"

struct FTATMapNodeSettings;
enum class EOSESaveDataState : uint8;
enum class ETATDifficulty : uint8;

// Subsystem to persist selected difficulty, and default difficulty to Thief once it is unlocked
//
// Likely overkill to make this a subsystem, but it needed game-instance
// lifetime in order to not override player choices, and there wasn't a
// great place to stuff it. (now less overkill since it does more stuff)
UCLASS()
class TAT_API UTATDefaultDifficultySubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

private:
   UFUNCTION()
   void _OnSaveDataStateChanged(EOSESaveDataState state);
   void _WaitForSave();
   void _InitializeFromSave();
   UFUNCTION()
   void _OnUnlocksChanged();
   void _RefreshDifficultyUnlock(bool isInitial);
   UFUNCTION()
   void _OnMapNodeSettingsChanged(const FTATMapNodeSettings& newMapNodeSettings);

   void _SetGameInstanceDifficulty(ETATDifficulty newDifficulty);

   bool _hadUnlockedNormal = false;
};
