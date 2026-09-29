// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotify_AkPlayerEvent.h"

// ose
#include "OSECommon.h"
#include "Audio/OSEAkAudioComponentSystemInterface.h"

// ue4
#include "Camera/CameraShakeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_AkPlayerEvent)

#if WITH_EDITOR
#include "AnimationEditorPreviewActor.h"
#endif

DEFINE_LOG_CATEGORY(LogAkPlayerEvent);

//---------------------------------------------------------------------------------------
// UAnimNotify_AkPlayerEvent
//---------------------------------------------------------------------------------------

UAnimNotify_AkPlayerEvent::UAnimNotify_AkPlayerEvent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotify_AkPlayerEvent::GetNotifyName_Implementation() const
{
   return FString::Printf(TEXT("AK:%s: %s"), *UEnum::GetDisplayValueAsText(AkComponentType).ToString(), Event ? *Event->GetFName().ToString() : TEXT("None"));
}

void UAnimNotify_AkPlayerEvent::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::Notify(meshComp, animation, eventReference);

   AActor* owningActor = meshComp->GetOwner();

   if (!IsValid(owningActor))
   {
      // Sometimes, when switching the editor animation Notify will be called on a skeletal mesh component
      // that has no actor and sits in the Engine/Transient package. In this case we're safe to ignore this notify.
      return;
   }

#if WITH_EDITOR
   AAnimationEditorPreviewActor* previewActor = Cast<AAnimationEditorPreviewActor>(owningActor);
   if (previewActor)
   {
      // We know this is notify is coming from the preview window in the editor.
      // So we post the event not to the AkComponent of AkComponentClass but to the actor instead.
      // This will create an AkComponent on the first post, but will be reuse it afterwards.
      if (IsValid(Event))
      {
         constexpr bool stopWhenAttachedObjectDestroyed = true;
         Event->PostOnActor(owningActor, nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr, stopWhenAttachedObjectDestroyed);
      }
      return;
   }
#endif

   if (!owningActor->Implements<UOSEAkAudioComponentSystemInterface>())
   {
      UE_LOG(LogAkPlayerEvent, Warning, TEXT("Warning: The actor containing the skeletal mesh with this AkPlayerEvent AnimNotify doesn't implement OSEAkAudioComponentSystemInterface. No audio will be played."));
      return;
   }

   UAkComponent* playerComponent = IOSEAkAudioComponentSystemInterface::Execute_GetAkComponent(owningActor, AkComponentType);
  
   if (!playerComponent)
   {
      UE_LOG(LogAkPlayerEvent, Warning, TEXT("Warning: AnimNotify_AkPlayerEvent could not find an AkComponent of type %s. No sound will be played."), *UEnum::GetValueAsString(AkComponentType));
      return;
   }
   
   if (IsValid(Event))
   {
      constexpr bool stopWhenAttachedObjectDestroyed = true;
      Event->PostOnComponent(playerComponent, nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr, stopWhenAttachedObjectDestroyed);
   }

   //Call an optional blueprint event.
   if (!BlueprintEvent.IsNone())
   {
      UFunction* eventFunction = playerComponent->FindFunction(BlueprintEvent);
      if (IsValid(eventFunction))
      {
         if (eventFunction->NumParms == 0)
         {
            playerComponent->ProcessEvent(eventFunction, nullptr);
         }
         else
         {
            UE_LOG(LogAkPlayerEvent, Warning, TEXT("Warning: AnimNotify_AkPlayerEvent only supports blueprint events with 0 inputs. %s has %d"), *BlueprintEvent.ToString(), eventFunction->NumParms);
         }
      }
      else
      {
         UE_LOG(LogAkPlayerEvent, Warning, TEXT("Warning: AnimNotify_AkPlayerEvent has BlueprintEvent %s specified but cannot find the event on %s component (Animation %s)"), *BlueprintEvent.ToString(), *UEnum::GetDisplayValueAsText(AkComponentType).ToString(), *animation->GetName());
      }
   }
}

