// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityTask.h"
#include "VoiceOver/OSEVoiceOverLineRequestParams.h"

#include "AbilityTask_EventTimeLine.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAbilityTaskTimeLine, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEventTimelineDelegate);

UCLASS(BlueprintType, abstract, EditInlineNew)
class OSECORE_API UEventTimelineAction : public UObject
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const { }

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float Time = 0.0f;
};

UCLASS(BlueprintType)
class OSECORE_API UEventTimelineActionApplyEffect : public UEventTimelineAction
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSubclassOf<UGameplayEffect> GameplayEffectClass;
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TMap<FGameplayTag, float> SetByCallerTagMagnitudes;
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool UseOtherActorAsInstigator = false;
};

UCLASS(BlueprintType)
class OSECORE_API UEventTimelineActionRemoveEffect : public UEventTimelineAction
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSubclassOf<UGameplayEffect> GameplayEffectClass;
};

UCLASS(BlueprintType)
class OSECORE_API UEventTimelineActionTriggerVO : public UEventTimelineAction
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEVoiceOverLineRequestParams VoiceOverParams;
};

UCLASS(BlueprintType, EditInlineNew)
class OSECORE_API UAbilityTask_EventTimeline : public UOSEAbilityTask
{
   GENERATED_BODY()

public:

   UPROPERTY(BlueprintAssignable)
   FEventTimelineDelegate OnCompleted;

   UAbilityTask_EventTimeline(const FObjectInitializer& objectInitializer);

   virtual void TickTask(float deltaTime) override;
   virtual void Activate() override;
   virtual void OnDestroy(bool inOwnerFinished) override;

   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_EventTimeline* StartEventTimeLine(UGameplayAbility* owningAbility, FName taskInstanceName, const UAbilityTask_EventTimeline* templateTask, AActor* otherActor);

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> ActivationActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> TimelineActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> ShutdownActions;

   // If we're the source, this is a reference to the target actor, and vice-versa
   UPROPERTY(Transient, BlueprintReadOnly)
   AActor* OtherActor = nullptr;

private:

   float _startTime = 0.0f;
   float _timeStamp = 0.0f;
};

