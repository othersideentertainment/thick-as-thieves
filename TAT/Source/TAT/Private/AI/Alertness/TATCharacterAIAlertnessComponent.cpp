// (c) 2022-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Alertness/TATCharacterAIAlertnessComponent.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATAIFunctionLibrary.h"
#include "AI/TATAIStateWorldSubsystem.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Alertness/TATAlertnessAsset.h"
#include "AI/Alertness/TATAlertnessUtl.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "OSEProjectSettings.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterAIAlertnessComponent)

UTATCharacterAIAlertnessComponent::UTATCharacterAIAlertnessComponent()
{
}

void UTATCharacterAIAlertnessComponent::BeginPlay()
{
   Super::BeginPlay();
   
   if (GetOwner()->HasAuthority())
   {
      // listen for anything that changes the alertness level
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         // listen for anything that makes us unconscious
         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
         _unconsciousDelegateHandle = asc->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).AddUObject(this, &UTATCharacterAIAlertnessComponent::_OnIsUnconsciousTagChanged);
      }
   }
}

void UTATCharacterAIAlertnessComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   check(GetOwner()->HasAuthority());

   _AuthorityTickPlayerDetection();

   Super::TickComponent(deltaTime, tickType, thisTickFunction);
}

bool UTATCharacterAIAlertnessComponent::_AuthorityCanDecayAlertnessToNeutral() const 
{
   _CheckHasAuthority();

   if (!Super::_AuthorityCanDecayAlertnessToNeutral())
      return false;

   if (!AlertnessSettingsAsset)
      return false;

   switch(const EAlertnessLevel alertnessLevel = GetAlertnessLevel())
   {
   case EAlertnessLevel::Neutral:
      // no decay from neutral
      return false;
   case EAlertnessLevel::Suspicious:
      return AlertnessUtl::CanCharacterDecayAlertnessToNeutral(GetOwner(), alertnessLevel);
   case EAlertnessLevel::Combat:
   case EAlertnessLevel::Alerted:
      return AlertnessUtl::CanCharacterDecayAlertnessToNeutral(GetOwner(), alertnessLevel);
   default:
         unimplemented();
   }
   
   // shouldn't hit
   return false;
}

float UTATCharacterAIAlertnessComponent::_AuthorityGetSecondsUntilAlertnessDecay() const
{
   _CheckHasAuthority();
   return Super::_AuthorityGetSecondsUntilAlertnessDecay();
}

float UTATCharacterAIAlertnessComponent::_AuthorityGetTotalSecondsForAlertnessDecay() const
{
   _CheckHasAuthority();
   return Super::_AuthorityGetTotalSecondsForAlertnessDecay();
}

float UTATCharacterAIAlertnessComponent::_AuthorityGetAlertnessDecayMultiplier() const
{
   _CheckHasAuthority();

   if (_AuthorityDoesOwnAlertnessDecayForAlernessLevel(GetAlertnessLevel()))
   {
      if (_tatAlertnessSettingsAsset && UTATAIFunctionLibrary::IsBamboozled(GetOwner()))
      {
         // when we're talking about our character bamboozle modifier, it's only one bamboozled guard
         constexpr int numBamboozled = 1;
         return _tatAlertnessSettingsAsset->BamboozledSettings.GetBamboozledDecayRateMultiplier(numBamboozled);
      }
   }
   return Super::_AuthorityGetAlertnessDecayMultiplier();
}

void UTATCharacterAIAlertnessComponent::_OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel)
{
   Super::_OnAlertnessLevelChanged(oldAlertnessLevel, newAlertnessLevel);

   // Send it to the AIStateWorldSubsystem
   if (const ATATCharacterAIBase* aiCharacter = &_GetOwningCharacter())
   {
      if (UTATAIStateWorldSubsystem* aiStateWorldSubsystem = GetWorld()->GetSubsystem<UTATAIStateWorldSubsystem>())
      {
         aiStateWorldSubsystem->OnAIAlertnessLevelChanged(aiCharacter);
      }
   }

   // cancel any bamboozle status effects on us once we've hit a neutral alertness level
   if (GetOwner()->HasAuthority() && newAlertnessLevel < oldAlertnessLevel && newAlertnessLevel == EAlertnessLevel::Neutral)
   {
      UTATAIFunctionLibrary::CancelBamboozledEffects(GetOwner());
   }
}
void UTATCharacterAIAlertnessComponent::OnStimReceived(const FStimInfo& stimInfo)
{
}

void UTATCharacterAIAlertnessComponent::_OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   if (newTagCount > 0)
   {
      _AuthorityResetAlertLevel();
   }
   else if (RaiseAlertnessOnRevive)
   {
      AuthorityRaiseAlertnessLevelToAtLeast(AlertnessOnRevive);
   }
}

void UTATCharacterAIAlertnessComponent::_AuthorityTickPlayerDetection()
{
   if (!_tatAlertnessSettingsAsset)
      return;

   EAlertnessLevel currentAlertnessLevel = GetAlertnessLevel();
   EAlertnessLevel targetAlertnessLevel = currentAlertnessLevel;
   AActor* instigator = nullptr;

   const FGameplayTag unconsciousGameplayTag = UOSEProjectSettings::Get().ConditionUnconsciousTag;
   UTATAIFunctionLibrary::AuthorityForEachEnemyKnowledge(GetOwner(),
      [this, &targetAlertnessLevel, &currentAlertnessLevel, &instigator, &unconsciousGameplayTag](const FTATActorKnowledge& actorKnowledge)
      {
         const FTATDetectionAlertnessInfluenceSettings& detectionSettings = _tatAlertnessSettingsAsset->DetectionAlertnessInfluences;
         const IGameplayTagAssetInterface* target = Cast<IGameplayTagAssetInterface>(actorKnowledge.GetActor());
         if (!target)
         {
            return;
         }
         // ignore dead players
         if(target->HasMatchingGameplayTag(unconsciousGameplayTag))
         {
            return;
         }
         if (actorKnowledge.GetDetectionState() == EActorDetectionState::Identified)
         {
            // if any actor is identified, move us straight into our identified alertness level (combat) so we can respond to them
            targetAlertnessLevel = detectionSettings.IdentifiedAlertnessLevel;
            instigator = actorKnowledge.GetActor();
         }
         else if (actorKnowledge.GetDetectionValue() >= 1.0f)
         {
            // if any actor is detected, and we need an alertness bump in response, do that now
            for (const FTATDetectionValueAlertness& settings : detectionSettings.DetectionValueEffects)
            {
               if (settings.CurrentAlertnessLevel == currentAlertnessLevel && settings.TargetAlertnessLevel > targetAlertnessLevel)
               {
                  targetAlertnessLevel = settings.TargetAlertnessLevel;
                  instigator = actorKnowledge.GetActor();
                  break;
               }
            }
         }
      }
   );

   if (targetAlertnessLevel != currentAlertnessLevel)
   {
      AuthorityRaiseAlertnessLevelToAtLeast(targetAlertnessLevel, instigator);
   }
}

ATATCharacterAIBase& UTATCharacterAIAlertnessComponent::_GetOwningCharacter() const
{
   //TODO: Cache this variable off to avoid cast cost every time this is called.
   return *CastChecked<ATATCharacterAIBase>(GetOwner());
}
