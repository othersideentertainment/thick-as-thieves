// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Modules/TATQuestGraph.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATActiveQuestSubsystem.generated.h"

class UTATQuestActorSpawnerComponent;
struct FTATMissionInfo;
struct FTATQuestObjectiveInfo;
struct FTATQuestInfo;
struct FTATContractInfo;
class UTATQuestGraphClueSet;


namespace ActiveQuestHelpers
{
   // TODO: move elsewhere?
   TAT_API TConstArrayView<FGameplayTag> GetMissionsForWorld(const UWorld* world);
   TAT_API int32 ChooseRandomMissionIndex(int32 seed, int32 count);
   const TCHAR* Indent(int32 level);

   // A tiny wrapper for a bitmask
   struct FPlayerMask
   {
      uint32 Mask = 0;
      void Reset();
      void Add(int32 playerId);
      bool Contains(int32 playerId) const;
   };
}

// Authority-only subsystem to track active quest/objectives for players in the match
UCLASS()
class TAT_API UTATActiveQuestSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   FGameplayTag GetMission() const { return _mission; }
   const FTATMissionInfo* GetMissionInfo() const;

   TOptional<float> GetEndgameDurationOverride() const;

   TOptional<float> GetQuestProperty_Float(ETATQuestGraphPropertyType propType) const;

   FGameplayTag GetContract() const { return _contract; }
   const FTATQuestObjectiveInfo* GetContractObjective() const;
   TConstArrayView<TSoftObjectPtr<UWorld>> GetContractSublevels() const;

   
   // Accomplices participate in the contract with their ally,
   // but are not officially on it, and don't get metagame rewards
   bool IsPlayerAccompliceForContract(int32 playerId) const;
   void ForEachObjective(TFunctionRef<void (const FTATQuestObjectiveInfo&)> handler) const;
   void ForEachContract(TFunctionRef<void (const FTATContractInfo&)> handler) const;

   /// Gets the ClueSourceTag on the modular quest for the current level
   FGameplayTag GetModularQuestClueSourceTag() const;
   
   const FTATClueFormatParams& GetMissionFormatParams() const;
   const FTATQuestObjectiveInfo* GetMissionObjective() const;
   TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> GetMissionSceneVariants() const;
   const FGameplayTagContainer& GetMissionQuestTags() const;
   TConstArrayView<FSoftObjectPath> GetMissionClueSetAssets() const;
   TConstArrayView<FTATClueSetForSpawn> GetClueSetsForMissionSpawns() const;
   TObjectPtr<const UTATQuestGraph> GetQuestGraph() const { return _questGraph; }

   void RegisterActorSpawner(UTATQuestActorSpawnerComponent* spawner);
   const TArray<TObjectPtr<UTATQuestActorSpawnerComponent>>& GetSpawners() const { return _spawners; }

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
   void _InitQuestGraph(UWorld* world, int32 seed);

private:
   FGameplayTag _contract;
   ActiveQuestHelpers::FPlayerMask _playersOfficiallyOnContract;
   FGameplayTag _mission;

   UPROPERTY(Transient)
   TArray<TObjectPtr<UTATQuestActorSpawnerComponent>> _spawners;

   UPROPERTY(Transient)
   TObjectPtr<const UTATQuestGraph> _questGraph;

   UPROPERTY(Transient)
   FTATQuestGraphResult _questGraphResult;
};
