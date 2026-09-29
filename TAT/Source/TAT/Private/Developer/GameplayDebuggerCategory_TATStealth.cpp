// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "Developer/GameplayDebuggerCategory_TATStealth.h"

#if WITH_GAMEPLAY_DEBUGGER

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"
#include "Player/TATCharacter.h"

// ose
#include "OSECommon.h"
#include "AI/Perception/OSEStimDatabaseInterface.h"

// ue4
#include "DrawDebugHelpers.h"
#include "AIController.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"

FGameplayDebuggerCategory_TATStealth::FGameplayDebuggerCategory_TATStealth()
{
   SetDataPackReplication<FRepData>(&DataPack);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_TATStealth::MakeInstance()
{
   return MakeShareable(new FGameplayDebuggerCategory_TATStealth());
}

void FGameplayDebuggerCategory_TATStealth::FRepData::Serialize(FArchive& Ar)
{
   Ar << DetectionRate;
   Ar << DistanceMultiplier;
   Ar << Distance;
   Ar << TimeStamp;
   Ar << TimeStimLastPerceived;
   Ar << AlertnessLevel;
   Ar << ViewerEyePoint;
   Ar << ViewerEyeDirection;
   Ar << ViewerEyeUp;
}

void FGameplayDebuggerCategory_TATStealth::CollectData(APlayerController* ownerPC, AActor* debugActor)
{
   const ATATCharacter* playerCharacter = Cast<ATATCharacter>(ownerPC->GetPawn());
   if (playerCharacter && debugActor)
   {
      const float now = playerCharacter->GetWorld()->GetTimeSeconds();

      AActor* viewerActor = debugActor;
      // If DebugActor is a pawn, try to get its controller.
      if (APawn* viewerPawn = Cast<APawn>(debugActor))
      {
         viewerActor = viewerPawn->GetController();
      }

      if (viewerActor)
      {
         DataPack.TimeStimLastPerceived = 0.0f;

         if (const auto* aiController = Cast<ATATAIController>(viewerActor))
         {
            if (const IOSEStimDatabaseInterface* dbOwner = Cast<IOSEStimDatabaseInterface>(aiController))
            {
               if (const UOSEStimDatabase* db = dbOwner->AuthorityGetStimDatabase())
               {
                  if (const FStimInfo* stimInfo = db->FindNewestStim())
                  {
                     DataPack.TimeStimLastPerceived = (now - stimInfo->Timestamp);
                  }
               }
            }
            
            DataPack.AlertnessLevel = UOSECommon::UnqualifiedEnumToString(aiController->GetAlertnessLevel());

            if (const UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent())
            {
               if (const FVisibilityLog* log = knowledge->GetVisibilityLogForActor(playerCharacter))
               {
                  DataPack.DetectionRate = log->DetectionRate;
                  DataPack.DistanceMultiplier = log->DistanceMultiplier;
                  DataPack.Distance = log->Distance;
                  DataPack.TimeStamp = log->Timestamp;
               }
            }
         }

         FVector eyeViewPoint, eyeDirection;
         FRotator eyeRotation;
         viewerActor->GetActorEyesViewPoint(eyeViewPoint, eyeRotation);
         eyeDirection = eyeRotation.RotateVector(FVector::ForwardVector);
         DataPack.ViewerEyePoint = eyeViewPoint;
         DataPack.ViewerEyeDirection = eyeDirection;
         DataPack.ViewerEyeUp = eyeRotation.RotateVector(FVector::UpVector);
      }
   }
}

void FGameplayDebuggerCategory_TATStealth::DrawData(APlayerController* ownerPC, FGameplayDebuggerCanvasContext& canvasContext)
{
   float elapsed = 60.0f;
   // Note that local time might have some network drift from wherever this timestamp was collected.
   // So, it's only accurate to within however accurate our current network synchronization is.
   if (UWorld* world = ownerPC->GetWorld())
   {
      elapsed = world->TimeSeconds - DataPack.TimeStamp;
   }

   // Does the visualization based on the data in the DataPack. See GameplayDebuggerCategory_BehaviorTree for an example.
   canvasContext.Printf(TEXT("Detection rate = %.2f %s"), DataPack.DetectionRate, elapsed > 0.5f ? TEXT("(Not current)") : TEXT(""));

   canvasContext.Printf(TEXT("Distance multiplier = %5.2f   Distance = %.2f"), DataPack.DistanceMultiplier, DataPack.Distance);

   canvasContext.Printf(TEXT("Time stim last perceived = %.2f"), DataPack.TimeStimLastPerceived);

   canvasContext.Printf(TEXT("Alertness Level = %s"), *DataPack.AlertnessLevel);

   // Draw eye direction.
   DrawDebugLine(ownerPC->GetWorld(), DataPack.ViewerEyePoint, DataPack.ViewerEyePoint + 500.0f * DataPack.ViewerEyeDirection,
      FColor::Red, false, -1.0f, 0, 3.0f);
   // Draw eye _up_ direction, to make sure it's not rolled unexpectedly.
   DrawDebugLine(ownerPC->GetWorld(), DataPack.ViewerEyePoint, DataPack.ViewerEyePoint + 100.0f * DataPack.ViewerEyeUp,
      FColor::Blue, false, -1.0f, 0, 2.0f);
}

void FGameplayDebuggerCategory_TATStealth::DrawFilledBox(FGameplayDebuggerCanvasContext& canvasContext, FVector2D position, FVector2D size, FLinearColor color)
{
   if (size.X < 0.0f)
   {
      size.X = -size.X;
      position.X -= size.X;
   }
   if (size.Y < 0.0f)
   {
      size.Y = -size.Y;
      position.Y -= size.Y;
   }

   // Filled box is just a thick line, either horizontal (if width>height) or vertical (otherwise)
   float lineWidth = FMath::Min(size.X, size.Y);
   position += 0.5f * lineWidth * FVector2D::UnitVector; // not actually a unit vector, but (1,1)
   if (size.X > size.Y)
   {
      size.X -= lineWidth;
      size.Y = 0.0f;
   }
   else
   {
      size.Y -= lineWidth;
      size.X = 0.0f;
   }
   // The initialized position of this doesn't matter, since it (maybe surprisingly) just gets
   // overridden in the call to FGameplayDebuggerCanvasContext::DrawItem().
   FCanvasLineItem line = FCanvasLineItem(FVector2D::ZeroVector, position + size);
   line.LineThickness = lineWidth;
   line.SetColor(color);
   canvasContext.DrawItem(line, position.X, position.Y);
}

void FGameplayDebuggerCategory_TATStealth::DrawGraphBar(FGameplayDebuggerCanvasContext& canvasContext, float rangeWidth, float minValue, float maxValue, float value, float margin)
{
   canvasContext.CursorX += margin;
   if (rangeWidth <= 0.0f || minValue >= maxValue)
   {
      canvasContext.CursorX += margin;
      return;
   }

   value = FMath::Clamp(value, minValue, maxValue);

   const float pixelsPerValue = rangeWidth / (maxValue - minValue);
   const float barHeight = canvasContext.GetLineHeight();
   const float xPosLeft = canvasContext.CursorX;
   const float yPos = canvasContext.CursorY;
   const float zeroXPos = xPosLeft + (-minValue) * pixelsPerValue;
   float barWidth = value * pixelsPerValue;
   float xPos = zeroXPos;
   FLinearColor color = FLinearColor::Green;
   if (barWidth < 0)
   {
      barWidth = -barWidth;
      xPos -= barWidth;
      color = FLinearColor::Red;
   }

   DrawFilledBox(canvasContext, FVector2D(xPos, yPos), FVector2D(barWidth, barHeight), value >= 0.0f ? FLinearColor::Green: FLinearColor::Red);

   canvasContext.CursorX = xPosLeft + rangeWidth + margin;
}

#endif // WITH_GAMEPLAY_DEBUGGER
