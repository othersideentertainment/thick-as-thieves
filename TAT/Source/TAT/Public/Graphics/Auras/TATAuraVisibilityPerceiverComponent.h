// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

// tat
#include "Graphics/Auras/TATAuraVisibilityTypes.h"

#include "TATAuraVisibilityPerceiverComponent.generated.h"

class AOSECharacterBase;
class UTATAuraVisibilityTargetComponent;
class ATATPlayerController;


UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Config = Game)
class TAT_API UTATAuraVisibilityPerceiverComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATAuraVisibilityPerceiverComponent();

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UActorComponent
   virtual void BeginPlay() override;
   virtual void InitializeComponent() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   const TArray<FTATAuraVisibilityPerceiverSense>& GetAuraVisibilityPerceiverSenses() const { return _auraPerceivedVisibilitySenses; }
   const FTATAuraVisibilityPerceiverSense* GetAuraSense(FGameplayTag auraSenseTag) const 
   { 
      return _auraSensesToEvaluate.FindByPredicate([&](const FTATAuraVisibilityPerceiverSense& auraSense) { return auraSense.AuraSenseTag == auraSenseTag; });
   }

   const AOSECharacterBase* GetOwnerCharacter() const { return _ownerCharacter; }

   bool ShouldEvaluateAnyAuraSenses() const { return _auraSensesToEvaluate.Num() > 0; }
   bool ShouldEvaluateAuraSense(const FGameplayTag auraSenseTag) const { return GetAuraSense(auraSenseTag) != nullptr; }

protected:

   UPROPERTY(EditDefaultsOnly)
   TArray<FTATAuraVisibilityPerceiverSense> _auraPerceivedVisibilitySenses;

private:
   void _EvaluateAuraVisibilityPerceiverSense(const FTATAuraVisibilityPerceiverSense& perceiverSense);

   void _EnableAuraPerception();

   void _DisableAuraPerception();

   UFUNCTION()
   void _OnOwnerCharacterPossessed(AController* controller);

   TArray<FTATAuraVisibilityPerceiverSense> _GetAuraSensesToEvaluate() const;
   bool _ShouldEverEvaluateAuraSense(const FTATAuraVisibilityPerceiverSense& auraSense) const;

   
private:

   UPROPERTY(Transient)
   AOSECharacterBase* _ownerCharacter = nullptr;

   UPROPERTY(Transient)
   ATATPlayerController* _ownerPlayerController = nullptr;

   UPROPERTY(Transient)
   TArray<FTATAuraVisibilityPerceiverSense> _auraSensesToEvaluate;

   UPROPERTY(Transient)
   TArray<FTATAuraVisibilityTargetCachedData> _auraTargetCachedData;

   UPROPERTY(EditDefaultsOnly)
   TEnumAsByte<ECollisionChannel> _auraLineOfSightCollisionChannel = ECC_Visibility;
};
