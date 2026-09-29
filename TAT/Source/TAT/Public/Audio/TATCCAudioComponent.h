// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "TATCCAudioComponent.generated.h"


class UAkAudioEvent;

// This scene component is used for infinitely looping fire and forget sounds. 
// 
// Ak Events added here will auto play when the owning actor is spawned into a scene.
// A CCAudioManager will also be spawned into the scene, which holds the transforms of every instance of this component.
// The CCAudioManager associated with this component will then spawn one Ak Component, which will playback at every transform location.
UCLASS(Blueprintable, BlueprintType, Category="TATAudio", ClassGroup = "Audio", meta=(BlueprintSpawnableComponent, DisplayName="TATCarbonCopyAudioComponent"))
class UTATCCAudioComponent : public USceneComponent
{
   GENERATED_BODY()

public:
   UTATCCAudioComponent();

   virtual void OnUnregister() override;
   virtual void Activate(bool reset=false) override;
   virtual void Deactivate() override;
   virtual bool NeedsLoadForServer() const override;

   UFUNCTION(BlueprintCallable)
   void SetLoopingAudioEvent(UAkAudioEvent* event);

#if WITH_EDITOR
   UFUNCTION(CallInEditor, Category = Debug)
   void DebugDrawAllOfThisClass();
#endif

protected:
   void _Register();
   void _Unregister();

   // Audio to play
   UPROPERTY(BlueprintReadonly, EditAnywhere, Category="Default")
   TObjectPtr<UAkAudioEvent> AkLoopingAudio;

#if WITH_EDITORONLY_DATA
   // Choose to use the attenuation radius of a valid ak event. 
   // Uses the Custom Radius if false.
   // If the ak event is invalid, then the Custom Radius will be used instead
   UPROPERTY(EditAnywhere, Category="Debug")
   bool bUseAttenuationRadius = false;

   UPROPERTY(EditAnywhere, Category="Debug", meta = (EditCondition = "!bUseAttenuationRadius"))
   float CustomRadius = 200;

   UPROPERTY(EditAnywhere, Category="Debug")
   float DebugDrawDuration = 3;
#endif
   
private:
   // TODO: replace with handle
   UPROPERTY(Transient)
   FTransform _registeredWorldTransform;
};
