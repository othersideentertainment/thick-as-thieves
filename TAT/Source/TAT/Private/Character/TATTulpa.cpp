// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/TATTulpa.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Character/TATTeams.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTulpa)

DEFINE_LOG_CATEGORY_STATIC(LogTATTulpa, Log, All)

ATATTulpa::ATATTulpa()
{
}

void ATATTulpa::BeginPlay()
{
   UWorld* world = GetWorld();
   if (HasAuthority())
   {
      _spawnedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(world);
   }

   Super::BeginPlay();

   // It is begin-play or nothing, a late transition here would be weird
   if (IsValid(_sourceCharacter) && !UOSEInteractionHelpers::IsOld(world, _spawnedServerTime))
   {
      BP_OnSpawnedFromCharacter(_sourceCharacter);
   }
}

void ATATTulpa::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATTulpa, _sourceCharacter, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATTulpa, _spawnedServerTime, COND_InitialOnly);
}

void ATATTulpa::_OnDamageChanged(const FOnAttributeChangeData& data)
{
   Super::_OnDamageChanged(data);

   if (data.NewValue > 0 && data.OldValue <= 0)
   {
      // apply gameplay effect to attacking character
      AActor* attacker = data.GEModData ? data.GEModData->EffectSpec.GetEffectContext().GetInstigator() : nullptr;
      UAbilitySystemComponent* attackerAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(attacker, false);
      if (attackerAsc)
      {
         for(const FOSEEffectWithSetByCallerTagAndMagnitude& effect : _attackerEffects)
         {
            effect.ApplyEffect(attackerAsc);
         }
      }
      
      // apply gameplay effect to instigator
      UAbilitySystemComponent* instigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator(), false);
      if (instigatorASC)
      {
         for(const FOSEEffectWithSetByCallerTagAndMagnitude& effect : _instigatorDamageEffects)
         {
            effect.ApplyEffect(instigatorASC);
         }
      }

      // destroy self on taking damage.
      // This is latent, so should be fine to have nested
      Destroy();
   }
}

uint8 ATATTulpa::GetTeam() const
{
   // inherit team from instigator is available
   /*const AActor* instigator = GetInstigator();
   if (instigator && instigator->Implements<UOSETeamInterface>())
   {
      return IOSETeamInterface::Execute_GetTeam(instigator);
   }*/

   return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Player);
}

void ATATTulpa::AuthorityTargetLocation(FVector location)
{
   _authorityTargetType = ETATTulpaTargetType::Location;
   _authorityTargetLocation = location;

   // Push the change to blackboard so Behavior Tree blueprints can access it
   if (UBlackboardComponent* blackboardComponent = _GetControllerBlackboardComponent())
   {
      blackboardComponent->SetValue<UBlackboardKeyType_Vector>(_blackboardKeyTargetLocationName, location);
   }
}

void ATATTulpa::AuthorityTargetHostileActor(AActor* actor)
{
   _authorityTargetType = ETATTulpaTargetType::HostileActor;
   _authorityTargetActor = actor;

   // Push the change to blackboard so Behavior Tree blueprints can access it
   if (UBlackboardComponent* blackboardComponent = _GetControllerBlackboardComponent())
   {
      blackboardComponent->SetValue<UBlackboardKeyType_Object>(_blackboardKeyTargetActorName, actor);
   }
}

void ATATTulpa::AuthorityTargetInteractable(AActor* actor)
{
   _authorityTargetType = ETATTulpaTargetType::Interactable;
   _authorityTargetActor = actor;

   // Push the change to blackboard so Behavior Tree blueprints can access it
   if (UBlackboardComponent* blackboardComponent = _GetControllerBlackboardComponent())
   {
      blackboardComponent->SetValue<UBlackboardKeyType_Object>(_blackboardKeyTargetActorName, actor);
   }
}

FVector ATATTulpa::AuthorityGetTargetLocation() const
{
   if (_authorityTargetType != ETATTulpaTargetType::Location)
   {
      UE_LOG(LogTATTulpa, Error,
         TEXT("Calling AuthorityGetTargetLocation on '%s', but tulpa's target type is %s"),
         *GetName(),
         *UEnum::GetValueAsString(_authorityTargetType));
      return FVector::ZeroVector;
   }

   return _authorityTargetLocation;
}

AActor* ATATTulpa::AuthorityGetTargetActor() const
{
   if (_authorityTargetType != ETATTulpaTargetType::HostileActor && _authorityTargetType != ETATTulpaTargetType::Interactable)
   {
      UE_LOG(LogTATTulpa, Error,
         TEXT("Calling AuthorityGetTargetActor on '%s', but tulpa's target type is %s"),
         *GetName(),
         *UEnum::GetValueAsString(_authorityTargetType));
      return nullptr;
   }

   return _authorityTargetActor;
}

UBlackboardComponent* ATATTulpa::_GetControllerBlackboardComponent() const
{
   if (AAIController* aiController = GetController<AAIController>())
   {
      return aiController->GetBlackboardComponent();
   }
   else
   {
      return nullptr;
   }
}
