// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "Environment/TATPrivateSpaceCharacterComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"
#include "Developer/TATProjectSettings.h"
#include "Disguise/TATDisguisableCharacterInterface.h"
#include "Disguise/TATDisguiseComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"

// ose
#include "OSEIndividualAttitudesBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPrivateSpaceCharacterComponent)
DEFINE_LOG_CATEGORY_STATIC(LogAIPrivateSpace, Log, All)

UTATPrivateSpaceCharacterComponent::UTATPrivateSpaceCharacterComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

// static
UTATPrivateSpaceCharacterComponent* UTATPrivateSpaceCharacterComponent::TryGet(AActor* actor)
{
   APawn* actorPawn = nullptr;
   if (const AController* actorController = Cast<AController>(actor))
   {
      actorPawn = actorController->GetPawn();
   }
   else
   {
      actorPawn = Cast<APawn>(actor);
   }

   if (ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(actorPawn))
   {
      return privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent();
   }

   return nullptr;
}

void UTATPrivateSpaceCharacterComponent::AuthorityOnEnterPrivateSpaceVolume(const bool isOffLimits)
{
   if(isOffLimits)
   {
      _authorityInOffLimitSpaceCounter++;
   }
   else
   {
      _authorityInPrivateSpaceCounter++;
   }
   AuthorityUpdatePrivateSpaceTag();
}

void UTATPrivateSpaceCharacterComponent::AuthorityOnExitPrivateSpaceVolume(const bool isOffLimits)
{
   if(isOffLimits)
   {
      _authorityInOffLimitSpaceCounter--;
   }
   else
   {
      _authorityInPrivateSpaceCounter--;
   }
   AuthorityUpdatePrivateSpaceTag();
}

bool UTATPrivateSpaceCharacterComponent::_AuthorityIsInPrivateSpace() const
{
   check(GetOwner())
   check(GetOwner()->HasAuthority());
   return _authorityInPrivateSpaceCounter > 0 || _authorityInOffLimitSpaceCounter > 0;
}

void UTATPrivateSpaceCharacterComponent::AuthorityUpdatePrivateSpaceTag()
{
   check(GetOwner())
   check(GetOwner()->HasAuthority());
   if (_AuthorityIsInPrivateSpace())
   {
      if (!_privateSpaceEffectHandle.IsValid())
      {
         _AuthorityApplyPrivateSpaceGameplayEffect();
      }
   }
   else
   {
      if (_privateSpaceEffectHandle.IsValid())
      {
         _AuthorityRemovePrivateSpaceGameplayEffect();
      }
   }
}

void UTATPrivateSpaceCharacterComponent::AuthorityTryForceRemovalOfPrivateSpaceEffect()
{
   if(_privateSpaceEffectHandle.IsValid() == false)
      return;
   _AuthorityRemovePrivateSpaceGameplayEffect();
}

bool UTATPrivateSpaceCharacterComponent::AuthorityIsAllowedInPrivateZone(const FGameplayTag privateZoneTag) const
{
   if(GetOwner()->Implements<UTATDisguisableCharacterInterface>())
   {
      if(const UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(GetOwner()))
      {
         if(disguiseComponent->IsDisguiseActive())
         {
            // DG - Design stated that if disguised, you are implicitly allowed in private zones - regardless of the disguise
            return true;
         }
      }
   }
   if(AllowedPrivateZones.HasTag(privateZoneTag))
   {
      return true;
   }

   if(_temporaryAllowedPrivateZones.IsEmpty())
   {
      return false;
   }

   // Not even sure if matching subtags is interesting, but probably ought to match behavior of others
   for(FGameplayTag tag = privateZoneTag; tag.IsValid(); tag = tag.RequestDirectParent())
   {
      if(_temporaryAllowedPrivateZones.Contains(tag))
      {
         return true;
      }
   }
   return false;
}

void UTATPrivateSpaceCharacterComponent::AuthorityAddTemporaryAllowedPrivateZone(FGameplayTag privateZoneTag)
{
   int32& count = _temporaryAllowedPrivateZones.FindOrAdd(privateZoneTag);
   count += 1;
   if(count == 1)
   {
      OnTemporaryAllowedChanged.Broadcast(privateZoneTag, GetOwner());
   }
}

void UTATPrivateSpaceCharacterComponent::AuthorityRemoveTemporaryAllowedPrivateZone(FGameplayTag privateZoneTag)
{
   if(int32* count = _temporaryAllowedPrivateZones.Find(privateZoneTag))
   {
      if(*count > 1)
      {
         *count -= 1;
      }
      else
      {
         _temporaryAllowedPrivateZones.Remove(privateZoneTag);
         OnTemporaryAllowedChanged.Broadcast(privateZoneTag, GetOwner());
      }
   }
}

void UTATPrivateSpaceCharacterComponent::_ApplyGameplayEffect(FActiveGameplayEffectHandle& handle, const TSubclassOf<UGameplayEffect> gameplayEffectClass) const
{
   check(GetOwner())
   check(GetOwner()->HasAuthority());
   check(handle.IsValid() == false);
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      if (gameplayEffectClass)
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         effectContext.AddSourceObject(this);
         handle = asc->ApplyGameplayEffectToSelf(gameplayEffectClass.GetDefaultObject(), UGameplayEffect::INVALID_LEVEL, effectContext);
         UE_LOG(LogAIPrivateSpace, Verbose, TEXT("Applying Gameplay Effect %s"), *gameplayEffectClass->GetName());
      }
   }
}

void UTATPrivateSpaceCharacterComponent::_RemoveGameplayEffectByHandle(FActiveGameplayEffectHandle& handle) const
{
   check(GetOwner())
   check(GetOwner()->HasAuthority());
   if(handle.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         UE_LOG(LogAIPrivateSpace, Verbose, TEXT("Removing private space Effect"))
         asc->RemoveActiveGameplayEffect(handle);
      }
      handle.Invalidate();
   }
}
UTATKnowledgeComponent* GetKnowledgeComponentForPawn(const APawn* identifyingActor)
{
   check(identifyingActor);
   const ATATAIController* aiController = identifyingActor->GetController<ATATAIController>();
   check(aiController);
   
   UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   check(knowledgeComponent);
   return knowledgeComponent;
}
void UTATPrivateSpaceCharacterComponent::AuthorityIdentifiedAsIntruder(APawn* identifyingActor)
{
   if(_authorityInOffLimitSpaceCounter > 0 )
   {
      // If we are currently in an off-limit area, immediately cause the identifying actor to become hostile.
      AuthorityIdentifiedAsHostileIntruder(identifyingActor);
      return;
   }
   UTATKnowledgeComponent* knowledgeComponent = GetKnowledgeComponentForPawn(identifyingActor);
   knowledgeComponent->OnActorKnowledgeAboutToBeRemoved.RemoveAll(this);
   knowledgeComponent->OnActorKnowledgeAboutToBeRemoved.AddUObject(this,
      &ThisClass::_OnIdentifiedActorKnowledgeAboutToBeRemoved,
      identifyingActor);
   // Multiple guards may respond to an intruder, but we only want to apply the effect once.
   if(_identifiedIntruderEffectHandle.IsValid() == false)
   {
      _ApplyGameplayEffect(_identifiedIntruderEffectHandle, IdentifiedIntruderEffect);
   }
   _identifyingActor.AddUnique(identifyingActor);
   UE_LOG(LogAIPrivateSpace, Log, TEXT("Identified as intruder by %s"), *identifyingActor->GetName());
}

void UTATPrivateSpaceCharacterComponent::AuthorityIdentifiedAsHostileIntruder(APawn* identifyingActor)
{
   UTATKnowledgeComponent* knowledgeComponent = GetKnowledgeComponentForPawn(identifyingActor);
   knowledgeComponent->OnActorKnowledgeAboutToBeRemoved.RemoveAll(this);

   if(UTATProjectSettings::ShouldUseIndividualAttitudes())
   {
      UE_LOG(
         LogAIPrivateSpace,
         Log,
         TEXT("Identified as hostile by %s [Individual Attitudes]"),
         *identifyingActor->GetName()
      );
      UOSEIndividualAttitudesBlueprintFunctionLibrary::SetIndividualAttitude(
         identifyingActor,
         GetOwner(),
         EOSEIndividualAttitude::Hostile
      );
      return;
   }
   
   knowledgeComponent->OnActorKnowledgeAboutToBeRemoved.AddUObject(this,
      &ThisClass::_OnHostileActorKnowledgeAboutToBeRemoved,
      identifyingActor);
   
   // Multiple guards may respond to an intruder, but we only want to apply the effect once.
   if(_identifiedHostileIntruderEffectHandle.IsValid() == false)
   {
      _ApplyGameplayEffect(_identifiedHostileIntruderEffectHandle, IdentifiedHostileIntruderEffect);
   }
   _identifyingHostileActor.AddUnique(identifyingActor);
   UE_LOG(LogAIPrivateSpace, Log, TEXT("Identified as hostile by %s"), *identifyingActor->GetName());
}

void RemoveIdentifyingActorFromArray(TArray<TWeakObjectPtr<APawn>>& array, APawn* identifyingActor)
{
   array.Remove(identifyingActor);
   // TODO: If the pointers are cleaned up, we may have stale refs in here that never get cleaned out UNLESS another actor
   // loses sight of us first, I don't like this solution, but it will do _for now_
   array.RemoveAll([](const TWeakObjectPtr<APawn>& identifyingActor)
   {
      return !identifyingActor.IsValid();      
   });
}

void UTATPrivateSpaceCharacterComponent::AuthorityRemoveIdentifiedAsHostileIntruder(APawn* identifyingActor)
{
   UE_LOG(LogAIPrivateSpace, Log, TEXT("Removing hostile identifying pawn %s"), *identifyingActor->GetName());
   RemoveIdentifyingActorFromArray(_identifyingHostileActor, identifyingActor);
   if(_identifyingHostileActor.Num() == 0)
   {
      _RemoveGameplayEffectByHandle(_identifiedHostileIntruderEffectHandle);
   }
}

void UTATPrivateSpaceCharacterComponent::AuthorityRemoveIdentifyingActor(APawn* identifyingActor)
{
   UE_LOG(LogAIPrivateSpace, Log, TEXT("Removing identifying pawn %s"), *identifyingActor->GetName());
   RemoveIdentifyingActorFromArray(_identifyingActor, identifyingActor);
   if(_identifyingActor.Num() == 0)
   {
      _RemoveGameplayEffectByHandle(_identifiedIntruderEffectHandle);
   }
}

void UTATPrivateSpaceCharacterComponent::_OnHostileActorKnowledgeAboutToBeRemoved(
   const FTATActorKnowledge& knowledge,
   APawn* identifyingActor)
{
   if(knowledge.GetActor() != GetOwner())
   {
      // We only care about knowledge of our owner
      return;
   }
   AuthorityRemoveIdentifiedAsHostileIntruder(identifyingActor);
}

void UTATPrivateSpaceCharacterComponent::_OnIdentifiedActorKnowledgeAboutToBeRemoved(
   const FTATActorKnowledge& knowledge,
   APawn* identifyingActor)
{
   if(knowledge.GetActor() != GetOwner())
   {
      // We only care about knowledge of our owner
      return;
   }
   AuthorityRemoveIdentifyingActor(identifyingActor);
}

void UTATPrivateSpaceCharacterComponent::_AuthorityApplyPrivateSpaceGameplayEffect()
{
   _ApplyGameplayEffect(_privateSpaceEffectHandle, PrivateSpaceEffect);
}

void UTATPrivateSpaceCharacterComponent::_AuthorityRemovePrivateSpaceGameplayEffect()
{
   _RemoveGameplayEffectByHandle(_privateSpaceEffectHandle); 
   _RemoveGameplayEffectByHandle(_identifiedIntruderEffectHandle);
}

void UTATPrivateSpaceCharacterComponent::_ClearIdentifyingActors()
{
   _identifyingActor.Empty();
}
