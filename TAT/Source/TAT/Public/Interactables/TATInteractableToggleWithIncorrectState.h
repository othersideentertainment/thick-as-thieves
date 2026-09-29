// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"
#include "Interactables/TATInteractableToggle.h"

#include "TATInteractableToggleWithIncorrectState.generated.h"

class UTATActionNodeComponent_IncorrectObjectState;
class UTATSmartObjectComponent;
struct FTATAuthorityBreakContext;

UCLASS()
class TAT_API ATATInteractableToggleWithIncorrectState : public ATATInteractableToggle
   , public ITATAIIncorrectObjectStateInterface
   , public ITATSmartObjectTagInterface
   , public ITATSmartObjectOwnerInterface
{
	GENERATED_BODY()

   ATATInteractableToggleWithIncorrectState();

   // from AActor
   virtual void PostInitializeComponents() override;

   // from ITATUtilityAITargetingGroupInterface (via ATATInteractableToggle)
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;

   // from ITATAIIncorrectObjectStateInterface
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const override;

   // from ITATSmartObjectTagInterface
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override;

   // from ITATSmartObjectOwnerInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override;

protected:
   // from AOSESyncedToggle
   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent) override;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|AI", BlueprintReadOnly)
   TObjectPtr<UTATActionNodeComponent_IncorrectObjectState> IncorrectStateActionNodeComponent = nullptr;
	
};
