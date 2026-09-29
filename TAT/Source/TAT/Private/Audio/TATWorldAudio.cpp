// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Audio/TATWorldAudio.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldAudio)

void UTATWorldAudio::OnNativeWorldBeginPlay(UWorld* world)
{
   _world = world;
   OnWorldBeginPlay(world);
}

