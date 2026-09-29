// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/TATAIStateWorldSubsystem.h"
#include "AI/TATKnowledgeComponent.h"

// ue
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "TATPrivateSpaceCharacterComponent.generated.h"

class UGameplayEffect;

UCLASS(ClassGroup=("Spaces"), meta=(BlueprintSpawnableComponent))
class TAT_API UTATPrivateSpaceCharacterComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATPrivateSpaceCharacterComponent();

   static UTATPrivateSpaceCharacterComponent* TryGet(AActor* actor);
   
   UFUNCTION(BlueprintCallable)
   void AuthorityIdentifiedAsIntruder(APawn* identifyingActor);
   UFUNCTION(BlueprintCallable)
   void AuthorityIdentifiedAsHostileIntruder(APawn* identifyingActor);

   UFUNCTION(BlueprintCallable)
   void AuthorityRemoveIdentifiedAsHostileIntruder(APawn* identifyingActor);   
   UFUNCTION(BlueprintCallable)
   void AuthorityRemoveIdentifyingActor(APawn* identifyingActor);
   
   void AuthorityOnEnterPrivateSpaceVolume(bool isOffLimits);
   void AuthorityOnExitPrivateSpaceVolume(bool isOffLimits);

   void AuthorityUpdatePrivateSpaceTag();
   
   void AuthorityTryForceRemovalOfPrivateSpaceEffect();

   const FGameplayTagContainer& AuthorityGetAllAllowedPrivateZone() const { return AllowedPrivateZones; }
   void AuthorityAddAllowedPrivateZone(const FGameplayTag privateZoneTag) { AllowedPrivateZones.AddTag(privateZoneTag); }
   bool AuthorityIsAllowedInPrivateZone(const FGameplayTag privateZoneTag) const;

   void AuthorityAddTemporaryAllowedPrivateZone(FGameplayTag privateZoneTag);
   void AuthorityRemoveTemporaryAllowedPrivateZone(FGameplayTag privateZoneTag);
   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTemporaryAllowedChanged, FGameplayTag, AActor*);
   FOnTemporaryAllowedChanged OnTemporaryAllowedChanged;
   
protected:
   
   void _ApplyGameplayEffect(
      FActiveGameplayEffectHandle& handle,
      TSubclassOf<UGameplayEffect> gameplayEffectClass
   ) const;
   void _RemoveGameplayEffectByHandle(
      FActiveGameplayEffectHandle& handle
   ) const;

  
   void _OnHostileActorKnowledgeAboutToBeRemoved(const FTATActorKnowledge& knowledge, APawn* identifyingActor);
   void _OnIdentifiedActorKnowledgeAboutToBeRemoved(const FTATActorKnowledge& knowledge, APawn* identifyingActor);

   // Effect to apply while character is in a private space
   UPROPERTY(EditDefaultsOnly, Category = "Ability|Private Space")
   TSubclassOf<UGameplayEffect> PrivateSpaceEffect;

   // Effect to apply while character is identified as an intruder within the private space
   UPROPERTY(EditDefaultsOnly, Category = "Ability|Private Space")
   TSubclassOf<UGameplayEffect> IdentifiedIntruderEffect;

   // Effect to apply while character is identified as an hostile intruder within the private space
   UPROPERTY(EditDefaultsOnly, Category = "Ability|Private Space")
   TSubclassOf<UGameplayEffect> IdentifiedHostileIntruderEffect;

   FGameplayTagContainer AllowedPrivateZones;
private:
   bool _AuthorityIsInPrivateSpace() const;
   void _AuthorityApplyPrivateSpaceGameplayEffect();
   void _AuthorityRemovePrivateSpaceGameplayEffect();
   void _ClearIdentifyingActors();

   // stores counts, so they can be removed
   TMap<FGameplayTag, int32> _temporaryAllowedPrivateZones;
   
   int _authorityInPrivateSpaceCounter = 0;
   int _authorityInOffLimitSpaceCounter = 0;
   
   FActiveGameplayEffectHandle _privateSpaceEffectHandle;
   FActiveGameplayEffectHandle _identifiedIntruderEffectHandle;
   FActiveGameplayEffectHandle _identifiedHostileIntruderEffectHandle;

   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<APawn>> _identifyingActor;
   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<APawn>> _identifyingHostileActor;
};
