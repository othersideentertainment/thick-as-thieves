// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "Perception/AISense.h"

// ose
#include "Character/OSETeamInterface.h"

#include "OSEAISense_Team.generated.h"

class UOSEAISense_Team;

USTRUCT()
struct OSEAI_API FOSEAITeamStimulusEvent
{   
   GENERATED_USTRUCT_BODY()

   typedef UOSEAISense_Team FSenseClass;

   FVector LastKnowLocation;
private:
   FVector BroadcastLocation;
public:
   float RangeSq;
   float InformationAge;
   EOSETeamAttitude TeamAttitude;
   float Strength;
private:
   UPROPERTY()
   AActor* Broadcaster;
public:
   UPROPERTY()
   AActor* Target;
      
   FOSEAITeamStimulusEvent() : Broadcaster(nullptr), Target(nullptr) {}
   FOSEAITeamStimulusEvent(AActor* inBroadcaster, AActor* inTarget, const FVector& inLastKnowLocation, float eventRange, float passedInfoAge = 0.f, float inStrength = 1.f);

   FORCEINLINE void CacheBroadcastLocation()
   {
      BroadcastLocation = Broadcaster ? Broadcaster->GetActorLocation() : FAISystem::InvalidLocation;
   }

   FORCEINLINE const FVector& GetBroadcastLocation() const 
   {
      return BroadcastLocation;
   }
};

UCLASS(ClassGroup=AI)
class OSEAI_API UOSEAISense_Team : public UAISense
{
   GENERATED_UCLASS_BODY()

   UPROPERTY()
   TArray<FOSEAITeamStimulusEvent> RegisteredEvents;

public:      
   void RegisterEvent(const FOSEAITeamStimulusEvent& inEvent);   

protected:
   virtual float Update() override;
};
