// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATLootInventorySubsystem.generated.h"

class UTATLootInventoryComponent;

// A subsystem to keep track of all loot inventory components
UCLASS()
class TAT_API UTATLootInventorySubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   void RegisterLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent);
   void UnregisterLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent);
   void NotifyLootInventorySetTeam(UTATLootInventoryComponent* lootInventoryComponent, uint8 newTeam);

   /// Calls the callback for each registered loot inventory component.
   void ForEachLootInventoryComponent(TFunctionRef<void(UTATLootInventoryComponent*)> callback) const;

   /// Calls the callback for each registered loot inventory component.
   void ForEachPlayerOwnedLootInventoryComponent(TFunctionRef<void(UTATLootInventoryComponent*)> callback, TOptional<uint8> teamFilter = {}) const;

   DECLARE_MULTICAST_DELEGATE_OneParam(FLootInventoryComponentDelegate, UTATLootInventoryComponent*);
   FLootInventoryComponentDelegate OnLootInventoryComponentRegistered;
   FLootInventoryComponentDelegate OnLootInventoryComponentUnregistered;
   
   DECLARE_MULTICAST_DELEGATE_TwoParams(FTeamLootInventoryComponentDelegate, uint8, UTATLootInventoryComponent*);
   FTeamLootInventoryComponentDelegate OnLootInventoryComponentTeamChanged;
   FTeamLootInventoryComponentDelegate& GetLootValueChangedDelegateForTeam(uint8 team) { return _perTeamLootValueChangedDelegates.FindOrAdd(team); }

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   TMap<uint8, FTeamLootInventoryComponentDelegate> _perTeamLootValueChangedDelegates;

   struct FInventoryInfo
   {
      TWeakObjectPtr<APlayerState> PlayerOwner;
      TOptional<uint8> Team;
      bool LootValueDirty = false; // Has the loot value event fired before SetTeam was called?
      FDelegateHandle LootValueChangedDelegateHandle;
      inline bool IsReadyAndPlayerOwned() const { return PlayerOwner.Get() != nullptr && Team.IsSet(); }
   };
   TMap<TWeakObjectPtr<UTATLootInventoryComponent>, FInventoryInfo> _registeredInventories;
};
