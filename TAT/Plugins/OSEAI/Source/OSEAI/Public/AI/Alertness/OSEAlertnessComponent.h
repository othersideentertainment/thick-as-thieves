// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue4
#include "Components/ActorComponent.h"

#include "OSEAlertnessComponent.generated.h"

struct FOnAttributeChangeData;
class AOSEAIController;
class AOSECharacterAIBase;
class UGameplayEffect;
class UOSEAlertnessSettingsAsset;

UCLASS(BlueprintType)
class OSEAI_API UOSEAlertnessComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAlertnessLevelChanged, EAlertnessLevel, oldAlertnessLevel, EAlertnessLevel, newAlertnessLevel);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAuthorityOnAlertnessLevelChanged, EAlertnessLevel, oldAlertnessLevel, EAlertnessLevel, newAlertnessLevel, AActor*, alertnessIncreaseInstigator);
   DECLARE_MULTICAST_DELEGATE_ThreeParams(FAlertnessLevelChanged, EAlertnessLevel, EAlertnessLevel, AActor*);

public:
   UOSEAlertnessComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   FAlertnessLevelChanged OnAlertnessLevelChangedEvent;

   UPROPERTY(BlueprintAssignable, Category = "Alertness")
   FOnAlertnessLevelChanged OnAlertnessLevelChanged;

   UPROPERTY(BlueprintAssignable, Category = "Alertness")
   FAuthorityOnAlertnessLevelChanged AuthorityOnAlertnessLevelChanged;

   UFUNCTION(BlueprintPure, Category = "Alertness")
   EAlertnessLevel GetAlertnessLevel() const { return _alertnessLevel; }

   UFUNCTION(BlueprintPure, Category = "Alertness")
   EAlertnessLevel GetMinAlertnessLevel() const { return MinAlertnessLevel; }

   UFUNCTION(BlueprintPure, Category = "Alertness")
   EAlertnessLevel GetMaxAlertnessLevel() const { return MaxAlertnessLevel; }

   UFUNCTION(BlueprintCallable, Category = "Alertness")
   void SetMinAndMaxAlertnessLevel(EAlertnessLevel minAlertnessLevel, EAlertnessLevel maxAlertnessLevel);

   // Raises the alertness level to at least this value, and does nothing if we're higher than it already
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|Alertness")
   void AuthorityRaiseAlertnessLevelToAtLeast(EAlertnessLevel alertnessLevel, AActor* optionalInstigator = nullptr);

   // Lowers the alertness level to at most this value, and does nothing if we're lower than it already
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|Alertness")
   void AuthorityLowerAlertnessLevelToAtMost(EAlertnessLevel alertnessLevel, AActor* optionalInstigator = nullptr);

protected:
   UPROPERTY(EditDefaultsOnly)
   UOSEAlertnessSettingsAsset* AlertnessSettingsAsset = nullptr;

   UFUNCTION()
   void _OnRep_AlertLevel(EAlertnessLevel oldAlertnessLevel);
   void _BroadcastAlertLevelChanged(EAlertnessLevel oldAlertnessLevel);
   virtual void _AuthorityBroadcastAlertLevelChanged(EAlertnessLevel oldAlertnessLevel, AActor* instigator) const;

   // for our subclasses to override
   virtual void _OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel) { }
   virtual void _OnAlertnessLevelChangeRequested() { }
   virtual void _OnAlertnessLevelChangeRefreshed() { }

   // for our subclasses to use
   void _AuthorityResetAlertLevel();
   
   /**
    *  Set the target alertness value with a required instigator 
    * @param instigator Required - The actor responsible for the alertness change
    * @param level The target alertness level
    * @return True if the value was changed, False if the value wasn't.
    */
   virtual bool _SetAlertnessLevel(AActor* instigator, EAlertnessLevel level);
   
   UPROPERTY(EditDefaultsOnly, Category = "Alertness")
   EAlertnessLevel MinAlertnessLevel = EAlertnessLevel::Neutral;

   UPROPERTY(EditDefaultsOnly, Category = "Alertness")
   EAlertnessLevel MaxAlertnessLevel = EAlertnessLevel::Combat;

private:
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_AlertLevel)
   EAlertnessLevel _alertnessLevel = EAlertnessLevel::Neutral;

   UPROPERTY(Transient)
   AController* _controller = nullptr;
};

OSEAI_API DECLARE_LOG_CATEGORY_EXTERN(LogAlertnessComponent, Log, All);
