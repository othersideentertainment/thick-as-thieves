// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEPlayerStats.generated.h"

class AOSEPlayerState;

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPlayerStat
{
   GENERATED_BODY()

public:

   UPROPERTY(BlueprintReadOnly, meta = (Categories = "PlayerStats"))
   FGameplayTag Tag;

   UPROPERTY(BlueprintReadOnly)
   int IntValue = 0;

   // TODO as needed:
   // ... other value types
   // ... some enum to interpret this union-ish struct
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPlayerStats
{
   GENERATED_BODY()

public:

   UPROPERTY(BlueprintReadOnly)
   TArray<FOSEPlayerStat> Stats;

   FOSEPlayerStat& GetOrAddStat(const FGameplayTag& tag);
   const FOSEPlayerStat* FindStat(const FGameplayTag& tag) const;
   int FindStatValue(const FGameplayTag& tag) const;
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEDamageLogEntry
{
   GENERATED_BODY()

public:

   // TODO: Will need to figure out something better here -- FString is ok for player names but guard
   // names should be localized and come from FText before being placed in here

   UPROPERTY(BlueprintReadOnly)
   bool IsIncomingDamage = true; // otherwise, it's outgoing damage

   UPROPERTY(BlueprintReadOnly)
   FString FromCharacterName;

   UPROPERTY(BlueprintReadOnly)
   FString ToCharacterName;

   UPROPERTY(BlueprintReadOnly)
   float DamageAmount = 0.0f;

   UPROPERTY(BlueprintReadOnly)
   FDateTime TimeStamp = FDateTime(ForceInit);
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEDamageLog
{
   GENERATED_BODY()

public:

   UPROPERTY(BlueprintReadOnly)
   TArray<FOSEDamageLogEntry> DamageLogEntries;

   void AddDamageEntry(const FOSEDamageLogEntry& entry, int maxEntriesAllowed);
};

UCLASS()
class OSECORE_API UOSEPlayerStatsFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "PlayerStats"), Category = "Stats|Player")
   static bool AuthorityUpdatePlayerStatInt(AActor* playerActor, FGameplayTag tag, int updateValue = 1);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "PlayerStats", WorldContext = "worldContextObject"), Category = "Stats|Session")
   static bool AuthorityUpdateSessionStatInt(UObject* worldContextObject, FGameplayTag tag, int updateValue = 1);

   // Tracks the number of unique names this is called with as a player stat
   // Useful when there is a natural FName (or tag) that will remain
   // 1. Stable enough in the match (on authority)
   // 2. Unique enough within callers to this
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "PlayerStats"), Category = "Stats|Session")
   static bool AuthorityUpdatePlayerStatUniqueByName(AActor* playerActor, FGameplayTag tag, FName nameKey);
   
   // Tracks the number of unique objects this is called with as a player stat
   // Useful when there is a natural uobject that will remain stable enough in the match (on authority).
   //
   // Probably not suitable to use with assets that may be unloaded and loaded again in a match,
   // or actors that will be destroyed and re-created (but want a stable identity)
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "PlayerStats"), Category = "Stats|Session")
   static bool AuthorityUpdatePlayerStatUniqueByObject(AActor* playerActor, FGameplayTag tag, UObject* objectKey);
   UFUNCTION(BlueprintCallable, meta = (GameplayTagFilter = "PlayerStats"), Category = "Stats|Player")
   static int GetPlayerStatInt(AActor* playerActor, FGameplayTag tag);
   UFUNCTION(BlueprintCallable, meta = (GameplayTagFilter = "PlayerStats"), meta = (WorldContext = "worldContextObject"), Category = "Stats|Session")
   static int GetSessionStatInt(UObject* worldContextObject, FGameplayTag tag);

private:
   static AOSEPlayerState* _GetServerOSEPlayerState(AActor* playerActor);
};
