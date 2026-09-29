// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/GameplayDebuggerCategory_TATUtilityTargets.h"

#if WITH_GAMEPLAY_DEBUGGER

// tat
#include "Character/TATTeams.h"
#include "Developer/TATProjectSettings.h"

// ose
#include "OSECommon.h"
#include "AI/OSEAIController.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"
#include "Character/OSETeamInterface.h"

// ue
#include "AIController.h"
#include "CanvasItem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace DebuggerCategory_UtilityTargets_Helpers
{
   static const float kColumnXVal = 500.0f;
   static const float kIndentXVal = 20.0f;

   void MoveToNewLineYOnly(FGameplayDebuggerCanvasContext& canvasContext)
   {
      const float lineHeight = canvasContext.GetLineHeight();
      canvasContext.CursorY += lineHeight;
   };

   void MoveToNewColumn(FGameplayDebuggerCanvasContext& canvasContext, float initialCursorX, float initialCursorY)
   {
      canvasContext.CursorX = initialCursorX + kColumnXVal;
      canvasContext.CursorY = initialCursorY;
   };

   FString FindTeamAssignment(uint8 team)
   {
      FString displayName = TEXT("Unknown");

      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      for (auto itTeamAssignment : settings.TeamAssignments)
      {
         if (itTeamAssignment.Value == team)
         {
            displayName = UEnum::GetValueAsString(itTeamAssignment.Key);
            break;
         }
      }

      return FString::Printf(TEXT("%s (%d)"), *displayName, team);
   }
}

FGameplayDebuggerCategory_TATUtilityTargets::FGameplayDebuggerCategory_TATUtilityTargets()
{
   SetDataPackReplication<FRepData>(&_dataPack);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_TATUtilityTargets::MakeInstance()
{
   return MakeShareable(new FGameplayDebuggerCategory_TATUtilityTargets());
}

void FGameplayDebuggerCategory_TATUtilityTargets::CollectData(APlayerController* ownerPC, AActor* debugActor)
{
   if (debugActor)
   {
      if (AOSEAIController* debugController = UOSECommon::GetController<AOSEAIController>(debugActor))
      {
         // goals
         if (UUtilityAIGoalComponent* goalComp = debugController->GetUtilityAIGoalComponent())
         {
            const FUtilityStateTarget& goalTarget = goalComp->GetCurrentTarget();
            _FillUtilityTargetInfo(_dataPack.GoalTargetInfo, goalTarget, debugActor);
         }

         // behaviors
         if (UUtilityAIBehaviorComponent* behaviorComp = debugController->GetUtilityAIBehaviorComponent())
         {
            const FUtilityStateTarget& behaviorTarget = behaviorComp->GetCurrentTarget();
            _FillUtilityTargetInfo(_dataPack.BehaviorTargetInfo, behaviorTarget, debugActor);
         }
      }
   }
}

void FGameplayDebuggerCategory_TATUtilityTargets::_FillUtilityTargetInfo(FUtilityTargetInfo& toFill, const FUtilityStateTarget& fillWith, AActor* debugActor)
{
   toFill.Name = fillWith.ToString();
   toFill.TargetType = fillWith.TargetType;

   if (toFill.TargetType == EBehaviorTargetType::Actor)
   {
      const AActor* targetActor = fillWith.Actor.Get();
      toFill.AttitudeTowardsTarget = UOSETeamFunctionLibrary::GetTeamAttitude(debugActor, targetActor);
      if (const IOSETeamInterface* teamInterface = Cast<IOSETeamInterface>(targetActor))
      {
         toFill.Team = teamInterface->GetTeam();
         toFill.AttitudeTowardsTargetTeam = UOSETeamFunctionLibrary::GetTeamAttitudeToTeam(debugActor, toFill.Team);
      }
   }
   // TODO : support other target types with other data
}

void FGameplayDebuggerCategory_TATUtilityTargets::DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext)
{
   float initialCursorX = canvasContext.CursorX;
   float initialCursorY = canvasContext.CursorY;

   // goal target
   _DrawUtilityTargetInfo(TEXT("Goal Target"), ownerPC, canvasContext, _dataPack.GoalTargetInfo);

   DebuggerCategory_UtilityTargets_Helpers::MoveToNewColumn(canvasContext, initialCursorX, initialCursorY);

   // behavior target
   _DrawUtilityTargetInfo(TEXT("Behavior Target"), ownerPC, canvasContext, _dataPack.BehaviorTargetInfo);
}

void FGameplayDebuggerCategory_TATUtilityTargets::_DrawUtilityTargetInfo(const FString& stateName, APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext, const FUtilityTargetInfo& targetInfo)
{
   canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::Cyan, TEXT("%s:"), *stateName);
   DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);
   DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);

   if (targetInfo.TargetType == EBehaviorTargetType::Actor)
   {
      canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Name: %s"), *targetInfo.Name);
      DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);

      const FString teamString = DebuggerCategory_UtilityTargets_Helpers::FindTeamAssignment(targetInfo.Team);
      canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Team: %s"), *teamString);
      DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);

      const FString attitudeTargetString = UEnum::GetValueAsString(targetInfo.AttitudeTowardsTarget);
      canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Attitude Towards Target: %s"), *attitudeTargetString);
      DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);

      const FString attitudeTeamString = UEnum::GetValueAsString(targetInfo.AttitudeTowardsTargetTeam);
      canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Attitude Towards Target Team: %s"), *attitudeTeamString);
      DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);
   }
   // TODO : support other target types with other data
   
   // extra newline between each thing
   DebuggerCategory_UtilityTargets_Helpers::MoveToNewLineYOnly(canvasContext);
}

void FGameplayDebuggerCategory_TATUtilityTargets::FRepData::Serialize(FArchive& ar)
{
   ar << BehaviorTargetInfo;
   ar << GoalTargetInfo;
}

#endif // WITH_GAMEPLAY_DEBUGGER
