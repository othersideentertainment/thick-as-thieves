// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootInstanceIDSubsystem.h"

// tat
#include "Loot/TATLootTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInstanceIDSubsystem)


// static
UTATLootInstanceIDSubsystem* UTATLootInstanceIDSubsystem::Get(const UObject* contextObj)
{
   check(IsValid(contextObj));

   UWorld* world = contextObj->GetWorld();
   check(world);
   return world->GetSubsystem<UTATLootInstanceIDSubsystem>();
}

void UTATLootInstanceIDSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   _nextIDToGiveOut = 0;
}

int32 UTATLootInstanceIDSubsystem::AuthorityGetNextLootID()
{
#if DO_CHECK
   ENetMode netMode = GetWorld()->GetNetMode();
   check(netMode != NM_Client);
#endif

   const int32 lootID = _nextIDToGiveOut;
   _nextIDToGiveOut++;
   return lootID;
}


