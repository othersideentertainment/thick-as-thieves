// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/TATAuthMapSeedSubsystem.h"

// tat
#include "Developer/TATEditorSettings.h"

// ose
#include "OSECoreCheats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAuthMapSeedSubsystem)

bool UTATAuthMapSeedSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATAuthMapSeedSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // For the heck of it?
   GetOrCreateSeed();
}

int32 UTATAuthMapSeedSubsystem::GetOrCreateSeed()
{
   if(!_hasSeed)
   {
      _seed = _ChooseSeed();
      _hasSeed = true;
   }
   return _seed;
}

bool UTATAuthMapSeedSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

int32 UTATAuthMapSeedSubsystem::_ChooseSeed() const
{
   int32 seed = ((int32)(FDateTime::Now().GetTicks() % (int64)MAX_int32));

   // want to allow this in perf tests
#if 1 // OSE_CHEATS_ENABLED || !UE_BUILD_SHIPPING
   const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
   if (editorSettings.WorldRandomizationSeed != INDEX_NONE)
   {
      seed = editorSettings.WorldRandomizationSeed;
   }
#endif // OSE_CHEATS_ENABLED

   return seed;
}
