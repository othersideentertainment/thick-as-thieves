// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Character/OSECharacterBase.h"

#include "TATTulpa.generated.h"

class UGameplayEffect;
class UBlackboardComponent;

UENUM(BlueprintType)
enum class ETATTulpaTargetType : uint8
{
   None,
   Location,
   HostileActor,
   Interactable
};


// A character spawned by tulpamancy that runs around until hit
UCLASS()
class TAT_API ATATTulpa : public AOSECharacterBase
{
   GENERATED_BODY()

   ATATTulpa();

   virtual void BeginPlay() override;

   // IOSETeamInterface
   virtual uint8 GetTeam() const override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   void AuthorityTargetLocation(FVector location);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   void AuthorityTargetHostileActor(AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   void AuthorityTargetInteractable(AActor* actor);

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   ETATTulpaTargetType AuthorityGetTargetType() const { return _authorityTargetType; }

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   FVector AuthorityGetTargetLocation() const;

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Tulpa|TAT")
   AActor* AuthorityGetTargetActor() const;

protected:
   virtual void _OnDamageChanged(const FOnAttributeChangeData& data) override;

   // A hook to add VFX on being doubled from a character
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnSpawnedFromCharacter(ACharacter* sourceCharacter);

protected:
   UPROPERTY(EditAnywhere)
   FName _blackboardKeyTargetActorName;

   UPROPERTY(EditAnywhere)
   FName _blackboardKeyTargetLocationName;

   // gameplay effects to apply to attacker when damaged
   UPROPERTY(EditDefaultsOnly, Category = Effects)
   TArray<FOSEEffectWithSetByCallerTagAndMagnitude> _attackerEffects;

   // gameplay effects to apply to instigator when damaged
   UPROPERTY(EditDefaultsOnly, Category = Effects)
   TArray<FOSEEffectWithSetByCallerTagAndMagnitude> _instigatorDamageEffects;

   // the source actor being doubled, not necessarily the caster
   UPROPERTY(Replicated, BlueprintReadOnly, EditInstanceOnly, meta = (ExposeOnSpawn = true))
   TObjectPtr<ACharacter> _sourceCharacter;

   UPROPERTY(Replicated)
   float _spawnedServerTime;

   UPROPERTY(Transient)
   ETATTulpaTargetType _authorityTargetType = ETATTulpaTargetType::None;

   UPROPERTY(Transient)
   AActor* _authorityTargetActor = nullptr;

   UPROPERTY(Transient)
   FVector _authorityTargetLocation = FVector::ZeroVector;

private:
   UBlackboardComponent* _GetControllerBlackboardComponent() const;
};
