// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionSettings.h"

// ue
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionSettings)
DEFINE_LOG_CATEGORY_STATIC(LogTATAIReactionSettings, Log, All);

void UTATAIReactionSettings::LoadEventConfigurationsAsync()
{
   if (_tableLoadRequested)
   {
      return;
   }

   _tableLoadRequested = true;
   UAssetManager::GetStreamableManager().RequestAsyncLoad(EventConfigurationsTable.ToSoftObjectPath());
}

bool UTATAIReactionSettings::HasEventConfigForTarget(const FTATAIReactionTarget& target, bool shouldCheckIsEventEnabled) const
{
   FTATAIReactionEventConfigId eventConfigId = FindEventConfigIdForTarget(target);
   if (eventConfigId.IsValid())
   {
      if (!shouldCheckIsEventEnabled)
      {
         return true;
      }

      const FTATAIReactionEventConfig* eventConfig = GetEventConfig(eventConfigId);
      check(eventConfig != nullptr);
      return eventConfig->IsEnabled;
   }

   return false;
}

FTATAIReactionEventConfigId UTATAIReactionSettings::FindEventConfigIdForTarget(const FTATAIReactionTarget& target) const
{
   const UDataTable* dataTable = _GetEventConfigurationsTable();
   check(dataTable);

   for (const auto& it : dataTable->GetRowMap())
   {
      const FName tableRowName = it.Key;
      FTATAIReactionEventConfig* eventConfig = dataTable->FindRow<FTATAIReactionEventConfig>(tableRowName, TEXT("FindEventConfigForTarget"));
      check(eventConfig);

      if (eventConfig->Target.TargetInstanceMeetsCriteria(target))
      {
         return FTATAIReactionEventConfigId(tableRowName);
      }
   }

   UE_LOG(LogTATAIReactionSettings, Error, 
      TEXT("UTATAIReactionSettings::FindEventConfigForTarget failed to find a config for %s target!"),
      *target.ToString());
   return FTATAIReactionEventConfigId();
}

const FTATAIReactionEventConfig* UTATAIReactionSettings::GetEventConfig(const FTATAIReactionEventConfigId& eventId) const
{
   const UDataTable* dataTable = _GetEventConfigurationsTable();
   check(dataTable);

   return dataTable->FindRow<FTATAIReactionEventConfig>(eventId.TableRowName, TEXT("GetEventConfig"));
}

const FTATAIConditionalReactionRoleConfig* UTATAIReactionSettings::GetConditionalRoleConfig(
   const FTATAIReactionEventConfigId& eventId,
   const FTATAIConditionalReactionRoleConfigId& roleId) const
{
   if (const FTATAIReactionEventConfig* eventConfig = GetEventConfig(eventId))
   {
      if (eventConfig->ConditionalRoles.IsValidIndex(roleId.RoleIndex))
      {
         return &eventConfig->ConditionalRoles[roleId.RoleIndex];
      }
      else
      {
         UE_LOG(LogTATAIReactionSettings, Error, 
            TEXT("[%hs] Failed to find role %i in conditional roles for %s"),
            __FUNCTION__,
            roleId.RoleIndex,
            *eventId.TableRowName.ToString());
         return nullptr;
      }
   }

   // This can validly fail if the roleId passed in is from a default role.
   return nullptr;
}

UDataTable* UTATAIReactionSettings::_GetEventConfigurationsTable() const
{
   // Will only load synchronously if not already loaded. Should hopefully already
   // be loaded async from LoadEventConfigurationsAsync(), but load instantly if not.
   return EventConfigurationsTable.LoadSynchronous();
}
