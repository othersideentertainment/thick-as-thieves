// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Character/OSETeamInterface.h"

#include "TATTeams.generated.h"

UENUM(BlueprintType)
enum class ETATTeamCharacterType : uint8
{
   Player = 0     UMETA(DisplayName = "Player"),
   Guard  = 1     UMETA(DisplayName = "Guard"),
   NPC    = 2     UMETA(DisplayName = "NPC (Neutral To All)"),
   NPCAllied = 3  UMETA(DisplayName = "NPC (Allied To All)"),
   Intruder = 4   UMETA(DisplayName = "Intruder (Hostile to All)"),
   MAX            UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ETATTeamDisguiseHandling : uint8
{
   /// When determining attitudes to an actor, use the team they're disguised as
   UseApparentTeam,

   /// When determining attitudes to an actor, use their actual team
   UseOriginalTeam,
};

UCLASS(BlueprintType)
class TAT_API UTATTeamAttitudeSolver : public UOSETeamAttitudeSolver
{
   GENERATED_BODY()

public:
   static void Init();

   virtual EOSETeamAttitude GetTeamAttitude(const AActor* fromActor, const AActor* toActor) const override;
   virtual EOSETeamAttitude GetTeamAttitude(uint8 teamA, uint8 teamB) const override;

   UFUNCTION(BlueprintPure, Category="OSE Team|TAT")
   static uint8 MakeDisguiseTeam(uint8 teamToDisguiseAs, uint8 originalTeam);

   // Get attitude from an actor to a specific TATTeamCharacterType
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   static EOSETeamAttitude GetTeamAttitudeToTeamCharacterType(const AActor* actor, ETATTeamCharacterType teamCharacterType);

   /// Determine the attitude fromActor has to toActor, with differing handling for disguises
   UFUNCTION(BlueprintPure)
   static EOSETeamAttitude GetTeamAttitudeBetweenActorsWithDisguise(const AActor* fromActor, const AActor* toActor, ETATTeamDisguiseHandling disguiseHandling);

   /// Determine the minimum attitude (i.e. least-friendly) fromActor has to toActor, checking toActor's apparent team and original team
   /// The lower of those two attitude is what is returned
   /// Intended to be used for checking for if an actor should be damaged by another actor
   UFUNCTION(BlueprintPure)
   static EOSETeamAttitude GetLeastFriendlyTeamAttitudeBetweenActorsWithDisguise(const AActor* fromActor, const AActor* toActor);

   // Gets the team value the team appears to be (eg. if disguised, returns the disguised team, otherwise returns the input value).
   UFUNCTION(BlueprintPure, Category = "OSE Team|TAT")
   static uint8 GetApparentTeam(uint8 team);

   // Gets the original team value (eg. if disguised, returns the original team, otherwise just returns the input value).
   UFUNCTION(BlueprintPure, Category = "OSE Team|TAT")
   static uint8 GetOriginalTeam(uint8 team);

   static EOSETeamAttitude GetLessFriendlyAttitude(EOSETeamAttitude attitudeA, EOSETeamAttitude attitudeB);

   // Checks if a character's team value matches a team type.
   // @param ignoreDisguised If true, checks against the actual team, ignoring any disguised team.
   UFUNCTION(BlueprintPure, Category = "OSE Team|TAT")
   static bool TeamEquals(uint8 team, ETATTeamCharacterType teamType, bool ignoreDisguised = false);
};
