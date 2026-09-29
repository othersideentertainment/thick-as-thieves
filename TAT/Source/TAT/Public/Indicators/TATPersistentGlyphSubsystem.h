// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Containers/Set.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATPersistentGlyphSubsystem.generated.h"

class UTATPersistentGlyphComponent;

///
/// Subsystem to manage persistent glyphs in the world.
/// Persistent glyph components (UTATPersistentGlyphComponent) register themselves with this subsystem so it can update visibility based on
/// the distance from the player.
///
UCLASS()
class TAT_API UTATPersistentGlyphSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

   // From USubsystem
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   // From UTickableWorldSubsystem
   virtual ETickableTickType GetTickableTickType() const override;

   // From UObject
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATPersistentGlyphSubsystem, STATGROUP_Tickables); }

public:
   void RegisterIndicator(UTATPersistentGlyphComponent* indicator);
   void UnregisterIndicator(UTATPersistentGlyphComponent* indicator);

private:
   UPROPERTY(Transient)
   TSet<UTATPersistentGlyphComponent*> _glyphs;
};
