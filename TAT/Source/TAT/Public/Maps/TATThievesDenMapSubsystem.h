// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATThievesDenMapSubsystem.generated.h"

class UTATMapPreviewInfo;
class UTATSaveGame;

// A subsystem that tracks the available maps in the thieves den
//
// Should make it easier to change where the unlocking comes from
//
// NOTE: Since there is only a single actor in the thieves den
//       that cares about this at the moment, I could have just
//       put this in an actor. I did not have a strong reason for
//       erring on the side of a subsystem.
UCLASS()
class TAT_API UTATThievesDenMapSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
   virtual void Deinitialize() override;

   UFUNCTION(BlueprintPure, Category="ThievesDen|Maps")
   const TArray<UTATMapPreviewInfo*>& GetMapPreviews() const { return ObjectPtrDecay(_mapPreviews); }
   
   // Consider: Should this be more-fine-grained, so it doesn't have to loop over hidden ones?
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapPreviewsChanged);
   UPROPERTY(BlueprintAssignable, Category="ThievesDen|Maps")
   FOnMapPreviewsChanged OnMapPreviewsChanged;

private:
   void _InitMapPreviews();
   UFUNCTION()
   void _RefreshMapPreviews();
   
   UPROPERTY(Transient)
   TObjectPtr<UTATSaveGame> _saveGame;

   // For now, assuming index is 1:1 with maps in project settings
   UPROPERTY(Transient)
   TArray<TObjectPtr<UTATMapPreviewInfo>> _mapPreviews;
};
