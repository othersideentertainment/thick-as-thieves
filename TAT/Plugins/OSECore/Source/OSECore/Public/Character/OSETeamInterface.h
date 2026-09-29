// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"

#include "OSETeamInterface.generated.h"

UENUM(BlueprintType)
enum class EOSETeamAttitude : uint8
{
   Friendly,
   Neutral,
   Hostile,
};

//////////////////////////////////////////////////////////////////////////
///            UOSETeamAttitudeSolver
//////////////////////////////////////////////////////////////////////////

UCLASS(BlueprintType)
class OSECORE_API UOSETeamAttitudeSolver : public UObject
{
   GENERATED_BODY()

public:
   virtual EOSETeamAttitude GetTeamAttitude(const AActor* fromActor, const AActor* toActor) const;
   virtual EOSETeamAttitude GetTeamAttitude(uint8 teamA, uint8 teamB) const;
};

//////////////////////////////////////////////////////////////////////////
///            IOSETeamInterface
//////////////////////////////////////////////////////////////////////////

UINTERFACE(BlueprintType, Category = "OSE Team Interface", meta=(CannotImplementInterfaceInBlueprint))
class OSECORE_API UOSETeamInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSETeamInterface
{
   GENERATED_BODY()

public:
   /// Invalid team (255)
   static const uint8 kInvalidTeam;

   /// Which team are we on?
   UFUNCTION(BlueprintCallable, Category = "OSE Team")
   virtual uint8 GetTeam() const { return kInvalidTeam; }
   
   // Used for the TAT Disguise component, the "GetTeam" function returns a different value for NPC's if certain tags are set on them
   // However for disguises we want to get their original team.
   virtual uint8 GetOriginalTeam() const { return GetTeam(); }
   
};

//////////////////////////////////////////////////////////////////////////
///            FOSEAISenseAffiliationFilter
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAISenseAffiliationFilter
{
   GENERATED_BODY()

   FOSEAISenseAffiliationFilter()
      : DetectHostiles(false)
      , DetectNeutrals(false)
      , DetectFriendlies(false)
   {
   }

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
   uint32 DetectHostiles : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
   uint32 DetectNeutrals : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
   uint32 DetectFriendlies : 1;
   
   uint8 GetAsFlags() const { return (DetectHostiles << static_cast<int>(EOSETeamAttitude::Hostile)) | (DetectNeutrals << static_cast<int>(EOSETeamAttitude::Neutral)) | (DetectFriendlies << static_cast<int>(EOSETeamAttitude::Friendly)); }
   FORCEINLINE bool ShouldDetectAll() const { return (DetectHostiles && DetectNeutrals && DetectFriendlies); }

   static FORCEINLINE uint8 DetectAllFlags() { return (1 << static_cast<int>(EOSETeamAttitude::Hostile)) | (1 << static_cast<int>(EOSETeamAttitude::Neutral)) | (1 << static_cast<int>(EOSETeamAttitude::Friendly)); }

   static bool ShouldSenseAttitude(EOSETeamAttitude attitude, uint8 affiliationFlags)
   {
      static const uint8 allFlags = DetectAllFlags();
      return affiliationFlags == allFlags || ((1 << static_cast<int>(attitude)) & affiliationFlags);
   }
};

//////////////////////////////////////////////////////////////////////////
///            UOSETeamFunctionLibrary
//////////////////////////////////////////////////////////////////////////

UCLASS()
class OSECORE_API UOSETeamFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Game-specific attitude solver
   static void SetTeamAttitudeSolverClass(TSubclassOf<UOSETeamAttitudeSolver> teamSolverClass);

   /// What is our fromActor's attitude towards toActor
   UFUNCTION(BlueprintPure, Category = "OSE Team")
   static EOSETeamAttitude GetTeamAttitude(const AActor* fromActor, const AActor* toActor);
   static EOSETeamAttitude GetTeamAttitudeToTeam(const AActor* fromActor, const uint8 toTeam);

private:
   static EOSETeamAttitude GetTeamAttitudeFromTeamToTeam(const uint8 fromTeam, const uint8 toTeam);
   static const UOSETeamAttitudeSolver* GetSolver();

   // can be replaced by game-specific implementations by settings this
   static TSubclassOf<UOSETeamAttitudeSolver> sTeamAttitudeSolverClass;
};
