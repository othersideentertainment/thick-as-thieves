// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/TATAISenseConfig_Hearing.h"

// ue4
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAISenseConfig_Hearing)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

//----------------------------------------------------------------------
// UTATAISenseConfig_Hearing
//----------------------------------------------------------------------

UTATAISenseConfig_Hearing::UTATAISenseConfig_Hearing(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , HearingRange(3000.f)
{
   DebugColor = FColor::Yellow;
}

TSubclassOf<UAISense> UTATAISenseConfig_Hearing::GetSenseImplementation() const
{
   return UTATAISense_Hearing::StaticClass();
}

#if WITH_GAMEPLAY_DEBUGGER
static FString DescribeColorHelper(const FColor& color)
{
   const int32 maxColors = GColorList.GetColorsNum();
   for (int32 idx = 0; idx < maxColors; idx++)
   {
      if (color == GColorList.GetFColorByIndex(idx))
      {
         return GColorList.GetColorNameByIndex(idx);
      }
   }
   return FString(TEXT("color"));
}

void UTATAISenseConfig_Hearing::DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   if (!perceptionComponent || !debuggerCategory)
   {
      return;
   }

   FColor hearingRangeColor = FColor::Yellow;

   // don't call Super implementation on purpose, replace color description line
   debuggerCategory->AddTextLine(
      FString::Printf(TEXT("%s: {%s}%s {white}range:{%s}%s"), *GetSenseName(),
         *GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
         *hearingRangeColor.ToString(), *DescribeColorHelper(hearingRangeColor))
   );

   const AActor* bodyActor = perceptionComponent->GetBodyActor();
   if (bodyActor)
   {
      FVector ownerLocation = bodyActor->GetActorLocation();

      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeCylinder(ownerLocation, HearingRange, 25.0f, hearingRangeColor));
   }
}
#endif // WITH_GAMEPLAY_DEBUGGER

