// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "Quests/TATQuestHandle.h"
#include "UI/TATScreenWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATContractJournalScreen.generated.h"

class UTATUIQueueAction;
class UListView;
struct FGameplayTag;
enum class ETATContractState : uint8;

UCLASS(BlueprintType)
class TAT_API UTATContractStateUIProxy : public UObject
{
   GENERATED_BODY()
public:
   UPROPERTY(BlueprintReadOnly)
   FTATQuestHandle Contract;

   UPROPERTY(BlueprintReadOnly)
   ETATContractState ContractState = static_cast<ETATContractState>(0);
};

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATContractJournalScreen : public UTATScreenWidget
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintPure, Category="Quest|Flow", meta=(DisplayName="Create Contract Journal Action"))
   static UTATUIQueueAction* CreateAction(const FGameplayTag& initialQuestTag);

protected:
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   UListView* ListView;

private:
   UPROPERTY(Transient)
   TArray<UTATContractStateUIProxy*> _quests;

   FGameplayTag _initialContractTag;
};
