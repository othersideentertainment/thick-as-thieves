// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Generator/TATEnvQueryGenerator_ContextualLocations.h"

// tat
#include "AI/Environment/TATAIContextualLocationSubsystem.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"

// ue
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnvQueryGenerator_ContextualLocations)
DEFINE_LOG_CATEGORY_STATIC(LogTATEnvQueryGenerator_ContextualLocations, Log, All);

#define LOCTEXT_NAMESPACE "TATEnvQueryGenerator_ContextualLocations"

UTATEnvQueryGenerator_ContextualLocations::UTATEnvQueryGenerator_ContextualLocations(
   const FObjectInitializer& objectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();
}

void UTATEnvQueryGenerator_ContextualLocations::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   UObject* queryOwner = queryInstance.Owner.Get();
   if (queryOwner == nullptr)
   {
      UE_LOG(LogTATEnvQueryGenerator_ContextualLocations, Error, TEXT("ContextualLocations generator failed to find the \
            query owner!"));
      return;
   }

   const UWorld* world = GEngine->GetWorldFromContextObject(queryOwner, EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
   {
      UE_LOG(LogTATEnvQueryGenerator_ContextualLocations, Error, TEXT("ContextualLocations generator failed to retrieve the \
            world context!"));
      return;
   }

   // Can't search for locations if a type isn't set.
   if (!LocationType.IsValid())
   {
      UE_LOG(LogTATEnvQueryGenerator_ContextualLocations, Error, TEXT("ContextualLocations generator executed with an empty \
            LocationType!"));
      return;
   }

   const UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent = nullptr;

   UObject* queryOwnerTarget = queryOwner;
   if(const AController* queryOwnerAsController = Cast<AController>(queryOwner))
   {
      queryOwnerTarget = queryOwnerAsController->GetPawn();
   }
   
   
   if (ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(queryOwnerTarget))
   {
      privateSpaceCharacterComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent();
   }

   if (const UTATAIContextualLocationSubsystem* contextualLocationSubsystem = world->GetSubsystem<UTATAIContextualLocationSubsystem>())
   {
      if (const FTATAIContextualLocationSet* locationSet = contextualLocationSubsystem->GetLocationSetForType(LocationType))
      {
         for (const ATATAIContextualLocation* location : locationSet->Locations)
         {
            check(location);
            if (!ConsiderUnclaimedLocationsOnly || !location->IsClaimed())
            {
               if (privateSpaceCharacterComponent != nullptr)
               {
                  const FGameplayTag locationPrivateTag = location->AuthorityGetPrivateZoneTag();
                  if (locationPrivateTag.IsValid() 
                     && !privateSpaceCharacterComponent->AuthorityIsAllowedInPrivateZone(locationPrivateTag))
                  {
                     // If we aren't allowed to use the location within a private zone, skip over it.
                     continue;
                  }
               }

               queryInstance.AddItemData<UEnvQueryItemType_Actor>(location);
            }
         }
      }
      else
      {
         UE_LOG(LogTATEnvQueryGenerator_ContextualLocations, Error, TEXT("ContextualLocations generator failed to find any locations \
            of type %s!"), *LocationType.ToString());
      }
   }
   else
   {
      UE_LOG(LogTATEnvQueryGenerator_ContextualLocations, Error, TEXT("ContextualLocations generator failed to retrieve the \
            AI contextual location subsystem!"));
   }
}

FText UTATEnvQueryGenerator_ContextualLocations::GetDescriptionTitle() const
{
   static constexpr const TCHAR* prefix = TEXT("AI.ContextualLocation.");

   FString locationTypeString = LocationType.ToString();
   locationTypeString.RemoveFromStart(prefix);

   return FText::Format(
      LOCTEXT("TATEnvQueryGenerator_ContextualLocations_Title", "Find all '{0}' contextual locations in the world"),
      FText::FromString(locationTypeString));
}

#undef LOCTEXT_NAMESPACE
