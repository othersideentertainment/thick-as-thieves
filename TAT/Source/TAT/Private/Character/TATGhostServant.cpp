// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "Character/TATGhostServant.h"

// ose
#include "OSEProjectSettings.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue5 
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGhostServant)

// Sets default values
ATATGhostServant::ATATGhostServant()
{
   // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATATGhostServant::BeginPlay()
{
   Super::BeginPlay();

   if(const AActor* instigator = GetInstigator())
   {
      if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(instigator))
      {
         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
         TWeakObjectPtr<ATATGhostServant> weakThis = this;
         auto removeResponderLambda = [weakThis](const FGameplayTag tag, const int32 newTagCount)
         {
            if(weakThis.IsValid())
            {
               if(newTagCount > 0)
               {
                  weakThis->_OnCreatorDied();
               }
            }
         };
         abilitySystemComponent->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).
                                                                AddWeakLambda(this,removeResponderLambda);
         // Check if they are already unconscious
         if(abilitySystemComponent->HasMatchingGameplayTag(settings.ConditionUnconsciousTag))
         {
            _OnCreatorDied();
         }
      }
   }
}

void ATATGhostServant::Tick(float DeltaSeconds)
{
   Super::Tick(DeltaSeconds);

   if(!HasAuthority())
      return;
   
   if(bTriggeredLeftRangeOfCreator)
      return;
   
   if(const AActor* instigator = GetInstigator())
   {
      // We don't want to get too far away from our creator
      const FVector creatorLocation = instigator->GetActorLocation();
      if(FVector::Distance(creatorLocation, GetActorLocation()) > _MaxDistanceFromCreator)
      {
         bTriggeredLeftRangeOfCreator = true;
         _OnLeftRangeOfCreator();
      }
   }
}

void ATATGhostServant::_OnLeftRangeOfCreator_Implementation()
{
}

void ATATGhostServant::_OnCreatorDied_Implementation()
{
}
