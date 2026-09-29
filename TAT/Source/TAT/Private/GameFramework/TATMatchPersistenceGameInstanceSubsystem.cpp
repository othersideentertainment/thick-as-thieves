// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchPersistenceGameInstanceSubsystem)

void UTATMatchPersistenceGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATMatchPersistenceGameInstanceSubsystem::SetMatchPersistentData(const FMatchPersistentData& data)
{
   _persistentData = data;
   _bHasDataToPresent = true;
}

void UTATMatchPersistenceGameInstanceSubsystem::ResetMatchPersistentData()
{
   _bHasDataToPresent = false;
   _persistentData.Reset();
   _playerRankingPersistentData.Reset();
   _persistentXPGainedData.Reset();
}

void UTATMatchPersistenceGameInstanceSubsystem::SetPersistentPlayerRankingData(const TArray<FMatchPersistentRankingPlayerData>& persistentRankingData)
{
   _playerRankingPersistentData = persistentRankingData;
}

const TArray<FMatchPersistentRankingPlayerData>& UTATMatchPersistenceGameInstanceSubsystem::GetPersistentPlayerRankingData() const
{
   return _playerRankingPersistentData;
}

void UTATMatchPersistenceGameInstanceSubsystem::SetXPGainedData(const FMatchPersistentXPGainedData& persistentRankingData)
{
   _persistentXPGainedData = persistentRankingData;
}

const FMatchPersistentXPGainedData& UTATMatchPersistenceGameInstanceSubsystem::GetPersistentXPGainedData() const
{
   return _persistentXPGainedData;
}

