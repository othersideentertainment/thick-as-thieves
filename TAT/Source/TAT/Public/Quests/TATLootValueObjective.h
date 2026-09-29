// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestObjectiveTracker.h"

#include "TATLootValueObjective.generated.h"

class UTATLootInventoryComponent;

/// Steal loot worth a combined total value
USTRUCT(meta = (DisplayName = "Loot Value"))
struct TAT_API FTATLootValueObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere)
   int32 Amount = 0;

   // Objective text used in the metagame
   // Replacement keywords are {Amount}
   // Default defined in Project Settings
   UPROPERTY(EditAnywhere, meta = (EditCondition="OverrideObjectiveText"))
   FText ObjectiveText;

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool OverrideObjectiveText = false;

   virtual FText GetObjectiveText(const UObject* worldContext) const override;
   virtual FGameplayTag GetRelatedLootTag() const override;
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual ETATQuestObjectiveProgressStyle GetProgressStyle() const override { return ETATQuestObjectiveProgressStyle::Integer; }
   virtual int32 GetTargetProgress() const override { return Amount; }
   virtual FString GetDebugDescription() const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

UCLASS()
class TAT_API UTATLootValueObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

public:
   UTATLootValueObjectiveTracker();
   virtual void Initialize(const FTATObjectiveTrackerContext& context) override;
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const override;
   virtual void CheatComplete() override;

private:
   void _OnOwningPlayerTeamChanged(uint8 newTeam);
   void _OnLootInventoryComponentLootValueChanged(uint8 team, UTATLootInventoryComponent* inventoryComponent);
   int32 _GetCurrentLootValue() const;
   void _RefreshObjectiveState();
   bool _IsAlliedCoopPlayerInventory(UTATLootInventoryComponent* inventoryComponent) const;

   int32 _amount = 0;

   UPROPERTY(Transient)
   TObjectPtr<UTATLootInventoryComponent> _inventory;

   uint8 _owningPlayerTeam;
};
