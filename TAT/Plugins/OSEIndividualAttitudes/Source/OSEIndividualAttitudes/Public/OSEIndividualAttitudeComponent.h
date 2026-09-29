// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "IndividualAttitudeTypes.h"

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "OSEIndividualAttitudeComponent.generated.h"

class FGameplayDebuggerCategory;
struct FVisualLogEntry;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnIndividualAttitudeChangedForActor, const AActor*)

UCLASS(ClassGroup="OSE", meta=(BlueprintSpawnableComponent))
class OSEINDIVIDUALATTITUDES_API UOSEIndividualAttitudeComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEIndividualAttitudeComponent();
   virtual EOSEIndividualAttitude GetAttitudeTowardsActor(const AActor* target);

   // Sets an attitude towards a specific actor, optionally with an expiration time in seconds.
   virtual void SetAttitudeTowardsActor(AActor* target,
                                        EOSEIndividualAttitude individualAttitude,
                                        float timeUntilExpiration = INDEX_NONE,
                                        bool triggerEvent = true);

   // Clears any attitues set towards a specific actor
   virtual void ClearAttitudeTowardsActor(AActor* target,
                                          bool triggerEvent = true);

   // This ticks once a second to remove the expired attitudes
   virtual void TickComponent(float deltaTime,
                              ELevelTick tickType,
                              FActorComponentTickFunction* thisTickFunction) override;

   virtual void InformTargetOfAttitudeChange(AActor* target, EOSEIndividualAttitude attitude);

   // Share all individual attitudes with the target component 
   virtual void ShareAllIndividualAttitudes(
      UOSEIndividualAttitudeComponent* attitudeComponent,
      const EOSEAttitudeCopyRules& attitudeCopyRules = EOSEAttitudeCopyRules::Overwrite,
      const EOSEExpirationTimeCopyRules& expirationTimeCopyRules = EOSEExpirationTimeCopyRules::Overwrite,
      bool triggerEvent = true);
   
   // Share a specific target individual attitudes with the target component 
   virtual bool ShareSpecificIndividualAttitudeWithTarget(
      AActor* target,
      UOSEIndividualAttitudeComponent* attitudeComponent,
      const EOSEAttitudeCopyRules& attitudeCopyRules = EOSEAttitudeCopyRules::Overwrite,
      const EOSEExpirationTimeCopyRules& expirationTimeCopyRules = EOSEExpirationTimeCopyRules::Overwrite,
      bool triggerEvent = true);

   
#if ENABLE_VISUAL_LOG
   virtual void DescribeSelfToVisLog(FVisualLogEntry* snapshot) const;
#endif
#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* debuggerCategory); 
#endif

   void BroadcastAttitudeChangeFromSharing(UOSEIndividualAttitudeComponent* source);

   FOnIndividualAttitudeChangedForActor OnIndividualAttitudeChangedForActor; 
protected:
   FIndividualAttitude& FindOrAddAttitude(AActor* target);
   FIndividualAttitude* FindAttitude(const AActor* target);
   bool ClearAttitude(const AActor* target);

   void BroadcastAttitudeChangeSingleTarget(const AActor* target);
private:

   UPROPERTY(Transient)
   TArray<FIndividualAttitude> _individualAttitudes;
};
