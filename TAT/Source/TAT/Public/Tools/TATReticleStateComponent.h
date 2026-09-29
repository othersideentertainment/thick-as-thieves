// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "TATReticleStateComponent.generated.h"

class UToolComponent;
class UTATCustomReticleWidgetBase;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATReticleStateComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UTATReticleStateComponent();

   // From UActorComponent
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   FGameplayTag GetCurrentReticleState() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   AActor* GetCurrentReticleTargetActor() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   TSubclassOf<UTATCustomReticleWidgetBase> GetCurrentReticleWidgetClass() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReticleStateChanged, FGameplayTag, oldState, FGameplayTag, newState);
   UPROPERTY(BlueprintAssignable, Category = "TAT|Tools")
   FOnReticleStateChanged OnReticleStateChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReticleTargetActorChanged, AActor*, oldTargetActor, AActor*, newTargetActor);
   UPROPERTY(BlueprintAssignable, Category = "TAT|Tools")
   FOnReticleTargetActorChanged OnReticleTargetActorChanged;


   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReticleWidgetChanged, TSubclassOf<UTATCustomReticleWidgetBase>, widget);
   UPROPERTY(BlueprintAssignable, Category = "TAT|Tools")
   FOnReticleWidgetChanged OnReticleWidgetChanged;

protected:
   // Tags on the character that cause the tool to be ignored
   UPROPERTY(EditDefaultsOnly, Category=Tools)
   FGameplayTagContainer _ignoreToolTags;

private:

   UFUNCTION()
   void _OnOwnerPawnChanged(APawn* newPawn);

   UToolComponent* _GetCurrentlyEquippedTool();
   void _MaybeNotifyNewReticleWidget(TSubclassOf<UTATCustomReticleWidgetBase> oldWidget);
   void _MaybeNotifyNewReticleTag(FGameplayTag oldReticleTag);
   void _MaybeNotifyNewReticleTargetActor(AActor* oldTargetActor);

   UPROPERTY(Transient)
   TObjectPtr<AActor> _currentReticleTargetActor;

   UPROPERTY(Transient)
   TSubclassOf<UTATCustomReticleWidgetBase> _currentReticleWidgetClass;

   FGameplayTag _currentReticleTag;

   TWeakObjectPtr<APawn> _currentlyPossessedPawn;
};


