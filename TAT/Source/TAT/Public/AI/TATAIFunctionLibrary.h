// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat 
#include "AI/TATKnowledgeComponent.h"

// ose

// ue4
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATAIFunctionLibrary.generated.h"

class UNavLinkCustomComponent;
struct FSmartObjectRequestResult;

UCLASS()
class TAT_API UTATAIFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // C++-facing

   // returns the number of players iterated on
   static int AuthorityForEachPlayerKnowledge(const AActor* actor, const TFunctionRef<void(const FTATActorKnowledge&, const APawn&)>& cb);
   // returns the number of enemies iterated on
   static int AuthorityForEachEnemyKnowledge(const AActor* actor, const TFunctionRef<void(const FTATActorKnowledge&)>& cb);

   // Blueprint-facing
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   static bool IsBamboozled(const AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   static bool CancelBamboozledEffects(const AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   static const APawn* AuthorityFindClosestVisibleDetectedPlayerPawn(const AActor* actor);

   UFUNCTION(BlueprintCallable)
   static void FindNavLinkStartingPoints(const TArray<UNavLinkCustomComponent*>& navLinkComponents, TArray<FVector>& outStartingPoints);

   UFUNCTION(BlueprintCallable)
   static void SetNavLinkEnabled(UNavLinkCustomComponent* navlink, bool enabled);
   UFUNCTION(BlueprintPure)
   static bool GetNavLinkEnabled(const UNavLinkCustomComponent* navlink);

   UFUNCTION(BlueprintCallable)
   static FVector FindPositionOfInteractableComponents(AActor* interactableActor);

   // Because FTATActorKnowledge is a struct, I can't create UFUNCTION()'s on it. Which leads to this kind of library.
   // I don't think it's a blocker, but if _feels_ weird to do it this way.
   // Some behaviors need to be able to react to changes in the knowledge of actor, which are implemented in blueprint.
   // Perhaps this could be done in some sort of gameplay task that the behavior runs? 
   UFUNCTION(BlueprintCallable)
   static bool IsKnowledgeAboutActor(const FTATActorKnowledge& knowledge, const AActor* actor);
   UFUNCTION(BlueprintCallable)
   static EActorDetectionState GetKnowledgeDetectionState(const FTATActorKnowledge& knowledge);

   UFUNCTION(BlueprintCallable)
   static void LockAIResourcesOnPawn(APawn* pawn, bool bLockMovement, bool bLockLogic);
   UFUNCTION(BlueprintCallable)
   static void UnlockAIResourcesOnPawn(APawn* pawn, bool bLockMovement, bool bLockLogic);
   UFUNCTION(BlueprintCallable)
   static void GetAIResourceLockStates(APawn* pawn, bool& isMovementLocked, bool& isLogicLocked);

   UFUNCTION(BlueprintCallable)
   static bool FindClosestSmartObjectSlotToActor(const AActor* queryingActor, const TArray<FSmartObjectRequestResult>& smartObjectSlots,
       FSmartObjectRequestResult& outClosestSlot);
};
