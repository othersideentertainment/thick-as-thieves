// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TATExclusiveSwitchSet.generated.h"

class UTATExplicitTransitionToggleComponent;

/// An actor that keeps track of the togglables controlled by exclusive switches, in order to
//  enforce that only one is in use at a time
UCLASS()
class TAT_API ATATExclusiveSwitchSet : public AActor
{
	GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInUseChanged, bool, inUse);

public:	
	// Sets default values for this actor's properties
	ATATExclusiveSwitchSet();

   UFUNCTION(BlueprintPure)
   bool IsInUse() const { return _inUseToggle != nullptr; }

   const UTATExplicitTransitionToggleComponent* GetInUseToggle() const { return _inUseToggle; }

   void AuthorityRegisterSwitchToggle(UTATExplicitTransitionToggleComponent* switchToggle);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
   UPROPERTY(BlueprintAssignable)
   FOnInUseChanged OnInUseChanged;

   // The default state of the switch, from which only one can deviate
	UPROPERTY(EditAnywhere)
   bool DefaultState;

private:
   bool _IsToggleInUse(const UTATExplicitTransitionToggleComponent* toggle) const;
   const UTATExplicitTransitionToggleComponent* _FindAnyToggleInUse() const;
   void _SetInUseToggle(const UTATExplicitTransitionToggleComponent* toggle);
   void _BroadcastInUseChanged();

   UFUNCTION()
   void _OnToggleStateChanged(bool isOn, bool isTransitioning);

   UFUNCTION()
   void _OnRep_InUseToggle();


   UPROPERTY(Transient)
   TArray<TObjectPtr<const UTATExplicitTransitionToggleComponent>> _switchedToggles;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_InUseToggle)
   TObjectPtr<const UTATExplicitTransitionToggleComponent> _inUseToggle;
};
