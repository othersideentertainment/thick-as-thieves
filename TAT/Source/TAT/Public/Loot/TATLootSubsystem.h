// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Developer/TATDataTableMap.h"
#include "Loot/TATLootTypes.h"

// ue
#include "Subsystems/GameInstanceSubsystem.h"


#include "TATLootSubsystem.generated.h"

class ATATPlayerState;
class UDataTable;


UCLASS(BlueprintType)
class TAT_API UTATLootSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   UTATLootSubsystem() {}

   static const UTATLootSubsystem* Get(const UObject* contextObj);

   // USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   // Returns an array of player states in order from highest -> lowest total stashed loot value
   UFUNCTION(BlueprintPure, meta=(WorldContext="contextObject"))
   static TArray<ATATPlayerState*> GetPlayersRankedByStashedLoot(const UObject* contextObject);

   static int GetStashedLootRankIndexForPlayer(const ATATPlayerState* playerState);
   
   UFUNCTION(BlueprintCallable)
   const UDataTable* GetLootDataTable() const;

   const FTATLootInfo* FindLootInfo(const FTATLootIdentifier& lootId) const;
   const FTATLootInfo& GetLootInfoChecked(const FTATLootIdentifier& lootId) const;

private:
   UPROPERTY(Transient)
   TObjectPtr<const UDataTable> _lootDataTable = nullptr;

   // Map of loot identifier to data table row names
   TTATDataTableMap<FTATLootInfo, FTATLootIdentifier> _lootDataTableMap;
};
