// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Engine/NavigationObjectBase.h"

#include "TATAIContextualLocation.generated.h"

class ATATCharacterAIBase;

// Designer-placed actor defining a location where some AI behavior can take place.
UCLASS(Abstract, Blueprintable)
class TAT_API ATATAIContextualLocation : public ANavigationObjectBase
{
   GENERATED_BODY()

public:
   ATATAIContextualLocation(const FObjectInitializer& objectInitializer);

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UFUNCTION(BlueprintCallable)
   FORCEINLINE FGameplayTag GetLocationType() const { return _locationType; }

   UFUNCTION(BlueprintCallable)
   bool TryClaim(ATATCharacterAIBase* aiCharacter);

   UFUNCTION(BlueprintCallable)
   bool TryRelease(ATATCharacterAIBase* aiCharacter);

   UFUNCTION(BlueprintCallable)
   bool SetIsInUse(bool isInUse, const ATATCharacterAIBase* usingAICharacter, bool& isInUseStateChanged);

   UFUNCTION(BlueprintCallable)
   FORCEINLINE bool IsClaimed() const { return _claimedByAICharacter.IsValid(); }

   UFUNCTION(BlueprintCallable)
   FORCEINLINE bool IsInUse() const { return _isInUse; }

   FORCEINLINE void AuthoritySetPrivateZoneTag(const FGameplayTag privateZoneTag) { _privateZoneTag = privateZoneTag; }
   FORCEINLINE FGameplayTag AuthorityGetPrivateZoneTag() const { return _privateZoneTag; };

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void OnIsInUseStateChanged(bool isInUse);

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.ContextualLocation"))
   FGameplayTag _locationType;

private:
   UFUNCTION()
   void _OnClaimingAIDestroyed(AActor* actor);

#if WITH_EDITORONLY_DATA
   /** Arrow component to indicate forward direction of start. */
   UPROPERTY()
   TObjectPtr<class UArrowComponent> _arrowComponent;
#endif

   // If non-null, the AI character currently using this location.
   TWeakObjectPtr<const ATATCharacterAIBase> _claimedByAICharacter;

   // An indicator that the claiming AI has arrived at this contextual location.
   // Set via SetIsInUse, should be set to true upon arriving and false upon leaving.
   bool _isInUse = false;

   // If non-null, describes the private zone this location is within.
   FGameplayTag _privateZoneTag;
	
};
