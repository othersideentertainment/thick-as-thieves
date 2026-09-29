// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/CollisionProfile.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATItemFunctionLibrary.generated.h"

class UTATItemInventoryComponent;
class ACharacter;
class UToolComponent;

UCLASS()
class TAT_API UTATItemFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Tools")
   static bool IsWeaponTool(const UToolComponent* item);

   UFUNCTION(BlueprintPure, Category = "Tools")
   static bool DoesActorHaveWeaponEquipped(const AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "Items|OSE")
   static bool FindDropLocationFromSuggestedStart(const AActor* actorToDrop, const AActor* droppingActor, FCollisionProfileName traceProfile, FVector suggestedStartWorldPosition, FVector& outDropPosition);

   // basically just traces in the view direction
   static FVector FindSuggestedDropStartFromActorEyes(const AActor* droppingActor, float horizontalDropDistance, FCollisionProfileName traceProfile);

   // Is the given character allowed to pick up things? (but there may be more restrictions depending on the type of thing)
   static bool CanCharacterPickUpThings(const ACharacter* character);

   static void DropItemsForKO(UTATItemInventoryComponent* itemInventoryComponent);

private:
   static void _GetActorBoundingBox(const AActor* actor, FVector& center, FVector &extents);
};
