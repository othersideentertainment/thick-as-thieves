// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

// ue4
#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;

class TAT_API FGameplayDebuggerCategory_TATStealth : public FGameplayDebuggerCategory
{
public:
   FGameplayDebuggerCategory_TATStealth();

   virtual void CollectData(APlayerController* ownerPC, AActor* debugActor) override;
   virtual void DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext) override;

   static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:

   /// Helper method to draw a specified filled box in a canvasContext.
   static void DrawFilledBox(FGameplayDebuggerCanvasContext& canvasContext, FVector2D position, FVector2D size, FLinearColor color);

   /// Helper method to draw bar graphs of visibility factors.
   ///
   /// Position determined by current cursor position. Draws a horizontal line as wide as the
   /// text height, interpolated so that a total width of "rangeWidth" pixels spans the range of possible
   /// values from minValue to maxValue.
   ///
   /// Moves the current cursor position in canvasContext to the right of the graph being drawn.
   static void DrawGraphBar(FGameplayDebuggerCanvasContext& canvasContext, float barWidth, float minValue, float maxValue, float value, float margin = 10.0f);

   struct FRepData
   {
      float DetectionRate = 0.0f;
      float DistanceMultiplier = 0.0f;
      float Distance = 0.0f;
      float TimeStamp = 0.0f;
      float TimeStimLastPerceived = 0.0f;
      FString AlertnessLevel;
      FVector ViewerEyePoint;
      FVector ViewerEyeDirection;
      FVector ViewerEyeUp;

      void Serialize(FArchive& Ar);
   };
   FRepData DataPack;
};

#endif // WITH_GAMEPLAY_DEBUGGER
