// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "TATNavLinkOwnerComponent.h"

// ue4
#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"

#include "TATPathFollowingComponent.generated.h"

class UTATNavLinkCustomComponent;

UCLASS(BlueprintType)
class TAT_API UTATPathFollowingComponent : public UPathFollowingComponent
{
   GENERATED_BODY()

   virtual FVector GetMoveFocus(bool bAllowStrafe) const override;
   virtual void UpdateCachedComponents() override;

   virtual bool HasReachedCurrentTarget(const FVector& currentLocation) const override;
   virtual void UpdatePathSegment() override;
   void UnReserveNavLink();
   virtual void OnPathUpdated() override;
   virtual void OnPathFinished(const FPathFollowingResult& result) override;
   virtual void SetMoveSegment(int32 segmentStartIndex) override;
   virtual void OnSegmentFinished() override;
   virtual void FollowPathSegment(float deltaTime) override;

   virtual void BeginPlay() override;

   UFUNCTION()
   void OnOwnerEndPlay(AActor* actor, EEndPlayReason::Type arg);
   virtual void BeginDestroy() override;

public:
   UPROPERTY(EditDefaultsOnly, Category="TAT|Focus")
   float MoveFocusLookForwardAmount = {500.f};

   UPROPERTY(EditDefaultsOnly, Category="TAT|Focus")
   bool bShouldFocusAtEyeHeightWhenMoving = {false};

   virtual FVector TransformGoalLocation(const AActor& goalActor,
                                         const FVector& goalActorLocation,
                                         const FVector& offset) const override;

   // Returns the vector from the AI's feet to the current segment's ending location
   FVector GetRemainingPathSegmentVector() const;

protected:
   UPROPERTY(Transient)
   TObjectPtr<UTATNavLinkOwnerComponent> _futureNavLinkOwner { nullptr };

   uint32 _futureNavLinkSegmentIndex {0};

   UPROPERTY(EditDefaultsOnly)
   uint8 _movementImportance { 0 };
   
private:
   UPROPERTY(Transient)
   TObjectPtr<APawn> OwnerPawn {};
};
