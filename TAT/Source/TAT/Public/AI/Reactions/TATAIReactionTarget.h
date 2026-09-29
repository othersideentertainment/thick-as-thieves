// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/TATKnowledgeComponent.h"

// ose
#include "AI/Perception/StimInfo.h"

// ue
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"

#include "TATAIReactionTarget.generated.h"

struct FTATAIReactionTarget;

UENUM()
enum class EReactionTargetType : uint8
{
   None UMETA(Hidden),
   Actor,
   HearingStim
};

// Content authoring capability for defining the targeted stims/actor for a reaction event.
USTRUCT(BlueprintType)
struct FTATAIReactionTargetConfig
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EReactionTargetType TargetType = EReactionTargetType::None;

   // Specifies a type of actor that is the target of this reaction event.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "TargetType == EReactionTargetType::Actor", EditConditionHides))
   TSoftClassPtr<AActor> TargetActorType = nullptr;

   // Specifies the stim tag that is the target of this reaction event.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "TargetType == EReactionTargetType::HearingStim", EditConditionHides, Categories = "AI.Stim.Hearing"))
   FGameplayTag TargetHearingStimType;

   // Specifies the set of stims with some severity that are the target of this reaction event.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "TargetType == EReactionTargetType::HearingStim", EditConditionHides))
   EStimSeverity TargetHearingStimSeverity = EStimSeverity::None;

   // Returns true of the provided reaction target instance matches this configuration's criteria.
   bool TargetInstanceMeetsCriteria(const FTATAIReactionTarget& target) const;

   friend uint32 GetTypeHash(const FTATAIReactionTargetConfig& target)
   {
      switch (target.TargetType)
      {
         case EReactionTargetType::Actor: return GetTypeHash(target.TargetActorType);
         case EReactionTargetType::HearingStim: 
            return HashCombine(GetTypeHash(target.TargetHearingStimType), GetTypeHash(target.TargetHearingStimSeverity));
         default: return 0;
      }
   }

   bool operator==(const FTATAIReactionTargetConfig& reactionTarget) const
   {
      switch (reactionTarget.TargetType)
      {
         case EReactionTargetType::Actor: return (TargetActorType == reactionTarget.TargetActorType);
         case EReactionTargetType::HearingStim: 
            return (TargetHearingStimType == reactionTarget.TargetHearingStimType) && 
               (TargetHearingStimSeverity == reactionTarget.TargetHearingStimSeverity);
         default: return false;
      }
   }
};

// Instance of the stim/actor target of a reaction event.
struct FTATAIReactionTarget
{
private:
   // Privatize valid target constructors to enforce usage of static Generate methods.
   FTATAIReactionTarget(const AActor* actor)
      : Type(EReactionTargetType::Actor), Actor(actor) { }
   FTATAIReactionTarget(const EStimSeverity stimSeverity, const int globalStimId, const FGameplayTag stimType, const FVector& stimLocation)
      : Type(EReactionTargetType::HearingStim), GlobalStimId(globalStimId), StimType(stimType), StimLocation(stimLocation), StimSeverity(stimSeverity) { }

public:
   FTATAIReactionTarget(const FTATAIReactionTarget& other)
      : Type(other.Type)
      , Actor(other.Actor)
      , GlobalStimId(other.GlobalStimId)
      , StimType(other.StimType)
      , StimLocation(other.StimLocation)
      , StimSeverity(other.StimSeverity)
      , _projectedLocation(other._projectedLocation)
   {
   }

   FTATAIReactionTarget& operator=(const FTATAIReactionTarget& other)
   {
      if (this == &other)
         return *this;
      Type = other.Type;
      Actor = other.Actor;
      GlobalStimId = other.GlobalStimId;
      StimType = other.StimType;
      StimLocation = other.StimLocation;
      StimSeverity = other.StimSeverity;
      _projectedLocation = other._projectedLocation;
      return *this;
   }

   FTATAIReactionTarget& operator=(FTATAIReactionTarget&& other) noexcept
   {
      if (this == &other)
         return *this;
      Type = other.Type;
      Actor = std::move(other.Actor);
      GlobalStimId = other.GlobalStimId;
      StimType = std::move(other.StimType);
      StimLocation = std::move(other.StimLocation);
      StimSeverity = other.StimSeverity;
      _projectedLocation = std::move(other._projectedLocation);
      return *this;
   }

   // Invalid target constructor.
   FTATAIReactionTarget() { }
   static FTATAIReactionTarget GenerateForStim(const ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim);
   static FTATAIReactionTarget GenerateForActor(const AActor* actor);

   // Defines which of the following fields will/will not be filled.
   EReactionTargetType Type = EReactionTargetType::None;

   // A "reaction target" can be either an actor or a stim, but not both.
   TWeakObjectPtr<const AActor> Actor = nullptr;

   // Used for comparisons to reaction targets generated for different AIs.
   // Normal stim Ids coming from them are locally generated and thus won't always match.
   int GlobalStimId = INDEX_NONE;

   // Tagged type of the target stim, set only if Type == HearingStim.
   FGameplayTag StimType = FGameplayTag::EmptyTag;

   // Location of the target stim, set only if Type == HearingStim.
   FVector StimLocation = FVector::ZeroVector;

   // Severity of the target stim, set only if Type == HearingStim.
   EStimSeverity StimSeverity = EStimSeverity::None;

   // Note that validity here does not check that a stim is still being tracked.
   bool IsValid() const;

   FVector GetLocation() const;
   FVector GetNavMeshProjectedLocation(const UObject* worldContextObject) const;

   FString ToString() const;

   bool operator==(const FTATAIReactionTarget& target) const
   {
      if(Type != target.Type)
         return false;
      switch (target.Type)
      {
         case EReactionTargetType::Actor: return (Actor == target.Actor);
         case EReactionTargetType::HearingStim: return (GlobalStimId == target.GlobalStimId);
         default: return false;
      }
   }

   friend uint32 GetTypeHash(const FTATAIReactionTarget& target)
   {
      switch (target.Type)
      {
         case EReactionTargetType::Actor: return GetTypeHash(target.Actor);
         case EReactionTargetType::HearingStim: return GetTypeHash(target.GlobalStimId);
         default: return 0;
      }
   }

private:
   // Location projected to the navmesh, cached to avoid consecutive
   // ProjectPointToNavigation calls.
   mutable TOptional<FVector> _projectedLocation;
};
