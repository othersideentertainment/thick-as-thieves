// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Audio/TATCCAudioComponent.h"

// tat
#include "Audio/TATCCAudioSubsystem.h"

// wwise
#include "AkAudioEvent.h"

// ue
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCCAudioComponent)

UTATCCAudioComponent::UTATCCAudioComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   bAutoActivate = true;
}

void UTATCCAudioComponent::OnUnregister()
{
   // auto-activate happens in OnRegister, so need to do this here in case never begun play
   if (IsActive() && !GetWorld()->bIsTearingDown)
   {
      _Unregister();
   }

   Super::OnUnregister();
}

void UTATCCAudioComponent::Activate(bool reset/*=false*/)
{
   if (IsNetMode(NM_DedicatedServer) || !GetWorld()->IsGameWorld() || AkLoopingAudio == nullptr)
   {
      return;
   }

   if (reset || ShouldActivate())
   {
      if (reset && IsActive())
      {
         _Unregister();
      }

      _Register();

      Super::Activate(reset);
   }
}

void UTATCCAudioComponent::Deactivate()
{
   if (!ShouldActivate())
   {
      _Unregister();
      Super::Deactivate();
   }
}

bool UTATCCAudioComponent::NeedsLoadForServer() const
{
   // can be stripped from dedicated servers
   return false;
}

void UTATCCAudioComponent::SetLoopingAudioEvent(UAkAudioEvent* event)
{
   if (event == AkLoopingAudio)
   {
      return;
   }

   if (IsActive() && AkLoopingAudio)
   {
      _Unregister();
   }

   AkLoopingAudio = event;

   if (IsActive())
   {
      if (AkLoopingAudio)
      {
         _Register();
      }
      else
      {
         // for now implicitly deactivate if set to nothing?
         Deactivate();
      }
   }
}

#if WITH_EDITOR
void UTATCCAudioComponent::DebugDrawAllOfThisClass()
{
#if ENABLE_DRAW_DEBUG
   UWorld* world = GetWorld();
   if (!world)
   {
      return;
   }


   FlushPersistentDebugLines(world);
   
   for (const AActor* actor : TActorRange<AActor>(world, GetOwner()->GetClass()))
   {
      TInlineComponentArray<UTATCCAudioComponent*> components;
      actor->GetComponents(components);
      for (const UTATCCAudioComponent* audioComponent : components)
      {
         // NOTE: explicitly using debug params on current component to match BP behavior, as they were probably used as a scratch
         constexpr int segments = 12;
         const FColor color = audioComponent->AkLoopingAudio ? FColor::Green : FColor::Red;
         const float radius = audioComponent->AkLoopingAudio && !bUseAttenuationRadius ? CustomRadius : audioComponent->AkLoopingAudio->MaxAttenuationRadius;

         DrawDebugSphere(world, audioComponent->GetComponentLocation(), radius, segments, color, false, DebugDrawDuration);
      }
   }
#endif
}
#endif

void UTATCCAudioComponent::_Register()
{
   if (UTATCCAudioSubsystem* audioSubsystem = GetWorld()->GetSubsystem<UTATCCAudioSubsystem>())
   {
      _registeredWorldTransform = GetComponentToWorld();
      audioSubsystem->AddSoundAtTransform(AkLoopingAudio, _registeredWorldTransform);
   }
}

void UTATCCAudioComponent::_Unregister()
{
   if (AkLoopingAudio == nullptr)
   {
      return;
   }

   if (UTATCCAudioSubsystem* audioSubsystem = GetWorld()->GetSubsystem<UTATCCAudioSubsystem>())
   {
      audioSubsystem->RemoveSoundAtTransform(AkLoopingAudio, _registeredWorldTransform);
   }
}
