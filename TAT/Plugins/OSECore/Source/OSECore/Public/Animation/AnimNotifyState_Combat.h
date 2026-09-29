// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_Combat.generated.h"

class UCombatComponent;
struct FAnimNotifyEventReference;

//---------------------------------------------------------------------------------------
// UAnimNotifyState_Combat
//---------------------------------------------------------------------------------------

// Base class for combat anim notify states
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Combat_NotifyStateBase"))
class OSECORE_API UAnimNotifyState_Combat : public UAnimNotifyState
{
   GENERATED_BODY()

public:

   UAnimNotifyState_Combat(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   virtual FString GetNotifyName_Implementation() const override;

   // for our subclasses
   virtual void PerformCombatActionBegin(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) { }
   virtual void PerformCombatActionTick(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) { }
   virtual void PerformCombatActionEnd(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) { }

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("Combat"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_Combat
//---------------------------------------------------------------------------------------

// Base class for combat anim notifies
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Combat_NotifyBase"))
class OSECORE_API UAnimNotify_Combat : public UAnimNotify
{
   GENERATED_BODY()

public:

   UAnimNotify_Combat(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from UAnimNotify
   virtual FString GetNotifyName_Implementation() const override;
   virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

   // for our subclasses
   virtual bool IsDrivenByCombatMontage() const { return true; }
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) { }

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("Combat"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAnimationStart
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_AnimStart"))
class OSECORE_API UAnimNotify_CombatAnimationStart : public UAnimNotify_Combat
{
   GENERATED_BODY()

public:
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("AnimStart"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAnimationEnd
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_AnimEnd"))
class OSECORE_API UAnimNotify_CombatAnimationEnd : public UAnimNotify_Combat
{
   GENERATED_BODY()

public:
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("AnimEnd"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAutoAim
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_AutoAim"))
class OSECORE_API UAnimNotify_CombatAutoAim : public UAnimNotify_Combat
{
   GENERATED_BODY()

public:
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("AutoAim"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatRangedProjectileSpawn
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_RangedProjectileSpawn"))
class OSECORE_API UAnimNotify_CombatRangedProjectileSpawn : public UAnimNotify_Combat
{
   GENERATED_BODY()

public:
   virtual bool IsDrivenByCombatMontage() const { return false; }
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("RangedProjectileSpawn"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatRangedReload
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_RangedReload"))
class OSECORE_API UAnimNotify_CombatRangedReload : public UAnimNotify_Combat
{
   GENERATED_BODY()

public:
   virtual bool IsDrivenByCombatMontage() const { return false; }
   virtual void PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("RangedReload"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotifyState_CombatHitbox
//---------------------------------------------------------------------------------------

UCLASS(Blueprintable, meta = (DisplayName = "Combat_Hitbox"))
class OSECORE_API UAnimNotifyState_CombatHitbox : public UAnimNotifyState_Combat
{
   GENERATED_BODY()

public:
   // for debug drawing in the editor window
   virtual void NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;

   virtual void PerformCombatActionBegin(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;
   virtual void PerformCombatActionEnd(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation) override;

public:
   /// Which hitbox does this notify index into?
   UPROPERTY(VisibleAnywhere, Category = "Combat")
   int HitboxIndex = INDEX_NONE;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("Hitbox"); };
};

//---------------------------------------------------------------------------------------
// UAnimNotifyState_CombatMontage
//---------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECombatAttackDirection : uint8
{
   None,
   LeftToRight,
   RightToLeft,
};

UCLASS(Blueprintable, meta = (DisplayName = "Combat_Montage"))
class OSECORE_API UAnimNotifyState_CombatMontage : public UAnimNotifyState_Combat
{
   GENERATED_BODY()

public:
   virtual void NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

   UPROPERTY(EditAnywhere, Category = "Combat Montage")
   ECombatAttackDirection CombatAttackDirection = ECombatAttackDirection::None;

protected:
   virtual FString _GetCombatNotifyName() const { return TEXT("Montage"); };

private:
   void _SendCombatNotifies(UAnimMontage& montage, UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, float prevMontagePosition, float currentMontagePosition);
};
