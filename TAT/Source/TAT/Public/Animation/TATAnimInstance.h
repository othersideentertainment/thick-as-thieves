// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"

// OSE
#include "Animation/Graph/OSEAnimInstance.h"
#include "Animation/Graph/OSEAnimAIAnimData.h"

// TAT
#include "TATAnimAIData.h"
#include "Animation/TATAnimData.h"
#include "TATAnimInstance.generated.h"


//--------------------------------------------------------------------------------------------------
/// TAT-specific animation instance graph. This is a baseclass for optimized animation blueprints,
/// avoiding any costly blueprint thunks.
//--------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class TAT_API UTATAnimInstance : public UOSEAnimInstance
{
   GENERATED_BODY()

public:

   UTATAnimInstance();

   void SetAnimNotifiesEnabled(bool newEnableAnimNotifies) { _enableAnimNotifies = newEnableAnimNotifies; }
   bool GetAnimNotifiesEnabled() const { return _enableAnimNotifies; }

protected:

   // From UAnimInstance
   /// Overriding to allocate our custom proxy object
   virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
   /// Intercept anim notify events
   virtual bool HandleNotify(const FAnimNotifyEvent& animNotifyEvent) override;
   /// Intercept anim notify state events
   virtual bool ShouldTriggerAnimNotifyState(const UAnimNotifyState* animNotifyState) const;

protected:

   virtual void PerformInitialize() override;

   virtual void PerformDataCopy(const UOSEAnimInstance* srcInstance) override;

   virtual void PerformDataUpdate() override;

protected:

   /// The ability data for the current state
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AbilityData);
   FTATAnimAbilityData CurrentAbilityData;

   /// The ability data from the previous update (usually the previous frame)
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AbilityData, AdvancedDisplay);
   FTATAnimAbilityData PreviousAbilityData;

   /// The ability data from the previous update (usually the previous frame)
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AIData, AdvancedDisplay);
   FTATAIAnimData AIData;

   // Thread-safe accessor for whether the main anim instance is playing any montages
   UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = AIData, AdvancedDisplay)
   bool CachedPlayingAnyMontage = false;
private:
   bool _enableAnimNotifies = true;
};
