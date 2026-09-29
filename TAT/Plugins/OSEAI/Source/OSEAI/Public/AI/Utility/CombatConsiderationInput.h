// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/ConsiderationInput.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// self
#include "CombatConsiderationInput.generated.h"

class UCombatComponent;

UCLASS(Abstract)
class OSEAI_API UCombatConsiderationInput_Base : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override { unimplemented() return 0.0f; }

protected:
   UCombatComponent* _GetMyCombatComponent(const FConsiderationContext& ctx) const;
   UCombatComponent* _GetTargetCombatComponent(const FConsiderationContext& ctx) const;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TimeSinceLastAttackInitiated : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   float MaxSecondsSinceLastAttackInitiated = 1.0f;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TimeSinceTargetLastAttackInitiated : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   float MaxSecondsSinceLastAttackInitiated = 1.0f;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TimeSinceCombatDisabled : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   float MaxSecondsSinceCombatDisabled = 1.0f;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TimeSinceTargetCombatDisabled : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   float MaxSecondsSinceCombatDisabled = 1.0f;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_RangedWeaponCanReload : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_RangedWeaponNeedsReload : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_RangedWeaponHasRequiredProjectilesForActivation : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TargetInRangedWeaponAttackRange : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UCombatConsiderationInput_TargetHasValidHitPath : public UCombatConsiderationInput_Base
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};
