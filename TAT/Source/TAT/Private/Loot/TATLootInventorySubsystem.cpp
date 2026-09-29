// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootInventorySubsystem.h"

// tat
#include "Loot/TATLootInventory.h"

// ose
#include "Character/OSETeamInterface.h"
#include "GameFramework/PlayerState.h"
#include "Player/TATPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventorySubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventorySubsystem, Log, All);

void UTATLootInventorySubsystem::RegisterLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent)
{
   if (!IsValid(lootInventoryComponent) || _registeredInventories.Contains(lootInventoryComponent))
   {
      return;
   }

   FInventoryInfo info{};

   APlayerState* owningPlayerState = Cast<APlayerState>(lootInventoryComponent->GetOwner());
   info.PlayerOwner = owningPlayerState;

   if (ATATPlayerState* ps = Cast<ATATPlayerState>(owningPlayerState))
   {
      const uint8 team = ps->GetTeam();
      if (team != IOSETeamInterface::kInvalidTeam)
      {
         info.Team = team;
      }
   }

   if (owningPlayerState != nullptr)
   {
      info.LootValueChangedDelegateHandle = lootInventoryComponent->OnAuthorityLootValueChanged.AddLambda(
         [weakThis = MakeWeakObjectPtr(this), weakInventory = MakeWeakObjectPtr(lootInventoryComponent)]()
         {
            UTATLootInventorySubsystem* self = weakThis.Get();
            if (self == nullptr)
            {
               return;
            }
            UTATLootInventoryComponent* inventory = weakInventory.Get();
            if (inventory == nullptr)
            {
               return;
            }
            if (FInventoryInfo* info = self->_registeredInventories.Find(inventory))
            {
               if (info->Team)
               {
                  // if we know the team already, just fire the event
                  self->GetLootValueChangedDelegateForTeam(*info->Team).Broadcast(*info->Team, inventory);
                  info->LootValueDirty = false;
               }
               else
               {
                  // unknown team, mark it as dirty so it can be fired when we do know the team
                  info->LootValueDirty = true;
               }
            }
         });
   }

   _registeredInventories.Add(lootInventoryComponent, info);
   OnLootInventoryComponentRegistered.Broadcast(lootInventoryComponent);

   UE_LOG(LogTATLootInventorySubsystem, Verbose, TEXT("Registered loot inventory component %s (owned by %s)"),
      *GetNameSafe(lootInventoryComponent), *GetNameSafe(lootInventoryComponent->GetOwner()));
}

void UTATLootInventorySubsystem::UnregisterLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent)
{
   if (!IsValid(lootInventoryComponent))
   {
      return;
   }
   FInventoryInfo* info = _registeredInventories.Find(lootInventoryComponent);
   if (info == nullptr)
   {
      return;
   }
   if (info->LootValueChangedDelegateHandle.IsValid())
   {
      lootInventoryComponent->OnAuthorityLootValueChanged.Remove(info->LootValueChangedDelegateHandle);
      info->LootValueChangedDelegateHandle.Reset();
   }
   _registeredInventories.Remove(lootInventoryComponent);
   OnLootInventoryComponentUnregistered.Broadcast(lootInventoryComponent);

   UE_LOG(LogTATLootInventorySubsystem, Verbose, TEXT("UN-registered loot inventory component %s (owned by %s)"),
      *GetNameSafe(lootInventoryComponent), *GetNameSafe(lootInventoryComponent->GetOwner()));
}

void UTATLootInventorySubsystem::NotifyLootInventorySetTeam(UTATLootInventoryComponent* lootInventoryComponent, uint8 newTeam)
{
   if (!IsValid(lootInventoryComponent))
   {
      return;
   }

   FInventoryInfo* info = _registeredInventories.Find(lootInventoryComponent);

   if (info == nullptr)
   {
      RegisterLootInventoryComponent(lootInventoryComponent);
      info = _registeredInventories.Find(lootInventoryComponent);
   }
   
   if (info == nullptr)
   {
      return;
   }

   const bool teamValueNewlyAssigned = !info->Team.IsSet();
   const bool teamValueChanged = !info->Team || *info->Team != newTeam;

   info->Team = newTeam;

   UE_LOG(LogTATLootInventorySubsystem, Verbose, TEXT("Loot inventory %s (owned by %s) changed team to %i"),
      *GetNameSafe(lootInventoryComponent), *GetNameSafe(lootInventoryComponent->GetOwner()), (int32)newTeam);

   if (teamValueChanged)
   {
      OnLootInventoryComponentTeamChanged.Broadcast(newTeam, lootInventoryComponent);
   }

   // broadcast the loot value changed event needed
   if (teamValueNewlyAssigned && info->LootValueDirty)
   {
      GetLootValueChangedDelegateForTeam(newTeam).Broadcast(newTeam, lootInventoryComponent);
      info->LootValueDirty = false;
   }
}

void UTATLootInventorySubsystem::ForEachLootInventoryComponent(TFunctionRef<void(UTATLootInventoryComponent*)> callback) const
{
   for (const TPair<TWeakObjectPtr<UTATLootInventoryComponent>, FInventoryInfo>& pair : _registeredInventories)
   {
      if (UTATLootInventoryComponent* inventoryComponent = pair.Key.Get())
      {
         callback(inventoryComponent);
      }
   }
}

void UTATLootInventorySubsystem::ForEachPlayerOwnedLootInventoryComponent(TFunctionRef<void(UTATLootInventoryComponent*)> callback, TOptional<uint8> teamFilter) const
{
   for (const TPair<TWeakObjectPtr<UTATLootInventoryComponent>, FInventoryInfo>& pair : _registeredInventories)
   {
      UTATLootInventoryComponent* inventoryComponent = pair.Key.Get();
      if (inventoryComponent == nullptr || !pair.Value.IsReadyAndPlayerOwned())
      {
         continue;
      }
      
      if (teamFilter && *teamFilter != *pair.Value.Team)
      {
         continue;
      }

      callback(inventoryComponent);
   }
}

bool UTATLootInventorySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}
