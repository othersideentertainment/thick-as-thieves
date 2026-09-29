// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATActorDerivedSeed.h"

// tat
#include "Online/TATGameState.h"

// ue5
#include "Misc/Fnv.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActorDerivedSeed)

DEFINE_LOG_CATEGORY_STATIC(LogTATActorDerivedSeed, Log, All);



int32 TATActorDerivedSeed::GetDerivedSeedForActor(const AActor* actor, const TCHAR* context)
{
   const int32 mapSeed = GetMapSeed(actor);
   if(mapSeed == 0)
   {
      return 0;
   }

   // Assumption: everything is little endian these days, so not worried about byte order
   auto fnvRef = [](const auto& ref, uint32 fnv) -> uint32 {
      return FFnv::MemFnv32(&ref, sizeof(ref), fnv);
   };
   auto fnvStr = [](FStringView string, uint32 fnv) -> uint32 {
      static_assert(sizeof(TCHAR) == 2);
      static_assert(PLATFORM_LITTLE_ENDIAN, "assuming little endian");
      return FFnv::MemFnv32(string.GetData(), string.Len() * sizeof(TCHAR), fnv);
   };

   uint32 hash = fnvRef(mapSeed, 0);

   if (actor->IsNameStableForNetworking())
   {
      // If the actor's name is stable for networking (e.g. placed in the level), then that name will be consistent on client and server
      TStringBuilder<128> nameBuilder;
      actor->GetFName().ToString(nameBuilder);
      hash = fnvStr(nameBuilder.ToView(), hash);
   }
   else
   {
      // Otherwise fall back to the location. If there are moving+dynamically spawned actor that want to use this, then the
      // server can just replicate a random number anyways, but that can be added when the need arises
      UE_CLOG(actor->GetRootComponent() != nullptr && actor->GetRootComponent()->Mobility == EComponentMobility::Movable,
         LogTATActorDerivedSeed, Warning, TEXT("ActorDerivedSeed(%s): Dynamically spawned actor %s has a movable root. This is used to derive a random seed, so if it actually moves, that might be inconsistent"),
         context ? context : TEXT("NoContext"), *actor->GetActorNameOrLabel());

      // The coarsest net quantization of actor locations is rounded to the nearest integer, so match that logic
      auto roundFloatToInt = [](double f) -> int64 {
         return int64(f + FPlatformMath::Sign(f) * 0.5);
      };
      const FVector actorLocation = actor->GetActorLocation();
      hash = fnvRef(roundFloatToInt(actorLocation.X), hash);
      hash = fnvRef(roundFloatToInt(actorLocation.Y), hash);
      hash = fnvRef(roundFloatToInt(actorLocation.Z), hash);
   }

   if (context)
   {
      hash = fnvStr(FStringView(context), hash);
   }

   UE_LOG(LogTATActorDerivedSeed, Verbose, TEXT("ActorDerivedSeed: Actor=%s Context=%s is %d"),
      *actor->GetActorNameOrLabel(), context ? context : TEXT("NoContext"), hash);
   return static_cast<int32>(hash);
}

int32 TATActorDerivedSeed::GetMapSeed(const UObject* worldContext)
{
   if (!ensure(worldContext))
   {
      return 0;
   }

   // NB: begin play is not called on actors by default until after the game state
   //     is replicated. So this should be safe as long as it isn't called before a
   //     BeginPlay. But there should be ensures to keep that noisy if that changes.
   ATATGameState* gameState = worldContext->GetWorld()->GetGameState<ATATGameState>();
   if(!ensure(gameState))
   {
      return 0;
   }

   return gameState->GetMapSeed();
}

FRandomStream UTATDerivedSeedFunctionLibrary::GetDerivedRandomStreamForActor(const AActor* actor, FName context)
{
   if (!context.IsNone())
   {
      TStringBuilder<128> nameBuilder;
      context.ToString(nameBuilder);
      // TODO: it would be nice to pass a FStringView directly to the function, so it doesn't need to do another strlen, but that gets awkward for logging
      return TATActorDerivedSeed::GetDerivedSeedForActor(actor, *nameBuilder);
   }
   else
   {
      return TATActorDerivedSeed::GetDerivedSeedForActor(actor, nullptr);
   }
}
