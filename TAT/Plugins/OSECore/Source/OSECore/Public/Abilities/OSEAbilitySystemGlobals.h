// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "OSEAbilityTypes.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "OSEAbilitySystemGlobals.generated.h"

USTRUCT()
struct FOSEPooledTargetActors
{
   GENERATED_BODY();

   /** Actors that are ready to be reused */
   UPROPERTY()
   TArray<AGameplayAbilityTargetActor*> InactiveList;

   /** Actors that are currently in world */
   UPROPERTY()
   TArray<AGameplayAbilityTargetActor*> ActiveList;
};


/// Override game-specific ability global functions
UCLASS(ClassGroup = (Ability))
class OSECORE_API UOSEAbilitySystemGlobals : public UAbilitySystemGlobals
{
   GENERATED_BODY()

public:
   /** Get game specific one, can't override the generic one and it's not templated */
   static UOSEAbilitySystemGlobals& OSEGet()
   {
      return *CastChecked<UOSEAbilitySystemGlobals>(IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals());
   }

   /** Get game-specific component */
   static class UOSEAbilitySystemComponent* GetOSEAbilitySystemComponentFromActor(const class AActor* actor, bool lookForComponent = false);

   /** Game-specific overrides */
   virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
   virtual void InitGlobalData() override;

   /** Handle target actor pools */
   AGameplayAbilityTargetActor* GetTargetActor(UWorld* world, TSubclassOf<AGameplayAbilityTargetActor> actorClass, EGameplayTargetingConfirmation::Type confirmationType);
   void DoneWithTargetActor(AGameplayAbilityTargetActor* targetActor);


   // Engine bug: HandlePreLoadMap is not virtual for some reason
   void OSEHandlePreLoadMap(const FString& mapName);

private:
   UPROPERTY()
   TMap<UClass*, FOSEPooledTargetActors> _targetActorClassPool;

   /** Max number of actors per class that will be stored in the inactive pool, anything above this gets destroyed */
   UPROPERTY(config)
   int32 MaxTargetActorPoolSize = 2;
};
