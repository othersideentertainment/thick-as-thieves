// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ose
#include "Audio/AudioEnums.h"

// ue4
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"


#include "AnimNotify_AkPlayerEvent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAkPlayerEvent, Log, All);

class UAkComponent;
class UAkAudioEvent;

//---------------------------------------------------------------------------------------
/// UAnimNotify_AkPlayerEvent
/// 
/// This AnimNotify will post AkAudioEvents to an AkComponent of AkComponentClass on the
/// actor that contains the SkeletalMeshComponent the animation is playing on. This lets
/// sounds easily be configured to play on the various AK components attached to NPCs and
/// Players.
/// 
/// Note: This only finds the first instance of AkComponentClass. It's best to not attach
/// multiple instances of the same AkComponent subclass to your actors.
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "AkPlayerEvent"))
class OSECORE_API UAnimNotify_AkPlayerEvent : public UAnimNotify
{
   GENERATED_BODY()
public:
   UAnimNotify_AkPlayerEvent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   /// Overriden to customize the text shown in the AnimNotify keyframing UI.
   virtual FString GetNotifyName_Implementation() const override;

   /// Gets called when the animation reaches the AnimNotify, we post our AkEvent in here.
   virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

   /// An AkComponent class used to look up the instance of AkComponent on the actor.
   UPROPERTY(EditInstanceOnly, Category = "Audio|OSE")
   EAkComponentType AkComponentType = EAkComponentType::Footsteps;

   /// The AkAudioEvent to post to the AkComponent of type AkComponentClass
   UPROPERTY(EditInstanceOnly, Category = "Audio|OSE")
   UAkAudioEvent* Event = nullptr;

   /// Optional name of a blueprint event to trigger on the AkPlayerComponent
   UPROPERTY(EditInstanceOnly, Category = "Audio|OSE")
   FName BlueprintEvent;

};
