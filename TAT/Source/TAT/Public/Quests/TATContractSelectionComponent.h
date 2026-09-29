// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "TATContractSelectionComponent.generated.h"

// A simple component for communicating quest selection in the lobby/hub to server
// On PlayerState, probably
// 
// Possibly overkill, but shouldn't be something to worry about in a lobby
// (also considered just stuffing it on TATDemoHubPlayerController)
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATContractSelectionComponent : public UActorComponent
{
   GENERATED_BODY()

public:	
   // Sets default values for this component's properties
   UTATContractSelectionComponent();

   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable)
   void InitFromLocalSave();

   UFUNCTION(BlueprintCallable)
   void SetSelectedContract(FGameplayTag questTag);

   UFUNCTION(BlueprintPure)
   FGameplayTag GetSelectedContract() const { return _selectedQuest; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedContractChanged, FGameplayTag, selectedContractTag);

   UPROPERTY(BlueprintAssignable)
   FOnSelectedContractChanged OnSelectedContractChanged;
private:
   bool _IsLocalPlayer() const;
   
   UFUNCTION(Server, Reliable)
   void _ServerSetSelectedContract(FGameplayTag questTag);

   UFUNCTION()
   void _OnRep_SelectedContract();

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_SelectedContract)
   FGameplayTag _selectedQuest;
};
