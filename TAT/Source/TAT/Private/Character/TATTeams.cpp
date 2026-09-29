// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/TATTeams.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Disguise/TATDisguiseComponent.h"

// ose
#include "OSEIndividualAttitudeInterface.h"
#include "OSEIndividualAttitudeReceiverInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeams)

DEFINE_LOG_CATEGORY_STATIC(LogTATTeamAttitude, Log, All)

namespace Teams
{
   // If team is disguised, the high nibble will be non-zero
   static bool HasDisguiseTeam(uint8 team)
   {
      return (team & 0xF0) != 0;
   }

   // The low nibble represents the "true" team
   // The high nibble is either 0 if there is no disguise, or it is the team they are disguised as
   static void ProcessTeamDisguises(uint8& fromTeam, uint8& toTeam)
   {
      // For the fromTeam, we care about the true team, since they are the one making the attitude determination
      fromTeam = fromTeam & 0x0F;

      // If toTeam is disguised, then we want the apparent team (the high nibble) if there is a disguise
      // If the high nibble is 0, there is no disguise and we can use the true team as-is
      if (HasDisguiseTeam(toTeam))
      {
         toTeam = (toTeam >> 4) & 0x0F;
      }
   }
}

/* static */
void UTATTeamAttitudeSolver::Init()
{
   // install our solver at the OSE level
   UOSETeamFunctionLibrary::SetTeamAttitudeSolverClass(UTATTeamAttitudeSolver::StaticClass());

#if DO_CHECK
   // Check that all our teams are in the right range: we want to be able to construct disguises as a pair of teams in a single uint8,
   // which means that team values need to be in the range [1, 15]. For now, this should still be plenty of room
   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   for (const auto& teamAssignment : settings.TeamAssignments)
   {
      ensureMsgf(teamAssignment.Value > 0 && teamAssignment.Value <= 0x0f,
         TEXT("TATTeamCharacterType '%s' is mapped to team %d, which is out of range: must be a non-zero number that can fit inside a 4-bit nibble"),
         *UEnum::GetValueAsString(teamAssignment.Key),
         static_cast<int32>(teamAssignment.Value));
   }
#endif
}

void SetWorstAttitude(TOptional<EOSETeamAttitude>& returnAttitude,
                      const EOSEIndividualAttitude& individualAttitude)
{
   if(individualAttitude == EOSEIndividualAttitude::Friendly)
      returnAttitude = EOSETeamAttitude::Friendly;
   else if(individualAttitude == EOSEIndividualAttitude::Hostile)
      returnAttitude = EOSETeamAttitude::Hostile;
   else if(individualAttitude == EOSEIndividualAttitude::Neutral)
      returnAttitude = EOSETeamAttitude::Neutral;
}

void GrabIndividualAttitudeFromActor(TOptional<EOSETeamAttitude>& returnAttitude,
                                     const AActor* a,
                                     const AActor* b)
{
   // If we're running the server (where they have authority over both actors), check the actual component
   if(a->HasAuthority() && b->HasAuthority())
   {
      const IOSEIndividualAttitudeInterface* fromActorInterface = Cast<IOSEIndividualAttitudeInterface>(a);
      if(fromActorInterface == nullptr)
         return;
      UOSEIndividualAttitudeComponent* attitudeComponent = fromActorInterface->GetAttitudeComponent();
      if(attitudeComponent == nullptr)
         return;
      const EOSEIndividualAttitude individualAttitude = attitudeComponent->GetAttitudeTowardsActor(b);
      if(individualAttitude != EOSEIndividualAttitude::Unknown)
      {
         SetWorstAttitude(returnAttitude, individualAttitude);
      }
      return;
   }
   else
   {
      // if we're checking this locally, check against the receiver
      const IOSEIndividualAttitudeReceiverInterface* toActorInterface = Cast<IOSEIndividualAttitudeReceiverInterface>(b);
      if(toActorInterface == nullptr)
         return;
      const EOSEIndividualAttitude individualAttitude = toActorInterface->GetAttitudeFromActor(a);
      if(individualAttitude != EOSEIndividualAttitude::Unknown)
      {
         SetWorstAttitude(returnAttitude, individualAttitude);
      }
   }
}

bool IsDisguiseActive(const uint8& teamID)
{
   return (teamID & 0xF0) != 0;
}

EOSETeamAttitude UTATTeamAttitudeSolver::GetTeamAttitude(const AActor* fromActor, const AActor* toActor) const
{
   const uint8 fromActorTeam = CastChecked<IOSETeamInterface>(fromActor)->GetTeam();
   const uint8 toActorTeam = CastChecked<IOSETeamInterface>(toActor)->GetTeam();

   // Temp: Feature flag for Individual Attitudes
   if(UTATProjectSettings::ShouldUseIndividualAttitudes() == false)
      return GetTeamAttitude(fromActorTeam, toActorTeam);

   // if an actor is disguised, they're fundamentally acting like someone else
   // thus we don't want to use any individual attitudes previously set
   TOptional<EOSETeamAttitude> worstAttitude;
   if (!Teams::HasDisguiseTeam(toActorTeam))
   {
      GrabIndividualAttitudeFromActor(worstAttitude, fromActor, toActor);
      if (worstAttitude.IsSet() && worstAttitude == EOSETeamAttitude::Hostile)
         return EOSETeamAttitude::Hostile;
   }
   if (!Teams::HasDisguiseTeam(fromActorTeam))
   {
      GrabIndividualAttitudeFromActor(worstAttitude, toActor, fromActor);
      if (worstAttitude.IsSet() && worstAttitude == EOSETeamAttitude::Hostile)
         return EOSETeamAttitude::Hostile;
   }
   if(worstAttitude.IsSet())
      return worstAttitude.GetValue();
  
   return GetTeamAttitude(fromActorTeam, toActorTeam);
}

EOSETeamAttitude UTATTeamAttitudeSolver::GetTeamAttitude(uint8 teamA, uint8 teamB) const
{
   Teams::ProcessTeamDisguises(teamA, teamB);

   // Intruders are hostile to everyone
   const uint8 intruderTeamVal = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Intruder);
   if ((teamA == intruderTeamVal || teamB == intruderTeamVal))
   {
      return EOSETeamAttitude::Hostile;
   }
   
   // allied npcs are allied to everyone
   const uint8 alliedNPCTeamVal = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::NPCAllied);
   if (teamA == alliedNPCTeamVal || teamB == alliedNPCTeamVal)
   {
      return EOSETeamAttitude::Friendly;
   }
   const uint8 guardTeamVal = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Guard);
   if(teamA == teamB && teamA == guardTeamVal && teamB == guardTeamVal)
   {
      return EOSETeamAttitude::Friendly;
   }

   const uint8 npcVal = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::NPC);
   if (teamA == npcVal || teamB == npcVal)
   {
      // NPC's are always neutral by default
      return EOSETeamAttitude::Neutral;
   }

   if(teamA < intruderTeamVal && teamB < intruderTeamVal)
   {
      if(teamA == teamB)
      {
         return EOSETeamAttitude::Friendly;
      }
   }
   // everyone else is Hostile
   return EOSETeamAttitude::Hostile;
}

uint8 UTATTeamAttitudeSolver::MakeDisguiseTeam(uint8 teamToDisguiseAs, uint8 originalTeam)
{
   ensureMsgf((teamToDisguiseAs > 0) && (teamToDisguiseAs <= 0x0f) && (originalTeam > 0) && (originalTeam <= 0x0f),
      TEXT("MakeDisguiseTeam called on out-of-range teams. Disguise team %d and original team %d,")
      TEXT("must both be non-negative numbers that fit in a 4-bit nibble"),
      static_cast<int32>(teamToDisguiseAs), static_cast<int32>(originalTeam));

   if (teamToDisguiseAs == originalTeam)
   {
      UE_LOG(LogTATTeamAttitude, Warning, TEXT("Trying to disguise team %d as itself: will return the original team unchanged"),  static_cast<int32>(originalTeam));
      return originalTeam;
   }
   else
   {
      return (teamToDisguiseAs << 4) | originalTeam;
   }
}

EOSETeamAttitude UTATTeamAttitudeSolver::GetTeamAttitudeToTeamCharacterType(const AActor* actor, ETATTeamCharacterType teamCharacterType)
{
   const uint8 team = UTATProjectSettings::GetTeamAssignmentForCharacterType(teamCharacterType);
   return UOSETeamFunctionLibrary::GetTeamAttitudeToTeam(actor, team);
}

// static
EOSETeamAttitude UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(const AActor* fromActor, const AActor* toActor, ETATTeamDisguiseHandling disguiseHandling)
{
   const IOSETeamInterface* toTeamInterface = Cast<IOSETeamInterface>(toActor);
   if (toTeamInterface == nullptr)
   {
      // Defer to the OSE default implementation, which will return the default attitude
      return UOSETeamFunctionLibrary::GetTeamAttitude(fromActor, toActor);
   }

   uint8 toTeam = toTeamInterface->GetTeam();
   switch (disguiseHandling) {
      case ETATTeamDisguiseHandling::UseApparentTeam:
         toTeam = GetApparentTeam(toTeam);
         break;

      case ETATTeamDisguiseHandling::UseOriginalTeam:
         toTeam = GetOriginalTeam(toTeam);
         break;

      default:
         checkNoEntry();
   }

   return UOSETeamFunctionLibrary::GetTeamAttitudeToTeam(fromActor, toTeam);
}

// static
EOSETeamAttitude UTATTeamAttitudeSolver::GetLeastFriendlyTeamAttitudeBetweenActorsWithDisguise(const AActor* fromActor, const AActor* toActor)
{
   const EOSETeamAttitude apparentTeamAttitude = GetTeamAttitudeBetweenActorsWithDisguise(fromActor, toActor, ETATTeamDisguiseHandling::UseApparentTeam);
   const EOSETeamAttitude originalTeamAttitude = GetTeamAttitudeBetweenActorsWithDisguise(fromActor, toActor, ETATTeamDisguiseHandling::UseOriginalTeam);

   return GetLessFriendlyAttitude(apparentTeamAttitude, originalTeamAttitude);
}

//static
uint8 UTATTeamAttitudeSolver::GetApparentTeam(uint8 team)
{
   const bool hasDisguise = Teams::HasDisguiseTeam(team);
   return hasDisguise ? ((team >> 4) & 0x0F) : team;
}

//static
uint8 UTATTeamAttitudeSolver::GetOriginalTeam(uint8 team)
{
   const bool hasDisguise = Teams::HasDisguiseTeam(team);
   return hasDisguise ? (team & 0x0F) : team;
}

// static
EOSETeamAttitude UTATTeamAttitudeSolver::GetLessFriendlyAttitude(EOSETeamAttitude attitudeA, EOSETeamAttitude attitudeB)
{
   if (attitudeA == EOSETeamAttitude::Hostile || attitudeB == EOSETeamAttitude::Hostile)
   {
      return EOSETeamAttitude::Hostile;
   }
   else if (attitudeA == EOSETeamAttitude::Neutral || attitudeB == EOSETeamAttitude::Neutral)
   {
      return EOSETeamAttitude::Neutral;
   }
   else if (attitudeA == EOSETeamAttitude::Friendly || attitudeB == EOSETeamAttitude::Friendly)
   {
      return EOSETeamAttitude::Friendly;
   }
   else
   {
      checkNoEntry();
      return EOSETeamAttitude::Neutral;
   }
}

//static
bool UTATTeamAttitudeSolver::TeamEquals(uint8 team, ETATTeamCharacterType teamType, bool ignoreDisguised)
{
   const uint8 teamTypeValue = UTATProjectSettings::GetTeamAssignmentForCharacterType(teamType);
   return ignoreDisguised ? (GetOriginalTeam(team) == teamTypeValue) : (GetApparentTeam(team) == teamTypeValue);
}
