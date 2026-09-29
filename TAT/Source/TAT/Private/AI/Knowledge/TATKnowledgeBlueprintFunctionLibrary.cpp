// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Knowledge/TATKnowledgeBlueprintFunctionLibrary.h"

// tat
#include "AI/TATKnowledgeComponent.h"
#include "AI/Utility/UtilityAIComponent.h"
#include "Character/TATCharacterAIBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATKnowledgeBlueprintFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogTATKnowledgeBlueprintFunctionLibrary, Log, All)

namespace SharedTargetHelpers
{
   
   static FTATSharedTarget MakeSharedTargetFromActor(
      AActor* target,
      const FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass)
   {
      if (target == nullptr)
      {
         UE_LOG(LogTATKnowledgeBlueprintFunctionLibrary, Error, TEXT("MakeSharedTargetFromUtilityTarget called with a null 'target'!"));
         return FTATSharedTarget();
      }

      FTATSharedTarget sharedTarget;
      sharedTarget.TargetActor = target;
      sharedTarget.TypeTag = sharedTargetType;
      sharedTarget.PartnerClass = partnerClass;
      return sharedTarget;
   }
   static FTATSharedTarget MakeSharedTargetFromUtilityTarget(
      const UUtilityAIComponent* utilityAIComponent,
      const FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass)
   {
      if (utilityAIComponent == nullptr)
      {
         UE_LOG(LogTATKnowledgeBlueprintFunctionLibrary, Error, TEXT("MakeSharedTargetFromUtilityTarget called with a null 'utilityAIComponent'!"));
         return FTATSharedTarget();
      }

      FTATSharedTarget sharedTarget;
      const FUtilityStateTarget& currentTarget = utilityAIComponent->GetCurrentTarget();
      sharedTarget.TargetStimId = currentTarget.Stim.Id;
      sharedTarget.TargetActor = currentTarget.Actor;
      sharedTarget.TypeTag = sharedTargetType;
      sharedTarget.PartnerClass = partnerClass;
      return sharedTarget;
   }

   static FTATSharedTarget MakeSharedTargetFromLocation(
      const FVector location,
      const FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass)
   {
      FTATSharedTarget sharedTarget;
      sharedTarget.TargetLocation = location;
      sharedTarget.TypeTag = sharedTargetType;
      sharedTarget.PartnerClass = partnerClass;
      return sharedTarget;
   }
}

bool UTATKnowledgeBlueprintFunctionLibrary::GetSharedTargetActor(const FTATSharedTarget& sharedTarget, AActor*& outActor)
{
   if (AActor* targetActor = sharedTarget.TargetActor.Get())
   {
      outActor = targetActor;
      return true;
   }
   else if (sharedTarget.TargetStimId != INDEX_NONE)
   {
      UTATKnowledgeComponent* knowledgeComponent = sharedTarget.Instigator.Get();
      check(knowledgeComponent);

      outActor = knowledgeComponent->GetStimInfo(sharedTarget.TargetStimId).Instigator.Get();
      return true;
   }

   outActor = nullptr;
   return false;
}

bool UTATKnowledgeBlueprintFunctionLibrary::GetSharedTargetLocation(const FTATSharedTarget& sharedTarget, FVector& outLocation)
{
   if (AActor* targetActor = sharedTarget.TargetActor.Get())
   {
      UTATKnowledgeComponent* knowledgeComponent = sharedTarget.Instigator.Get();
      check(knowledgeComponent);

      return knowledgeComponent->GetLastKnownActorLocation(targetActor, outLocation);
   }
   else if (sharedTarget.TargetStimId != INDEX_NONE)
   {
      UTATKnowledgeComponent* knowledgeComponent = sharedTarget.Instigator.Get();
      check(knowledgeComponent);

      const FStimInfo& stim = knowledgeComponent->GetStimInfo(sharedTarget.TargetStimId);
      outLocation = stim.Location;
      return stim.IsValid();
   }
   else if (sharedTarget.TargetLocation.IsSet())
   {
      outLocation = sharedTarget.TargetLocation.GetValue();
      return true;
   }

   outLocation = FVector::ZeroVector;
   return false;
}

const FGameplayTag& UTATKnowledgeBlueprintFunctionLibrary::GetSharedTargetTypeTag(const FTATSharedTarget& sharedTarget)
{
   return sharedTarget.TypeTag;
}

const FGameplayTagContainer& UTATKnowledgeBlueprintFunctionLibrary::GetSharedTargetContextTags(const FTATSharedTarget& sharedTarget)
{
   return sharedTarget.ContextTags;
}

ATATCharacterAIBase* UTATKnowledgeBlueprintFunctionLibrary::GetSharedTargetInstigator(const FTATSharedTarget& sharedTarget)
{
   const UTATKnowledgeComponent* instigatorKnowlecgeComponent = sharedTarget.Instigator.Get();
   return (instigatorKnowlecgeComponent != nullptr) ? instigatorKnowlecgeComponent->GetAICharacter() : nullptr;
}

bool UTATKnowledgeBlueprintFunctionLibrary::RememberTargetToShare(
   UTATKnowledgeComponent* knowledgeComponent, 
   const UUtilityAIComponent* utilityAIComponent, 
   const FGameplayTag sharedTargetType, 
   TSoftClassPtr<ATATCharacterAIBase> partnerClass)
{
   if (knowledgeComponent == nullptr)
   {
      UE_LOG(LogTATKnowledgeBlueprintFunctionLibrary, Error, TEXT("RememberTargetToShare called with a null 'knowledgeComponent'!"));
      return false;
   }

   const FTATSharedTarget sharedTarget = SharedTargetHelpers::MakeSharedTargetFromUtilityTarget(utilityAIComponent, sharedTargetType, partnerClass);
   return knowledgeComponent->RememberTargetToShare(sharedTarget);
}

bool UTATKnowledgeBlueprintFunctionLibrary::RememberExplicitTargetToShare(UTATKnowledgeComponent* knowledgeComponent, AActor* target, FGameplayTag sharedTargetType, TSoftClassPtr<ATATCharacterAIBase> partnerClass)
{
   if (knowledgeComponent == nullptr)
   {
      UE_LOG(LogTATKnowledgeBlueprintFunctionLibrary, Error, TEXT("RememberExplicitTargetToShare called with a null 'knowledgeComponent'!"));
      return false;
   }
   const FTATSharedTarget sharedTarget = SharedTargetHelpers::MakeSharedTargetFromActor(target, sharedTargetType, partnerClass);
   return knowledgeComponent->RememberTargetToShare(sharedTarget);
}

bool UTATKnowledgeBlueprintFunctionLibrary::RememberLocationToShare(
   UTATKnowledgeComponent* knowledgeComponent,
   const FVector location,
   FGameplayTag sharedTargetType,
   TSoftClassPtr<ATATCharacterAIBase> partnerClass)
{
   if (knowledgeComponent == nullptr)
   {
      UE_LOG(LogTATKnowledgeBlueprintFunctionLibrary, Error, TEXT("RememberLocationToShare called with a null 'knowledgeComponent'!"));
      return false;
   }

   const FTATSharedTarget sharedTarget = SharedTargetHelpers::MakeSharedTargetFromLocation(location, sharedTargetType, partnerClass);
   return knowledgeComponent->RememberTargetToShare(sharedTarget);
}
