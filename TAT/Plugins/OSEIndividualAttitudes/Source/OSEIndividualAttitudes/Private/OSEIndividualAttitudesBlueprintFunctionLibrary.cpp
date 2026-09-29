// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "OSEIndividualAttitudesBlueprintFunctionLibrary.h"

// ose
#include "OSEIndividualAttitudeInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEIndividualAttitudesBlueprintFunctionLibrary)

UOSEIndividualAttitudeComponent* UOSEIndividualAttitudesBlueprintFunctionLibrary::GetIndividualAttitudeComponent(
   const AActor* actor)
{
   const IOSEIndividualAttitudeInterface* attitudeInterface = Cast<IOSEIndividualAttitudeInterface>(actor);
   if(attitudeInterface == nullptr)
      return nullptr;
   UOSEIndividualAttitudeComponent* attitudeComponent = attitudeInterface->GetAttitudeComponent();
   return attitudeComponent;
}

void UOSEIndividualAttitudesBlueprintFunctionLibrary::SetIndividualAttitude(AActor* sourceActor,
                                                                            AActor* targetActor,
                                                                            const EOSEIndividualAttitude attitude,
                                                                            const bool shouldExpire,
                                                                            const float maxAge)
{
   UOSEIndividualAttitudeComponent* attitudeComponent = GetIndividualAttitudeComponent(sourceActor);
   if(attitudeComponent == nullptr)
      return;
   attitudeComponent->SetAttitudeTowardsActor(targetActor, attitude, shouldExpire ? maxAge : INDEX_NONE);
}

void UOSEIndividualAttitudesBlueprintFunctionLibrary::ClearIndividualAttitude(const AActor* sourceActor,
                                                                              AActor* targetActor)
{
   UOSEIndividualAttitudeComponent* attitudeComponent = GetIndividualAttitudeComponent(sourceActor);
   if(attitudeComponent == nullptr)
      return;
   attitudeComponent->ClearAttitudeTowardsActor(targetActor);
}

void UOSEIndividualAttitudesBlueprintFunctionLibrary::ShareAllIndividualAttitudesWithTarget(AActor* sourceActor,
   AActor* targetActor,
   EOSEAttitudeCopyRules attitudeCopyRules,
   EOSEExpirationTimeCopyRules expirationTimeCopyRules)
{
   UOSEIndividualAttitudeComponent* sourceAttitudeComponent = GetIndividualAttitudeComponent(sourceActor);
   UOSEIndividualAttitudeComponent* targetAttitudeComponent = GetIndividualAttitudeComponent(targetActor);

   if(sourceAttitudeComponent == nullptr || targetAttitudeComponent == nullptr)
      return;
   sourceAttitudeComponent->ShareAllIndividualAttitudes(
      targetAttitudeComponent,
      attitudeCopyRules,
      expirationTimeCopyRules
   );
}
