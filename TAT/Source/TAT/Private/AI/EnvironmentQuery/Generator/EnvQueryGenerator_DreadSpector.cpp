// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/EnvironmentQuery/Generator/EnvQueryGenerator_DreadSpector.h"

#include "AI/TATAIController.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Loot/TATLootInventory.h"
#include "Player/TATPlayerController.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_DreadSpector)

#define LOCTEXT_NAMESPACE "EnvQueryGenerator_DreadSpector"

UEnvQueryGenerator_DreadSpector::UEnvQueryGenerator_DreadSpector(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();
}  


void UEnvQueryGenerator_DreadSpector::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   UObject* queryOwner = queryInstance.Owner.Get();
   if (queryOwner == nullptr)
      return;

   const ATATAIController* aiController = Cast<ATATAIController>(queryOwner);
   if(aiController == nullptr)
      return;
   
   for (FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      APlayerController* playerActor = iterator->Get();
      if(const ATATPlayerController* asTatPlayerController = Cast<ATATPlayerController>(playerActor))
      {
         APawn* pawn = asTatPlayerController->GetPawn();
         const ITATLootInventoryInterface* inventoryInterface = Cast<ITATLootInventoryInterface>(pawn);
         if(inventoryInterface == nullptr)
            continue;

         if(const UTATLootInventoryComponent* lootInventoryComponent = inventoryInterface->GetLootInventoryComponent())
         {
            if(lootInventoryComponent->HasMajorLoot())
            {
               queryInstance.AddItemData<UEnvQueryItemType_Actor>(pawn);
            }
         }
      }
   }
}


FText UEnvQueryGenerator_DreadSpector::GetDescriptionTitle() const
{
   return LOCTEXT("UEnvQueryGenerator_DreadSpector_Title", "Generate valid items for the dread spector");
}
#undef LOCTEXT_NAMESPACE
