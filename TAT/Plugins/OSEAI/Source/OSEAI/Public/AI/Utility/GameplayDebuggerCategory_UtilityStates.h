// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

// ue4
#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;

class OSEAI_API FGameplayDebuggerCategory_UtilityStates : public FGameplayDebuggerCategory
{
public:
   FGameplayDebuggerCategory_UtilityStates();

   virtual void CollectData(APlayerController* ownerPC, AActor* debugActor) override;
   virtual void DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext) override;

   static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:

   struct FUtilityConsiderationInfo
   {
      FString Name;
      float Score = 0.0f;

      friend FArchive& operator<<(FArchive& ar, FUtilityConsiderationInfo& elem)
      {
         ar << elem.Name;
         ar << elem.Score;
         return ar;
      }
   };

   struct FUtilityStateInfo
   {
      FString Name;
      FString Target;
      float Weight = 0.0f;
      float Score = 0.0f;
      float Bonus = 0.0f;
      bool WasFullyConsidered = true;
      TArray<FUtilityConsiderationInfo> ConsiderationInfos;

      friend FArchive& operator<<(FArchive& ar, FUtilityStateInfo& elem)
      {
         ar << elem.Name;
         ar << elem.Target;
         ar << elem.Weight;
         ar << elem.Score;
         ar << elem.Bonus;
         ar << elem.WasFullyConsidered;
         ar << elem.ConsiderationInfos;
         return ar;
      }
   };

   struct FRepData
   {
      TArray<FUtilityStateInfo> BehaviorInfo;
      TArray<FUtilityStateInfo> GoalInfo;

      void Serialize(FArchive& ar);
   };
   FRepData _dataPack;

private:
   static void _DrawUtilityStateInfo(const FString& stateName, APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext, const TArray<FUtilityStateInfo>& stateInfo);
};

#endif // WITH_GAMEPLAY_DEBUGGER
