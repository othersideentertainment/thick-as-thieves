// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATPersistentGlyphSubsystem.h"

// tat
#include "Indicators/TATPersistentGlyphComponent.h"

// ue
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPersistentGlyphSubsystem)
DEFINE_LOG_CATEGORY_STATIC(LogTATPersistentGlyphSubsystem, Log, All);

bool UTATPersistentGlyphSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   if (const UWorld* world = outer->GetWorld())
   {
      // Game world only
      if (world->IsGameWorld())
      {
         return true;
      }
   }

   return false;
}

ETickableTickType UTATPersistentGlyphSubsystem::GetTickableTickType() const
{
   // Don't tick on dedicated server
   if (const UWorld* world = GetWorld())
   {
      if (world->IsNetMode(NM_DedicatedServer))
      {
         return ETickableTickType::Never;
      }
   }
   return Super::GetTickableTickType();
}

void UTATPersistentGlyphSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   check(!GetWorld()->IsNetMode(NM_DedicatedServer));

   const APlayerController* localPlayer = UGameplayStatics::GetPlayerController(this, 0);
   if (!IsValid(localPlayer))
   {
      return;
   }
   if (!localPlayer->IsLocalController())
   {
      return;
   }

   FVector actorEyesLocation;
   FRotator actorEyesRotation;
   localPlayer->GetActorEyesViewPoint(actorEyesLocation, actorEyesRotation);

   for (UTATPersistentGlyphComponent* indicator : _glyphs)
   {
      if (IsValid(indicator))
      {
         indicator->RefreshVisibilityForLocalPlayer(actorEyesLocation);
      }
      else
      {
         UE_LOG(LogTATPersistentGlyphSubsystem, Error, TEXT("Invalid UTATPersistentGlyphComponent detected in Tick()!"));
      }
   }
}

void UTATPersistentGlyphSubsystem::RegisterIndicator(UTATPersistentGlyphComponent* indicator)
{
   if (!IsValid(indicator))
   {
      return;
   }
   _glyphs.Add(indicator);
}

void UTATPersistentGlyphSubsystem::UnregisterIndicator(UTATPersistentGlyphComponent* indicator)
{
   _glyphs.Remove(indicator);
}
