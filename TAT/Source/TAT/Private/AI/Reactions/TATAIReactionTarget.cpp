// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionTarget.h"

// tat
#include "AI/TATAIController.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "NavigationSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionTarget)
DEFINE_LOG_CATEGORY_STATIC(LogTATAIReactionTargetConfig, Log, All);

namespace ReactionTargetHelpers
{
   static float kProjectionXY = 400.0f;
   static float kProjectionZ = 1000.0f;

   FORCEINLINE static FVector GetProjectedExtents()
   {
      return FVector(kProjectionXY, kProjectionXY, kProjectionZ);
   }

   FORCEINLINE static FVector GetProjectedOffset()
   {
      return FVector(0, 0, kProjectionZ / -2.0f);
   }
}

FTATAIReactionTarget FTATAIReactionTarget::GenerateForStim(const ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim)
{
   if (aiCharacter == nullptr)
   {
      UE_LOG(LogTATAIReactionTargetConfig, Error,
         TEXT("Failed to generate a valid stim reaction target : aiCharacter was null!"));
      return FTATAIReactionTarget();
   }

   const ATATAIController* aiController = aiCharacter->GetController<ATATAIController>();
   if (aiController == nullptr)
   {
      UE_LOG(LogTATAIReactionTargetConfig, Error,
         TEXT("Failed to generate a valid stim reaction target : Could not find AI controller!"));
      return FTATAIReactionTarget();
   }

   const UTATKnowledgeComponent* aiKnowledge = aiController->GetTATKnowledgeComponent();
   if (aiKnowledge == nullptr)
   {
      UE_LOG(LogTATAIReactionTargetConfig, Error,
         TEXT("Failed to generate a valid stim reaction target : Could not find knowledge component!"));
      return FTATAIReactionTarget();
   }

   const FGameplayTag stimType = aiKnowledge->GetHearingStimTag(localStim.Tag);
   return FTATAIReactionTarget(
      localStim.Severity,
      localStim.GlobalId,
      stimType,
      localStim.Location);
}

FTATAIReactionTarget FTATAIReactionTarget::GenerateForActor(const AActor* actor)
{
   if (actor == nullptr)
   {
      UE_LOG(LogTATAIReactionTargetConfig, Error,
         TEXT("Failed to generate a valid actor reaction target : actor was null!"));
      return FTATAIReactionTarget();
   }

   return FTATAIReactionTarget(actor);
}

bool FTATAIReactionTargetConfig::TargetInstanceMeetsCriteria(const FTATAIReactionTarget& target) const
{
   if (TargetType != target.Type)
   {
      return false;
   }

   switch (TargetType)
   {
      case EReactionTargetType::Actor:
      {
         if (const AActor* targetActor = target.Actor.Get())
         {
            return targetActor->IsA(TargetActorType.Get());
         }

         UE_LOG(LogTATAIReactionTargetConfig, Error,
            TEXT("Reaction Target passed to TargetInstanceMeetsCriteria with a null actor!"));
         return false;
      }
      case EReactionTargetType::HearingStim:
      {
         // While both the type tag and severity will be set on the reaction target instance,
         // in most cases only one of these two values will be set in the config. As a result,
         // we should only check for a match on the one that is actually defined in the config.
         const bool shouldCheckType = TargetHearingStimType.IsValid();
         const bool shouldCheckSeverity = TargetHearingStimSeverity != EStimSeverity::None;
         return (!shouldCheckType || TargetHearingStimType.MatchesTagExact(target.StimType))
            && (!shouldCheckSeverity || TargetHearingStimSeverity == target.StimSeverity);
      }
      default:
      {
         UE_LOG(LogTATAIReactionTargetConfig, Error,
            TEXT("TargetInstanceMeetsCriteria called on target with invalid Type, %s."),
            *UEnum::GetValueAsString(target.Type));
         return false;
      } 
   }
}

bool FTATAIReactionTarget::IsValid() const
{
   switch (Type)
   {
      case EReactionTargetType::Actor:
      {
         return Actor.IsValid();
      }
      case EReactionTargetType::HearingStim:
      {
         return (GlobalStimId != INDEX_NONE);
      }
      default: return false;
   }
}

FVector FTATAIReactionTarget::GetLocation() const
{
   switch (Type)
   {
      case EReactionTargetType::Actor:
      {
         if (const AActor* actor = Actor.Get())
         {
            return actor->GetActorLocation();
         }
      }
      case EReactionTargetType::HearingStim:
      {
         return StimLocation;
      }
      default: return FVector::ZeroVector;
   }
}

FVector FTATAIReactionTarget::GetNavMeshProjectedLocation(const UObject* worldContextObject) const
{
   if (!_projectedLocation.IsSet())
   {
      _projectedLocation = GetLocation();

      check(worldContextObject);
      const UWorld* world = worldContextObject->GetWorld();
      if (const UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world))
      {
         FNavLocation projectedLocation;
         if (navSys->ProjectPointToNavigation(
            _projectedLocation.GetValue() + ReactionTargetHelpers::GetProjectedOffset(),
            projectedLocation, 
            ReactionTargetHelpers::GetProjectedExtents()))
         {
            _projectedLocation = projectedLocation.Location;
         }
      }
   }

   return _projectedLocation.GetValue();
}

FString FTATAIReactionTarget::ToString() const
{
   switch (Type)
   {
      case EReactionTargetType::Actor:
      {
         if (const AActor* actor = Actor.Get())
         {
            return FString::Printf(TEXT("{ Actor: %s }"),
               *actor->GetName());
         }
         return TEXT("{ INVALID ACTOR }");
      }
      case EReactionTargetType::HearingStim:
      {
         return FString::Printf(TEXT("{ Stim: %s, Id: %d, Severity: %s }"),
            *StimType.ToString(),
            GlobalStimId,
            *UEnum::GetValueAsString(StimSeverity));
      }
   }

   return TEXT("{ INVALID TARGET }");
}
