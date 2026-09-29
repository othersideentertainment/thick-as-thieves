// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATCombatComponent.h"

// ose
#include "Combat/CombatComponent.h"

// ue
#include "Engine/DataAsset.h"

#include "TATAICombatComponent.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct TAT_API FTATAICombatCombo
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FName ComboName;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Ability"))
   TArray<FGameplayTag> AbilityTags;
};

UCLASS(BlueprintType)
class TAT_API UTATAICombatCombosAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty= ComboName))
   TArray<FTATAICombatCombo> Combos;

   const FGameplayTag* FindTagAtIndex(int comboIndex, int abilityIndex);

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR
};

UCLASS(BlueprintType)
class TAT_API UTATAICombatComponent : public UTATCombatComponent
{
   GENERATED_BODY()

public:
   UTATAICombatComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   FGameplayTag FindCurrentComboAbilityTag() const;

   UFUNCTION(BlueprintCallable)
   void ResetCombo();

protected:
   // Used to pick a random combo from
   UPROPERTY(EditDefaultsOnly)
   UTATAICombatCombosAsset* ComboAsset = nullptr;

protected:
   void _OnAbilityActivated(UGameplayAbility* ability);
   void _UpdateNextComboAbility();
   void _ChooseNewCombo();

private:
   FDelegateHandle _onAbilityActivatedDelegateHandle;
   int _currentComboIdx = INDEX_NONE;
   int _currentAbilityIdx = INDEX_NONE;
};
