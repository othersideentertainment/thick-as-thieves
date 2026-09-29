// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Audio/TATAudioWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAudioWorldSubsystem)

UTATAudioWorldSubsystem::UTATAudioWorldSubsystem()
   : _worldAudioClass(UTATWorldAudio::StaticClass())
{

}

void UTATAudioWorldSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   UWorld* world = GetWorld();
   check(world);
}
 
void UTATAudioWorldSubsystem::Deinitialize()
{
   Super::Deinitialize();

   if (_worldAudio)
   {
      _worldAudio->CleanUp();
      _worldAudio = nullptr;
   }
}

void UTATAudioWorldSubsystem::OnWorldBeginPlay(UWorld& world)
{
   Super::OnWorldBeginPlay(world);

   // If we haven't already loaded in our world audio class, do that here
   // TODO: Can this be an async load instead?
   if (!_loadedWorldAudioClass)
   {
      _loadedWorldAudioClass  = _worldAudioClass.LoadSynchronous();
   }

   if (_loadedWorldAudioClass)
   {
      _worldAudio = NewObject<UTATWorldAudio>(this, _loadedWorldAudioClass);
      if (_worldAudio)
      {
         _worldAudio->OnNativeWorldBeginPlay(&world);
      }
   }
}

bool UTATAudioWorldSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_DedicatedServer);
}
