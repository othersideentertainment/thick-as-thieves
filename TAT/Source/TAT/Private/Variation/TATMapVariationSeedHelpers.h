// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Hash/xxhash.h"

namespace SeedHelpers {

   constexpr int32 CombineSeed(int32 a, int32 b)
   {
      // constants stolen from PCGHelpers.cpp
      return ((a * 196314165U) + 907633515U) ^ ((b * 73148459U) + 453816763U);
   }

   // possibly move to cpp?
   inline int32 MakeSeedForName(FName name)
   {
      TUtf8StringBuilder<256> stringBuilder;
      name.AppendString(stringBuilder);

      const int32 hash = static_cast<int32>(FXxHash64::HashBuffer(stringBuilder.GetData(), stringBuilder.Len() * sizeof(*stringBuilder.GetData())).Hash & 0xFFFFFFFF);
      return hash;
   }

   inline int32 MakeSeedForName(FName name, int32 initialSeed)
   {
      const int32 hash = MakeSeedForName(name);
      return CombineSeed(hash, initialSeed);
   }

   // Makes hash for a component that might be used in multiple level instances
   inline int32 MakeHashForComponent(const UActorComponent* spawner)
   {
      TStringBuilder<256> pathBuilder;

      const AActor* owner = spawner->GetOwner();
      check(owner);

      // Include world name because of level instances
      const UObject* world = owner->GetTypedOuter(UWorld::StaticClass());
      if (ensure(world))
      {
         FName packageName = world->GetOuter()->GetFName();
         packageName.SetNumber(0); //< Level instances use a different suffix (0,1) for game worlds, clear so they have the same value
         packageName.AppendString(pathBuilder);
         pathBuilder.AppendChar('.');
      }

      owner->GetFName().AppendString(pathBuilder);
      pathBuilder.AppendChar('.');
      spawner->GetFName().AppendString(pathBuilder);

#if WITH_EDITOR
      if (spawner->GetWorld()->IsPlayInEditor())
      {
         pathBuilder = UWorld::RemovePIEPrefix(pathBuilder.ToString());
      }
#endif

      static_assert(PLATFORM_LITTLE_ENDIAN, "assuming little endian");
      static_assert(sizeof(TCHAR) == 2);
      return static_cast<int32>(FXxHash64::HashBuffer(pathBuilder.GetData(), pathBuilder.Len() * sizeof(TCHAR)).Hash & 0xFFFFFFFF);
   }

   // Just a trivial helper for sorting
   template <typename  T>
   struct TSpawnerWithHash
   {
      TSpawnerWithHash(T* spawner, int32 hash)
      : Spawner(spawner), Hash(hash)
      {}

      T* Spawner = nullptr;
      int32 Hash = 0;

      bool operator<(const TSpawnerWithHash<T>& other) const
      {
         return Hash < other.Hash;
      }
   };
}
