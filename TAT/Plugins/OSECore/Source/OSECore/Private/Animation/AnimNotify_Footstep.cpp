// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotify_Footstep.h"

// ose
#include "OSECommon.h"
#include "Character/OSEFootstepSimulatorComponent.h"
#include "Character/OSEFootstepSimulatorProviderInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_Footstep)

// ue4

DEFINE_LOG_CATEGORY_STATIC(LogAnimNotifyFootstep, Display, All);

UAnimNotify_Footstep::UAnimNotify_Footstep(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotify_Footstep::GetNotifyName_Implementation() const
{
   return TEXT("Footstep");
}

void UAnimNotify_Footstep::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
    Super::Notify(meshComp, animation, eventReference);

    AActor* owningActor = meshComp->GetOwner();

    if (!IsValid(owningActor))
    {
       // Sometimes, when switching the editor animation Notify will be called on a skeletal mesh component
       // that has no actor and sits in the Engine/Transient package. In this case we're safe to ignore this notify.
       return;
    }

    if (!owningActor->Implements<UOSEFootstepSimulatorProviderInterface>())
    {
       UE_LOG(LogAnimNotifyFootstep, Warning, TEXT("Warning: The actor containing the skeletal mesh with this Footstep AnimNotify doesn't implement OSEFootstepSimulatorComponent. No audio will be played."));
       return;
    }

    IOSEFootstepSimulatorProviderInterface* footstepProvider = CastChecked<IOSEFootstepSimulatorProviderInterface>(owningActor);

    UOSEFootstepSimulatorComponent* footstepComponent = footstepProvider->GetFootstepComponent();

    if (IsValid(footstepComponent))
    {
       footstepComponent->ReceiveAnimationFootstep(StrideTag);
    }
}

