// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/GameplayDebuggerCategory_UtilityStates.h"

#if WITH_GAMEPLAY_DEBUGGER

// ose
#include "OSECommon.h"
#include "AI/OSEAIController.h"
#include "AI/OSEAISettings.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"

// ue4
#include "AIController.h"
#include "CanvasItem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace UtilityAIBehaviorGameplayDebuggerCVars
{
   static int NumBehaviorsToDisplay = 5;
   FAutoConsoleVariableRef CVarNumBehaviorsToDisplay(
      TEXT("OSE.AI.GameplayDebugger.NumBehaviorsToDisplay"),
      NumBehaviorsToDisplay,
      TEXT("How many behaviors should we show in the gameplay debugger?"),
      ECVF_Default);

   static int NumGoalsToDisplay = 5;
   FAutoConsoleVariableRef CVarNumGoalsToDisplay(
      TEXT("OSE.AI.GameplayDebugger.NumGoalsToDisplay"),
      NumGoalsToDisplay,
      TEXT("How many goals should we show in the gameplay debugger?"),
      ECVF_Default);

   static int ShowAllConsiderations = 0;
   FAutoConsoleVariableRef CVarShowAllConsiderations(
      TEXT("OSE.AI.GameplayDebugger.ShowAllConsiderations"),
      ShowAllConsiderations,
      TEXT("Should the utility gameplay debugger show all considerations, or just failed ones?"),
      ECVF_Default);
}

FGameplayDebuggerCategory_UtilityStates::FGameplayDebuggerCategory_UtilityStates()
{
   SetDataPackReplication<FRepData>(&_dataPack);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_UtilityStates::MakeInstance()
{
   return MakeShareable(new FGameplayDebuggerCategory_UtilityStates());
}

void FGameplayDebuggerCategory_UtilityStates::FRepData::Serialize(FArchive& ar)
{
   ar << BehaviorInfo;
   ar << GoalInfo;
}

void FGameplayDebuggerCategory_UtilityStates::CollectData(APlayerController* ownerPC, AActor* debugActor)
{
   // reset
   _dataPack.BehaviorInfo.Reset();
   _dataPack.GoalInfo.Reset();
   
   if (debugActor)
   {
      if (AOSEAIController* debugController = UOSECommon::GetController<AOSEAIController>(debugActor))
      {
         // goals
         if (UUtilityAIGoalComponent* goalComp = debugController->GetUtilityAIGoalComponent())
         {
            // copy and sort array for our own display
            auto entries = goalComp->GetStateTargetDebugLogEntries();
            entries.StableSort([](const auto& a, const auto& b)
            {
               if (a.Score == b.Score)
                  return a.Name.LexicalLess(b.Name);
               return a.Score > b.Score;
            });

            for (const UUtilityAIBehaviorComponent::FStateTargetDebugLogEntry& entry : entries)
            {
               FUtilityStateInfo& info = _dataPack.GoalInfo.Add_GetRef(FUtilityStateInfo());
               info.Name = entry.Name.ToString();
               info.Target = entry.Target.ToString();
               info.Weight = entry.Weight;
               info.Score = entry.Score;
               info.Bonus = entry.Bonus;
               info.WasFullyConsidered = entry.WasFullyConsidered;

               for (const UUtilityAIBehaviorComponent::FConsiderationDebugLogEntry& considerationEntry : entry.ConsiderationLogEntries)
               {
                  FUtilityConsiderationInfo& considerationInfo = info.ConsiderationInfos.Add_GetRef(FUtilityConsiderationInfo());
                  considerationInfo.Name = considerationEntry.Name.ToString();
                  considerationInfo.Score = considerationEntry.Score;
               }
            }
         }

         // behaviors
         if (UUtilityAIBehaviorComponent* behaviorComp = debugController->GetUtilityAIBehaviorComponent())
         {
            // copy and sort array for our own display
            auto entries = behaviorComp->GetStateTargetDebugLogEntries();
            entries.StableSort([](const auto& a, const auto& b)
            {
               if (a.Score == b.Score)
                  return a.Name.LexicalLess(b.Name);
               return a.Score > b.Score;
            });

            for (const UUtilityAIBehaviorComponent::FStateTargetDebugLogEntry& entry : entries)
            {
               FUtilityStateInfo& info = _dataPack.BehaviorInfo.Add_GetRef(FUtilityStateInfo());
               info.Name = entry.Name.ToString();
               info.Target = entry.Target.ToString();
               info.Weight = entry.Weight;
               info.Score = entry.Score;
               info.Bonus = entry.Bonus;
               info.WasFullyConsidered = entry.WasFullyConsidered;

               for (const UUtilityAIBehaviorComponent::FConsiderationDebugLogEntry& considerationEntry : entry.ConsiderationLogEntries)
               {
                  FUtilityConsiderationInfo& considerationInfo = info.ConsiderationInfos.Add_GetRef(FUtilityConsiderationInfo());
                  considerationInfo.Name = considerationEntry.Name.ToString();
                  considerationInfo.Score = considerationEntry.Score;
               }
            }
         }
      }
   }
}

namespace DebuggerCategory_UtilityStates_Helpers
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
}

void FGameplayDebuggerCategory_UtilityStates::DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext)
{
   float initialCursorX = canvasContext.CursorX;
   float initialCursorY = canvasContext.CursorY;

   // goals
   _DrawUtilityStateInfo(TEXT("Goals"), ownerPC, canvasContext, _dataPack.GoalInfo);

   DebuggerCategory_UtilityStates_Helpers::MoveToNewColumn(canvasContext, initialCursorX, initialCursorY);

   // behaviors
   _DrawUtilityStateInfo(TEXT("Behaviors"), ownerPC, canvasContext, _dataPack.BehaviorInfo);
}

/* static */
void FGameplayDebuggerCategory_UtilityStates::_DrawUtilityStateInfo(const FString& stateName, APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext, const TArray<FGameplayDebuggerCategory_UtilityStates::FUtilityStateInfo>& stateInfo)
{
   const UOSEAISettings& settings = UOSEAISettings::Get();

   canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::Cyan, TEXT("%s:"), *stateName);
   DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
   DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
   for (int idx = 0; idx < UtilityAIBehaviorGameplayDebuggerCVars::NumGoalsToDisplay; ++idx)
   {
      if (stateInfo.IsValidIndex(idx))
      {
         const FUtilityStateInfo& info = stateInfo[idx];
         const FColor nameColor = (idx == 0 && info.Score > 0.0f) ? FColor::Green : FColor::Red;

         // state info
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, nameColor, TEXT("Name = %s"), *info.Name);
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Target = %s"), *info.Target);
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Score = %.02f"), info.Score);
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Bonus = %.02f"), info.Bonus);
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("Weight = %.02f"), info.Weight);
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
         canvasContext.PrintfAt(canvasContext.CursorX, canvasContext.CursorY, FColor::White, TEXT("WasFullyConsidered = %s"), info.WasFullyConsidered ? TEXT("Yes") : TEXT("No"));
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
      
         // considerations info
         for(const FUtilityConsiderationInfo& considerationInfo : info.ConsiderationInfos)
         {
            const float considerationScore = considerationInfo.Score < settings.MinStateScore ? 0.0f : considerationInfo.Score;

            if (considerationScore == 0.0f || UtilityAIBehaviorGameplayDebuggerCVars::ShowAllConsiderations)
            {
               const FColor considerationColor = (considerationScore > 0.0f) ? FColor::Green : FColor::Red;
               canvasContext.PrintfAt(canvasContext.CursorX + DebuggerCategory_UtilityStates_Helpers::kIndentXVal, canvasContext.CursorY, considerationColor, TEXT("%.02f : %s"), considerationInfo.Score, *considerationInfo.Name);
               DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
            }
         }

         // extra newline between each thing
         DebuggerCategory_UtilityStates_Helpers::MoveToNewLineYOnly(canvasContext);
      }
   }
}
#endif // WITH_GAMEPLAY_DEBUGGER
