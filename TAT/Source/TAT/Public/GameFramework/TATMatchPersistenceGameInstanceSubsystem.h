// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "Loot/TATLootTypes.h"
#include "TATMatchPersistentTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TATMatchPersistenceGameInstanceSubsystem.generated.h"


UCLASS()
class TAT_API UTATMatchPersistenceGameInstanceSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()
public:
   // USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   void SetMatchPersistentData(const FMatchPersistentData& data);

   UFUNCTION(BlueprintCallable)
   FMatchPersistentData& GetMatchPersistentData() { return _persistentData; }

   UFUNCTION(BlueprintCallable)
   bool HasDataToPresent() const { return _bHasDataToPresent; }
   
   UFUNCTION(BlueprintCallable)
   void ResetMatchPersistentData();

   void SetPersistentPlayerRankingData(const TArray<FMatchPersistentRankingPlayerData>& persistentRankingData);

   const TArray<FMatchPersistentRankingPlayerData>& GetPersistentPlayerRankingData() const;

   void SetXPGainedData(const FMatchPersistentXPGainedData& persistentRankingData);
   const FMatchPersistentXPGainedData& GetPersistentXPGainedData() const;

protected:

   UPROPERTY(Transient)
   bool _bHasDataToPresent { false };
   
   UPROPERTY(Transient)
   FMatchPersistentData _persistentData;

   UPROPERTY(Transient)
   TArray<FMatchPersistentRankingPlayerData> _playerRankingPersistentData;

   UPROPERTY(Transient)
   FMatchPersistentXPGainedData _persistentXPGainedData;
};
