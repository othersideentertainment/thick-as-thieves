// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "Animation/AnimMetaData.h"

#include "AnimMetadata_Combat.generated.h"

USTRUCT()
struct OSECORE_API FCombatHitboxMetadata
{
   GENERATED_BODY()

public:
   UPROPERTY()
   FVector Location = FVector::ZeroVector;
   UPROPERTY()
   float Radius = 0.0f;
   UPROPERTY()
   FLinearColor DebugTraceColor = FLinearColor::Red;
   UPROPERTY()
   FLinearColor DebugTraceHitColor = FLinearColor::Green;
};

USTRUCT()
struct OSECORE_API FCombatAnimationInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FGameplayTag AttackTag;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Combat: Hitbox Metadata"))
class OSECORE_API UAnimMetadata_CombatHitboxes : public UAnimMetaData
{
   GENERATED_BODY()

public:
   // static
   static UAnimMetadata_CombatHitboxes* GetAnimMetadata(const UAnimSequenceBase* animSequenceBase);

   // from UObject
#if WITH_EDITOR
   virtual bool CanEditChange(const FProperty* inProperty) const override;
#endif

   // api

   // combat anim metadata
   void SetCombatAnimationInfo(const FCombatAnimationInfo& combatAnimMetadata) { _combatAnimInfo = combatAnimMetadata; }
   const FCombatAnimationInfo& GetCombatAnimationInfo() const { return _combatAnimInfo; }

   // hitboxes
   void AddHitboxMetadata(int index, const FCombatHitboxMetadata& metadata);
   void ResetHitboxLocations();
   const FCombatHitboxMetadata& GetMetadataForHitboxIndex(int index) const;
   const TMap<int, FCombatHitboxMetadata>& GetHitboxMetadata() const { return _hitboxMetadata; }

   // swing reach
   float GetSwingReach() const { return _maxSwingReach; }

private:
   void _CalculateMaxSwingReach();

private:
   UPROPERTY()
   TMap<int, FCombatHitboxMetadata> _hitboxMetadata;
   UPROPERTY()
   FCombatAnimationInfo _combatAnimInfo;
   UPROPERTY(EditDefaultsOnly, Category = "Combat Anim Metadata")
   float _maxSwingReach = 0.0f;
};
