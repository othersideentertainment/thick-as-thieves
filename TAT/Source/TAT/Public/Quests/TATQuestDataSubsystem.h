// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TATQuestDataSubsystem.generated.h"

class UDataTable;
struct FGameplayTag;
struct FTATQuestInfo;
struct FTATMissionInfo;
struct FTATMinimalQuestInfo;
struct FTATContractInfo;
struct FTATQuestObjectiveInfo;

// Subsystem that keeps loaded quest data
// This is the N-th time building a quasi-data-registry
UCLASS()
class TAT_API UTATQuestDataSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   static const UTATQuestDataSubsystem& Get(const UObject* worldContext);
   static const UTATQuestDataSubsystem* TryGet(const UObject* worldContext);

   const FTATQuestInfo* FindQuestInfo(const FGameplayTag& questTag) const;
   const FTATMinimalQuestInfo* FindMinimalQuestInfo(const FGameplayTag& questTag) const;
   const FTATContractInfo* FindContractInfo(const FGameplayTag& questTag) const;
   const FTATMissionInfo* FindMissionInfo(const FGameplayTag& questTag) const;
   const FTATQuestObjectiveInfo* FindContractObjective(const FGameplayTag& questTag) const;

   bool FindQuestHandle(const FGameplayTag& questTag, struct FTATQuestHandle& outHandle) const;

protected:
   const UDataTable* _GetTableForMinimalTag(const FGameplayTag& questTag) const;

   const UDataTable* _GetContractData() const;
   const UDataTable* _GetMissionData() const;

private:
   UPROPERTY(Transient)
   mutable TObjectPtr<UDataTable> _contractData;
   
   UPROPERTY(Transient)
   mutable TObjectPtr<UDataTable> _missionData;
};
