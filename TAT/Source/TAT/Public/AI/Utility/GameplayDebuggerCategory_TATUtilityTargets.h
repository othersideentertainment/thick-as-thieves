// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

enum class EBehaviorTargetType : uint8;
enum class EOSETeamAttitude : uint8;
struct FUtilityStateTarget;

class TAT_API FGameplayDebuggerCategory_TATUtilityTargets : public FGameplayDebuggerCategory
{
public:
   FGameplayDebuggerCategory_TATUtilityTargets();

   static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

   // from FGameplayDebuggerCategory
   virtual void CollectData(APlayerController* ownerPC, AActor* debugActor) override;
   virtual void DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext) override;

protected:
   struct FUtilityTargetInfo
   {
      FString Name;
      EBehaviorTargetType TargetType;
      uint8 Team = 0;
      EOSETeamAttitude AttitudeTowardsTarget;
      EOSETeamAttitude AttitudeTowardsTargetTeam;

      friend FArchive& operator<<(FArchive& ar, FUtilityTargetInfo& elem)
      {
         ar << elem.Name;
         ar << elem.TargetType;
         ar << elem.Team;
         ar << elem.AttitudeTowardsTarget;
         ar << elem.AttitudeTowardsTargetTeam;
         return ar;
      }
   };

   struct FRepData
   {
      FUtilityTargetInfo BehaviorTargetInfo;
      FUtilityTargetInfo GoalTargetInfo;

      void Serialize(FArchive& ar);
   };
   FRepData _dataPack;

private:
   static void _FillUtilityTargetInfo(FUtilityTargetInfo& toFill, const FUtilityStateTarget& fillWith, AActor* debugActor);
   static void _DrawUtilityTargetInfo(const FString& stateName, APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext, const FUtilityTargetInfo& targetInfo);
	
};

#endif // WITH_GAMEPLAY_DEBUGGER
