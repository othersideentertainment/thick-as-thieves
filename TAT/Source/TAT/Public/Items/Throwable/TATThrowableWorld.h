// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATSimpleDamageableInterface.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATThrowableWorld.generated.h"

class ATATThrowableProjectile;

// An actor representing the version of a throwable that can be picked up in a level
UCLASS()
class TAT_API ATATThrowableWorld : public AActor
   , public IInteractableInterface
   , public ITATSimpleDamageableInterface
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATATThrowableWorld();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

public:   

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSubclassOf<ATATThrowableProjectile> ProjectileClass;

   // Temporary animations
   virtual void Tick(float deltaTime) override;

   virtual void PostNetReceiveLocationAndRotation() override;

   UFUNCTION(BlueprintCallable)
   void LocallyMaintainViewOffset_TEMP(FVector viewOffset);

   UFUNCTION(BlueprintCallable)
   void LocallyInterpolateViewOffset_TEMP(FVector viewOffset);

   UFUNCTION(BlueprintCallable)
   void Drop_TEMP(FVector worldPosition, FRotator worldRotation);


   UFUNCTION(BlueprintCallable)
   void StopMovement_TEMP();

   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnBroken();

   // ITATSimpleDamageableInterface
   virtual bool CanHandleDamage_Implementation() const override;
   virtual void AuthorityHandleDamage_Implementation(const FTATDamageWithType& damage, const FTATSimpleDamageSource& damageSource) override;

   
protected:
   UFUNCTION()
   void _OnRep_Broken();
   
   bool _maintainViewOffset_TEMP;
   bool _interpolateToViewOffset_TEMP;
   bool _interpolateToWorldPosition_TEMP;
   FVector _viewOffset_TEMP;
   FVector _targetViewOffset_TEMP;
   FVector _targetWorldPosition_TEMP;
   FRotator _targetWorldRotation_TEMP;
   float _lastWorldEndTime_TEMP;

   UPROPERTY(EditDefaultsOnly)
   float _viewTargetInterpolationRate_TEMP;
   UPROPERTY(EditDefaultsOnly)
   float _dropSpeed_TEMP;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_Broken)
   bool _broken;

   UPROPERTY(EditDefaultsOnly, Category=Damage)
   bool _canBreak = false;

   // damage required in a single event to break this
   UPROPERTY(EditDefaultsOnly, Category=Damage)
   int32 _minDamageToBreak = 5;
   
   UPROPERTY(EditDefaultsOnly, Category=Damage)
   float _lifespanAfterBreaking = 1.f;

   UPROPERTY(EditDefaultsOnly, Category=Damage, meta=(Categories="AI.Stim.Hearing"))
   FGameplayTag _breakStimTag;
};
