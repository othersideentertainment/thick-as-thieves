// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionFunctionLibrary.h"

// tat
#include "AI/Reactions/TATAIReactionCoordinator.h"
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "AI/Utility/UtilityAIStateTarget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogTATAIReactionFunctionLibrary, Log, All);

bool UTATAIReactionFunctionLibrary::TryRegisterForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
   ATATCharacterAIBase* reactingAI, FGameplayTag& outRoleTag)
{
   outRoleTag = FGameplayTag::EmptyTag;

   if (reactingAI == nullptr)
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error, 
         TEXT("TryRegisterForUtilityTargetReactionRole called with a null 'reactingAI'!"));
      return false;
   }

   if (UTATAIReactionCoordinatorSubsystem* airc = reactingAI->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (const AActor* targetActor = utilityTarget.Actor.Get())
      {
         return airc->RegisterAIForActorTarget(reactingAI, targetActor, outRoleTag);
      }
      else if (utilityTarget.Stim.IsValid())
      {
         return airc->RegisterAIForStimTarget(reactingAI, utilityTarget.Stim, outRoleTag);
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(utilityTarget.GetTargetUObject()))
      {
         return airc->RegisterAIForActorTarget(reactingAI, smartObjComp->GetOwner(), outRoleTag);
      }

      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("TryRegisterForUtilityTargetReactionRole called on %s utility target, only supports Actor/Stim/Smart Object!"),
         *UEnum::GetValueAsString(utilityTarget.TargetType));
      return false;
   }
   else
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("TryRegisterForUtilityTargetReactionRole failed to retrieve UTATAIReactionCoordinatorSubsystem!"));
      return false;
   }
}

bool UTATAIReactionFunctionLibrary::TryUnregisterForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
   ATATCharacterAIBase* reactingAI)
{
   if (reactingAI == nullptr)
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("TryUnregisterForUtilityTargetReactionRole called with a null 'reactingAI'!"));
      return false;
   }

   if (UTATAIReactionCoordinatorSubsystem* airc = reactingAI->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (const AActor* targetActor = utilityTarget.Actor.Get())
      {
         return airc->UnregisterActorForActorTarget(reactingAI, targetActor);
      }
      else if (utilityTarget.Stim.IsValid())
      {
         return airc->UnregisterActorForStimTarget(reactingAI, utilityTarget.Stim);
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(utilityTarget.GetTargetUObject()))
      {
         return airc->UnregisterActorForActorTarget(reactingAI, smartObjComp->GetOwner());
      }

      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("TryUnregisterForUtilityTargetReactionRole called on %s utility target, only supports Actor/Stim/Smart Object!"),
         *UEnum::GetValueAsString(utilityTarget.TargetType));
      return false;
   }
   else
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("TryUnregisterForUtilityTargetReactionRole failed to retrieve UTATAIReactionCoordinatorSubsystem!"));
      return false;
   }
}

bool UTATAIReactionFunctionLibrary::IsRegisteredForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
   ATATCharacterAIBase* reactingAI, const FGameplayTag& roleTag)
{
   if (reactingAI == nullptr)
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("IsRegisteredForUtilityTargetReactionRole called with a null 'reactingAI'!"));
      return false;
   }

   if (UTATAIReactionCoordinatorSubsystem* airc = reactingAI->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (const AActor* targetActor = utilityTarget.Actor.Get())
      {
         const FTATAIReactionTarget reactionTarget = FTATAIReactionTarget::GenerateForActor(targetActor);
         return airc->IsAIRegisteredForRole(reactingAI, reactionTarget, roleTag);
      }
      else if (utilityTarget.Stim.IsValid())
      {
         const FTATAIReactionTarget reactionTarget = FTATAIReactionTarget::GenerateForStim(reactingAI, utilityTarget.Stim);
         return airc->IsAIRegisteredForRole(reactingAI, reactionTarget, roleTag);
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(utilityTarget.GetTargetUObject()))
      {
         const FTATAIReactionTarget reactionTarget = FTATAIReactionTarget::GenerateForActor(smartObjComp->GetOwner());
         return airc->IsAIRegisteredForRole(reactingAI, reactionTarget, roleTag);
      }

      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("IsRegisteredForUtilityTargetReactionRole called on %s utility target, only supports Actor/Stim/Smart Object!"),
         *UEnum::GetValueAsString(utilityTarget.TargetType));
      return false;
   }
   else
   {
      UE_LOG(LogTATAIReactionFunctionLibrary, Error,
         TEXT("IsRegisteredForUtilityTargetReactionRole failed to retrieve UTATAIReactionCoordinatorSubsystem!"));
      return false;
   }
}
