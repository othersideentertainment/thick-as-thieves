// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"

// tat
#include "Loot/TATLootUtils.h"

#include "Quests/TATThievesDenQuestSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSavedLootItemUIProxy)
DEFINE_LOG_CATEGORY_STATIC(LogTATSavedLootItemUIProxy, Log, All);


void UTATSavedLootItemUIProxy::BuildFromLootStack(const FTATSavedLootStack& stack, const FTATLootInfo& info)
{
   _id = stack.LootIdentifier;
   UTATLootUtils::FindLootMetadata(GetWorld(), info.LootIdentifier, _metadata);
   _state.Quantity = stack.Count;
   _state.HasDateAcquired = false;
}

void UTATSavedLootItemUIProxy::Reset()
{
   _id = FTATLootIdentifier();
   _metadata = FTATLootMetadataBP();
   _state = FTATSavedLootItemUIProxyState();
   _isModifiable = false;
}

void UTATSavedLootItemUIProxy::SetSellQuantity(int32 quantity)
{
   if (_isModifiable || quantity == 0)
   {
      if (quantity >= 0 && quantity <= _state.Quantity)
      {
         _sellQuantity = quantity;
      }
      else
      {
         UE_LOG(LogTATSavedLootItemUIProxy, Error, TEXT("[%s] SetSellQuantity calls with quantity %d, outside the bounds of 0->%d")
            , *GetName(), quantity, _state.Quantity);
      }
   }
   else
   {
      UE_LOG(LogTATSavedLootItemUIProxy, Error, TEXT("[%s] SetSellQuantity called on a non-modifiable item with non-zero quantity %d!")
         , *GetName(), quantity);
   }
}

void UTATSavedLootItemUIProxy::UpdateLootStackState(const FTATSavedLootStack& stack)
{
   if (stack.Count != _state.Quantity)
   {
      _state.Quantity = stack.Count;
      _sellQuantity = FMath::Clamp(_sellQuantity, 0, _state.Quantity);
      OnDataChanged.Broadcast();
   }
}

void UTATSavedLootItemUIProxy::RefreshIsModifiable(const UTATThievesDenQuestSubsystem* questSubsystem)
{
   // TODO: delete further
   const bool newIsModifiable = true;
   if (_isModifiable != newIsModifiable)
   {
      _isModifiable = newIsModifiable;
      OnDataChanged.Broadcast();
   }
}
