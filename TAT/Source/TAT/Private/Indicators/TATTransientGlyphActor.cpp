// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATTransientGlyphActor.h"

// tat
#include "Indicators/TATGlyphComponent.h"

// ue
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTransientGlyphActor)
DEFINE_LOG_CATEGORY_STATIC(LogTATTransientGlyphActor, Log, All);

ATATTransientGlyphActor::ATATTransientGlyphActor()
{
   _glyphComponent = CreateDefaultSubobject<UTATGlyphComponent>(TEXT("GlyphComponent"));
   SetRootComponent(_glyphComponent);
}

void ATATTransientGlyphActor::OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams)
{
}

void ATATTransientGlyphActor::OnDestroyClientProxy_Implementation()
{
   check(_glyphComponent != nullptr);
   SetLifeSpan(_glyphComponent->GetOpacityFadeSeconds() + 0.25f);
   _glyphComponent->SetGlyphVisibility(false);
}

void ATATTransientGlyphActor::OnClientProxyActorSetVisible_Implementation(bool newVisible)
{
   check(_glyphComponent != nullptr);
   _glyphComponent->SetGlyphVisibility(newVisible);
}

void ATATTransientGlyphActor::OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan)
{
}
