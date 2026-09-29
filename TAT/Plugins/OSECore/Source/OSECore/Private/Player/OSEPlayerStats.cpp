// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/OSEPlayerStats.h"

// ose
#include "OSECommon.h"
#include "Online/OSEGameState.h"
#include "Player/OSEPlayerState.h"

// ue4
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerStats)

FOSEPlayerStat& FOSEPlayerStats::GetOrAddStat(const FGameplayTag& tag)
{
   if (const FOSEPlayerStat* stat = FindStat(tag))
   {
      return const_cast<FOSEPlayerStat&>(*stat);
   }

   FOSEPlayerStat newStat;
   newStat.Tag = tag;
   return Stats.Add_GetRef(newStat);
}

const FOSEPlayerStat* FOSEPlayerStats::FindStat(const FGameplayTag& tag) const
{
   for (const FOSEPlayerStat& stat : Stats)
   {
      if (stat.Tag == tag)
         return &stat;
   }
   return nullptr;
}

int FOSEPlayerStats::FindStatValue(const FGameplayTag& tag) const
{
   if (const FOSEPlayerStat* stat = FindStat(tag))
   {
      return stat->IntValue;
   }

   return 0;
}

void FOSEDamageLog::AddDamageEntry(const FOSEDamageLogEntry& entry, int maxEntriesAllowed)
{
   DamageLogEntries.Add(entry);
   while (DamageLogEntries.Num() > maxEntriesAllowed)
      DamageLogEntries.RemoveAt(0);
}

bool UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(AActor* playerActor, FGameplayTag tag, int updateValue /* 1 */)
{
   if(!tag.IsValid())
   {
      return false;
   }
   if (AOSEPlayerState* ps = _GetServerOSEPlayerState(playerActor))
   {
      ps->AuthorityUpdatePlayerStatInt(tag, updateValue);
      return true;
   }
   return false;
}

bool UOSEPlayerStatsFunctionLibrary::AuthorityUpdateSessionStatInt(UObject* worldContextObject, FGameplayTag tag, int updateValue /* 1 */)
{
   check(worldContextObject);
   AOSEGameState* gs = worldContextObject->GetWorld()->GetGameState<AOSEGameState>();
   if (tag.IsValid() && gs && gs->HasAuthority())
   {
      gs->AuthorityUpdateSessionStatInt(tag, updateValue);
      return true;
   }
   return false;
}

bool UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatUniqueByName(AActor* playerActor,
   FGameplayTag tag, FName nameKey)
{
   if(!tag.IsValid())
   {
      return false;
   }
   
   if (AOSEPlayerState* ps = _GetServerOSEPlayerState(playerActor))
   {
      return ps->AuthorityUpdatePlayerStatUniqueByName(tag, nameKey);
   }
   return false;
}

bool UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatUniqueByObject(AActor* playerActor, FGameplayTag tag,
   UObject* objectKey)
{
   if(!tag.IsValid())
   {
      return false;
   }
   
   if (AOSEPlayerState* ps = _GetServerOSEPlayerState(playerActor))
   {
      // does anything actually care if the return value changes the stat?
      return ps->AuthorityUpdatePlayerStatUniqueByObject(tag, objectKey);
   }
   return false;
}

int UOSEPlayerStatsFunctionLibrary::GetPlayerStatInt(AActor* playerActor, FGameplayTag tag)
{
   AOSEPlayerState* ps = UOSECommon::GetPlayerState<AOSEPlayerState>(playerActor);
   if (tag.IsValid() && ps)
   {
      const FOSEPlayerStats& stats = ps->GetPlayerStats();
      return stats.FindStatValue(tag);
   }
   return 0;
}

int UOSEPlayerStatsFunctionLibrary::GetSessionStatInt(UObject* worldContextObject, FGameplayTag tag)
{
   check(worldContextObject);
   if (AOSEGameState* gs = worldContextObject->GetWorld()->GetGameState<AOSEGameState>())
   {
      const FOSEPlayerStats& stats = gs->GetSessionStats();
      return stats.FindStatValue(tag);
   }
   return 0;
}

AOSEPlayerState* UOSEPlayerStatsFunctionLibrary::_GetServerOSEPlayerState(AActor* playerActor)
{
   if (AOSEPlayerState* ps = UOSECommon::GetPlayerState<AOSEPlayerState>(playerActor))
   {
      if (ps->HasAuthority())
      {
         return ps;
      }
   }
   return nullptr;
}

