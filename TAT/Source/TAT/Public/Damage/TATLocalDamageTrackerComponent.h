// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATDamageTypes.h"

// ue5
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"

#include "TATLocalDamageTrackerComponent.generated.h"

class AOSECharacterBase;
class UAbilitySystemComponent;
struct FActiveGameplayEffect;
struct FGameplayEffectSpec;

USTRUCT(BlueprintType)
struct FTATLocalDamageOverTimeEntry
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadOnly)
   FActiveGameplayEffectHandle SourceEffect;

   UPROPERTY(BlueprintReadOnly)
   float DamagePerSecond = 0;

   UPROPERTY(BlueprintReadOnly)
   FGameplayTag DamageType;

   bool operator==(const FActiveGameplayEffectHandle& handle)
   {
      return handle == SourceEffect;
   }
};

USTRUCT(BlueprintType)
struct FTATLocalDamageSource
{
   GENERATED_BODY()

   // optional instigator
   UPROPERTY(BlueprintReadOnly)
   TObjectPtr<AActor> Instigator = nullptr;

   // optional origin
   UPROPERTY(BlueprintReadOnly)
   FVector Origin = FVector::ZeroVector;

   // optional normal
   UPROPERTY(BlueprintReadOnly)
   FVector Normal = FVector::ZeroVector;
};

// A component on the controller to track damage taken by the locally controlled character for display.
//
// It does this in two ways:
// 1. For instant damage effects, the cues will use this as a relay (along with type and origin)
// 2. For damage over time, it will listen for effects and infer the magnitude of the damage (along with the type)
// 
// The damage over time makes some simplifying assumptions, but largely tries to mimic the actual damage calculation.
// Since the intent for this is to support visualizations, the more important thing is the type and rough magnitude.
//
// There is a partial argument for moving some of dispatch to a subsystem, but all the relevant things will likely have the controller anyways
UCLASS()
class TAT_API UTATLocalDamageTrackerComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATLocalDamageTrackerComponent();


   UFUNCTION(BlueprintCallable, Category = "Damage|Local")
   static void DispatchLocalDamageFromGameplayCue(AController* controller, const FGameplayCueParameters& params);

   UFUNCTION(BlueprintPure, Category="Damage|Local")
   static UTATLocalDamageTrackerComponent* GetLocalDamageTrackerForController(AController* controller);

   UFUNCTION(BlueprintPure, Category = "Damage|Local")
   const TArray<FTATLocalDamageOverTimeEntry>& GetActiveDamageOverTimeEffects() const { return _activeDotEffects; }

   UFUNCTION(BlueprintPure, Category = "Damage|Local")
   bool HasActiveDamageOverTimeEffects() const { return _activeDotEffects.Num() > 0; }

   // for convenience, since it harder to do in BP
   UFUNCTION(BlueprintCallable, Category = "Damage|Local", meta=(DefaultToSelf=source))
   void RemoveListenersFrom(const UObject* source);

   // for convenience in normalizing values
   UFUNCTION(BlueprintPure, Category = "Damage|Local")
   float GetMaxHealth() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDamageOverTimeChanged);
   UPROPERTY(BlueprintAssignable)
   FOnDamageOverTimeChanged OnDamageOverTimeChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInstantDamageTaken, FTATDamageWithType, damage, FTATLocalDamageSource, source);
   UPROPERTY(BlueprintAssignable)
   FOnInstantDamageTaken OnInstantDamageTaken;

protected:
   virtual void BeginPlay() override;

   const FTATLocalDamageOverTimeEntry* _FindEntryForHandle(FActiveGameplayEffectHandle effectHandle) const;
   FTATLocalDamageOverTimeEntry* _FindEntryForHandle(FActiveGameplayEffectHandle effectHandle);

   UFUNCTION()
   void _OnPawnChanged(APawn* oldPawn, APawn* newPawn);
   void _OnAbilitiesInitialized();

   void _OnEffectAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle effectHandle);
   void _OnEffectInhibitionChanged(FActiveGameplayEffectHandle effectHandle, bool isInhibited);
   void _OnEffectStackChanged(FActiveGameplayEffectHandle effectHandle, int32 newStackCount, int32 oldStackCount);

   void _TryRemoveActiveDotEffect(FActiveGameplayEffectHandle effectHandle);

   void _AddActiveDotEffect(const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle effectHandle);
   void _OnEffectRemoved(const FActiveGameplayEffect& activeEffect);

private:
   UPROPERTY(Transient)
   AOSECharacterBase* _currentCharacter;

   UPROPERTY(Transient)
   UAbilitySystemComponent* _abilitySystemComponent;

   UPROPERTY(Transient)
   TArray<FTATLocalDamageOverTimeEntry> _activeDotEffects;

   // Just for unsubscribing from inhibition on pawn change
   TArray<FActiveGameplayEffectHandle> _trackedDotHandles;
};
