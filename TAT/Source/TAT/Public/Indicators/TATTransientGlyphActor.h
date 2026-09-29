// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"

// tat
#include "Indicators/TATClientProxyActorInterface.h"

#include "TATTransientGlyphActor.generated.h"

class UTATGlyphComponent;

/// Actor class used for spawning glyph indicators at runtime.
/// Primarily intended as a native base class for thief vision indicator glyphs.
UCLASS(Blueprintable)
class TAT_API ATATTransientGlyphActor : public AActor, public ITATClientProxyActorInterface
{
   GENERATED_BODY()

   ATATTransientGlyphActor();

public:
   // From ITATClientProxyActorInterface
   virtual void OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams) override;
   virtual void OnDestroyClientProxy_Implementation() override;
   virtual void OnClientProxyActorSetVisible_Implementation(bool newVisible) override;
   virtual void OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan) override;

   UTATGlyphComponent* GetGlyphComponent() const { return _glyphComponent; }

protected:
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Glyph")
   UTATGlyphComponent* _glyphComponent = nullptr;
};
