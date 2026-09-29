// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATThievesDenQuestSubsystem.generated.h"

class UTATSaveGame;
struct FTATLootIdentifier;
struct FTATLootInstanceId;
enum class ETATContractOutroFlow : uint8;

// A subsystem to keep track of contract-derived data in the thieves den
// e.g. which contracts need to be completed
//
// TODO: rename to just be contracts?
UCLASS(BlueprintType)
class TAT_API UTATThievesDenQuestSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
   virtual void Deinitialize() override;

   DECLARE_MULTICAST_DELEGATE(FOnCompletableQuestsChanged);
   FOnCompletableQuestsChanged OnCompletableContractsChanged;

   FGameplayTag GetCompletableContractForFlow(ETATContractOutroFlow flow) const;

   UFUNCTION(BlueprintPure, Category = "Contracts")
   FGameplayTag GetStartableContractTag() const { return _startableContract; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartableContractChanged, FGameplayTag, startableContractTag);
   UPROPERTY(BlueprintAssignable, Category = "Contracts")
   FOnStartableContractChanged OnStartableContractChanged;

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

   UFUNCTION()
   void _RefreshContracts();

   void _ReconcileTriggeredContracts();
private:
   UPROPERTY(Transient)
   TObjectPtr<UTATSaveGame> _saveGame;

   using FCompletableContractMap = TMap<ETATContractOutroFlow, FGameplayTag, TInlineSetAllocator<8>>;
   
   // only the first possible quest for each type
   FCompletableContractMap _completableContracts;
   FGameplayTag _startableContract;
};
