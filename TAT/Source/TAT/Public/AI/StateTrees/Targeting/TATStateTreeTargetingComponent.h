// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/TATKnowledgeComponent.h"
#include "AI/StateTrees/TATStateTreeAIComponent.h"
#include "TATStateTreeTargetingTypes.h"

// ue
#include "Components/ActorComponent.h"

#include "TATStateTreeTargetingComponent.generated.h"


struct FTATHearingEventStimSettings;
DECLARE_LOG_CATEGORY_EXTERN(LogTATStateTreeTargeting, Log, All);

USTRUCT()
struct FTATStateTreeTargetingCalculationGroup
{
   GENERATED_BODY()

   float BestScore { 0.f };
   TWeakObjectPtr<AActor> BestTargetActor { nullptr };
   double TimeSet { 0 };

   UPROPERTY()
   UTATStateTreeTargetingConsiderations* TargetingConsiderations { nullptr };
   
   FTATStateTreeTargetingCalculationGroup() = default;
   FTATStateTreeTargetingCalculationGroup(UTATStateTreeTargetingConsiderations* targetingConsiderations)
   {
      TargetingConsiderations = targetingConsiderations;
      BestScore = 0.f;
      BestTargetActor = nullptr;
   }

   void CalculateScoreForTargetActor(AOSEAIController* controller, AOSECharacterBase* character, 
      const FTATStateTreeTargetingConsiderationTargetContext& targetContext);
   void ConsumeBestTarget() { _BestTargetChanged = false; }
   void SetBestTargetChanged() { _BestTargetChanged = true; }
   bool HasBestTargetChanged() const { return _BestTargetChanged; }
   void ResetBestScore(bool forceEvent);

private:
   bool _BestTargetChanged { false };
   void _SetBestTarget(const float score, AActor* target, double worldTime);;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATStateTreeTargetingComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATStateTreeTargetingComponent();

   void AddTargetingConsiderations(UTATStateTreeTargetingConsiderations* asset);
   void RemoveTargetingConsiderations(UTATStateTreeTargetingConsiderations* asset);

   // Returns the higher priority of the two supplied stims.
   static const FStimInfo& DetermineHighestPriorityStim(const FStimInfo& stimA, const FStimInfo& stimB);

   void ForceBestTargetEventsToRetrigger();
   bool GetStimDataForTag(FName tag, FTATHearingEventStimSettings& outStimSettings) const;

protected:
   void _DispatchStimEventToStateTree(const FStimInfo& stimInfo);

   UFUNCTION()
   void OnStimAdded(FStimInfo& stimInfo);
   
   UFUNCTION()
   void OnActorKnowledgeRemoved(const FTATActorKnowledge& tatActorKnowledge);

   virtual void BeginPlay() override;
   
   UPROPERTY(Transient)
   UTATKnowledgeComponent* _KnowledgeComponent { nullptr };
   UPROPERTY(Transient)
   UOSEStimDatabase* _StimDatabase { nullptr };

   UPROPERTY(Transient)
   ATATAIController* _TATAIController { nullptr };

   UPROPERTY(Transient)
   TArray<FTATStateTreeTargetingCalculationGroup> _TargetingCalculationGroups;

   int _CurrentTargetingCalculationGroupIndex { INDEX_NONE };

public:
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintCallable)
   AActor* GetBestTargetForTargetingGroup(const FGameplayTag& targetingGroup) const;
   float GetTimeTargetSetForTargetingGroup(const FGameplayTag& targetingGroup) const;
   
   const TArray<FStimInfo>& GetCurrentlyProcessedStimInfo() const;
   void ConsumeCurrentProcessedStims();

private:
   TArray<FStimInfo> _CurrentlyProcessedStims;
   int32 _GetTargetingGroupIndex(UTATStateTreeTargetingConsiderations* asset) const;
};
