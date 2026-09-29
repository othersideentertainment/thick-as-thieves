// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/OSETeamInterface.h"
#include "OSEProjectSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSETeamInterface)

namespace TeamInterfaceUtl
{
   // I suppose this is a reasonable default?  I think this will reveal more bugs than hostile/friendly would.
   const EOSETeamAttitude kDefaultAttitude = EOSETeamAttitude::Neutral;
}
const uint8 IOSETeamInterface::kInvalidTeam = 255;
TSubclassOf<UOSETeamAttitudeSolver> UOSETeamFunctionLibrary::sTeamAttitudeSolverClass = nullptr;

//////////////////////////////////////////////////////////////////////////
///            UOSETeamAttitudeSolver
//////////////////////////////////////////////////////////////////////////

EOSETeamAttitude UOSETeamAttitudeSolver::GetTeamAttitude(const AActor* fromActor, const AActor* toActor) const
{
  return GetTeamAttitude(CastChecked<IOSETeamInterface>(fromActor)->GetTeam(), CastChecked<IOSETeamInterface>(toActor)->GetTeam());
}

EOSETeamAttitude UOSETeamAttitudeSolver::GetTeamAttitude(uint8 teamA, uint8 teamB) const
{
   // default impl is mismatched teams are hostile to one another, subclass and reimplement for game-specific teams!
   return (teamA == teamB) ? EOSETeamAttitude::Friendly : EOSETeamAttitude::Hostile;
}

//////////////////////////////////////////////////////////////////////////
///            UOSETeamFunctionLibrary
//////////////////////////////////////////////////////////////////////////

/* static */
void UOSETeamFunctionLibrary::SetTeamAttitudeSolverClass(TSubclassOf<UOSETeamAttitudeSolver> teamSolverClass)
{
   check(teamSolverClass);
   sTeamAttitudeSolverClass = teamSolverClass;
}

const UOSETeamAttitudeSolver* UOSETeamFunctionLibrary::GetSolver()
{
   if (!sTeamAttitudeSolverClass)
   {
      SetTeamAttitudeSolverClass(UOSETeamAttitudeSolver::StaticClass());
   }
   check(sTeamAttitudeSolverClass);
   const UOSETeamAttitudeSolver* solverCDO = sTeamAttitudeSolverClass.GetDefaultObject();
   check(solverCDO);
   return solverCDO;
}

EOSETeamAttitude UOSETeamFunctionLibrary::GetTeamAttitude(const AActor* fromActor, const AActor* toActor)
{
   if (!fromActor || !toActor)
   {
      return TeamInterfaceUtl::kDefaultAttitude;
   }

   if (!fromActor->Implements<UOSETeamInterface>() || !toActor->Implements<UOSETeamInterface>())
   {
      return TeamInterfaceUtl::kDefaultAttitude;
   }
   const UOSETeamAttitudeSolver* solverCDO = GetSolver();
   return solverCDO->GetTeamAttitude(fromActor, toActor);
}

EOSETeamAttitude UOSETeamFunctionLibrary::GetTeamAttitudeToTeam(const AActor* fromActor, const uint8 toTeam)
{
   if (const IOSETeamInterface* fromTeamInterface = Cast<IOSETeamInterface>(fromActor))
   {
      return GetTeamAttitudeFromTeamToTeam(fromTeamInterface->GetTeam(), toTeam);
   }

   return TeamInterfaceUtl::kDefaultAttitude;
}

EOSETeamAttitude UOSETeamFunctionLibrary::GetTeamAttitudeFromTeamToTeam(const uint8 fromTeam, const uint8 toTeam)
{
   const UOSETeamAttitudeSolver* solverCDO = GetSolver();
   return solverCDO->GetTeamAttitude(fromTeam, toTeam);
}

