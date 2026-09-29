// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionEvent.h"
//#include "Developer/TATDataTableMap.h"

// ue
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "TATAIReactionSettings.generated.h"

struct FStreamableHandle;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] AI Reaction Settings"))
class TAT_API UTATAIReactionSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // For Blueprint
   UFUNCTION(BlueprintPure, Category = "TAT|AI|Reaction")
   static UTATAIReactionSettings* GetAIReactionSettings() { return GetMutableDefault<UTATAIReactionSettings>(); }

   // For C++
   static const UTATAIReactionSettings& Get() { return *GetDefault<UTATAIReactionSettings>(); }
   static UTATAIReactionSettings& GetMutable() { return *GetMutableDefault<UTATAIReactionSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATAIReactionEventConfig"))
   TSoftObjectPtr<UDataTable> EventConfigurationsTable;

   void LoadEventConfigurationsAsync();

   bool HasEventConfigForTarget(const FTATAIReactionTarget& target, bool shouldCheckIsEventEnabled = true) const;

   FTATAIReactionEventConfigId FindEventConfigIdForTarget(const FTATAIReactionTarget& target) const;

   const FTATAIReactionEventConfig* GetEventConfig(const FTATAIReactionEventConfigId& eventId) const;
   const FTATAIConditionalReactionRoleConfig* GetConditionalRoleConfig(
      const FTATAIReactionEventConfigId& eventId,
      const FTATAIConditionalReactionRoleConfigId& roleId) const;

private:
   bool _tableLoadRequested = false;

   UDataTable* _GetEventConfigurationsTable() const;
	
};
