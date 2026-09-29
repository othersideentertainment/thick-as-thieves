// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATQuestHandle.generated.h"

struct FTATMinimalQuestInfo;
struct FTATQuestInfo;
struct FTATContractInfo;
struct FTATMissionInfo;

// A struct for blueprint exposure of QuestInfo that wraps a row-handle
// 
// (The info struct would heap alloc on copy, so avoiding exposing it to BP directly)
// 
// There is an argument that this isn't doing much over helper methods that
// do tag->thing lookups, but it may be helpful for more repeated lookups.
// Hopefully.
USTRUCT(BlueprintType)
struct FTATQuestHandle
{
   GENERATED_BODY()

   FTATQuestHandle() = default;

   explicit FTATQuestHandle(const FDataTableRowHandle& rowHandle)
      : _rowHandle(rowHandle)
      {}

   FGameplayTag GetQuestTag() const;
   bool IsValid() const { return !_rowHandle.IsNull(); }

   const FTATQuestInfo* GetQuest() const;
   const FTATContractInfo* GetContract() const;
   const FTATMissionInfo* GetMission() const;
   const FTATMinimalQuestInfo* GetMinimalQuest() const;

   bool operator==(const FTATQuestHandle& other) const
   {
      return _rowHandle == other._rowHandle;
   }

   bool operator!=(const FTATQuestHandle& other) const
   {
      return !(*this == other);
   }

private:
   UPROPERTY()
   FDataTableRowHandle _rowHandle;
};

template<>
struct TStructOpsTypeTraits<FTATQuestHandle> : public TStructOpsTypeTraitsBase2<FTATQuestHandle>
{
   enum
   {
      WithIdenticalViaEquality = true,
   };
};
