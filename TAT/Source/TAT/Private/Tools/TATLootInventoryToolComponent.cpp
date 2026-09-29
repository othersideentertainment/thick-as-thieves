// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATLootInventoryToolComponent.h"

// tat
#include "Loot/TATLootInventory.h"

// ose
#include "Items/ToolSetComponent.h"

// ue5
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventoryToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventoryToolComponent, Log, All);

UTATLootInventoryToolComponent::UTATLootInventoryToolComponent()
{
}

UTATLootInventoryComponent* UTATLootInventoryToolComponent::GetOwningInventory() const
{
   if (AActor* owner = GetOwner())
   {
      UTATLootInventoryComponent* inventory = UTATLootInventoryComponent::GetForActor(GetOwner());
      if (IsValid(inventory))
      {
         return inventory;
      }
   }
   return nullptr;
}

bool UTATLootInventoryToolComponent::OnAddToToolSet_Implementation()
{
   Super::OnAddToToolSet_Implementation();

   if (_automaticallyEquipOnAddToToolset)
   {
      if (UToolSetComponent* toolsetComponent = _GetOwningToolSet())
      {
         toolsetComponent->EquipToolByClass(GetClass());
      }
   }

   return true;
}
