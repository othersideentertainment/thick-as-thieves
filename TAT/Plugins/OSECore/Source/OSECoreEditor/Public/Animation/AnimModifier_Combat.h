// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Animation/AnimMetadata_Combat.h"
#include "AnimationModifier.h"

#include "AnimModifier_Combat.generated.h"

USTRUCT(BlueprintType)
struct OSECOREEDITOR_API FCombatHitboxesSettings
{
   GENERATED_BODY()

public:

   FCombatHitboxesSettings();

   /// In this animation sequence, what time do we start adding hitboxes?  This will be the first hitbox time.
   UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", UIMin = "0"), Category = "Combat")
   float StartTime = 0.0f;
   
   /// In this animation sequence, what time do we stop adding hitboxes?  This will be the last hitbox time (inclusive).
   UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", UIMin = "0"), Category = "Combat")
   float EndTime = 0.5f;
   
   /// How many hitboxes do we generate during this time window?
   UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", UIMin = "0"), Category = "Combat")
   int NumHitboxes = 8;

   /// How many hitboxes do we generate during this time window?
   UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", UIMin = "0"), Category = "Combat")
   int HitboxRadius = 50.0f;
   
   /// How much time does each hitbox stay active?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   float Duration = 0.15f;
   
   /// What's the name of the hitbox track we apply our notifies to?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName HitboxTrackNameBase;

   /// Which bone do we sample for the hitbox center point?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName HitboxBoneName;

   /// Which bone do we sample for the hitbox center point?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FVector HitboxOffsetFromBone;

   UPROPERTY(EditInstanceOnly, Category = "Combat|Debug")
   FLinearColor DebugTraceColor = FLinearColor::Red;
   
   UPROPERTY(EditInstanceOnly, Category = "Combat|Debug")
   FLinearColor DebugTraceHitColor = FLinearColor::Green;
};

UCLASS(NotBlueprintable, Config = Editor, DefaultConfig, meta = (DisplayName = "Combat: Generate Attack Hitboxes"))
class OSECOREEDITOR_API UAnimModifier_CombatHitboxes : public UAnimationModifier
{
   GENERATED_BODY()

public:
   UAnimModifier_CombatHitboxes();

   /// Settings for combat hitboxes -- add multiples to give hitboxes to multiple bones, like for dual wielding
   UPROPERTY(EditInstanceOnly, meta = (TitleProperty = HitboxBoneName), Category = "Combat")
   TArray<FCombatHitboxesSettings> Settings;

   /// What's the name of the track we apply our start notify to?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName CombatAnimStartTrackName;

   /// What's the name of the track we apply our end notify to?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName CombatAnimEndTrackName;
   
   /// What's the name of the track we apply our auto aim notify to?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName CombatAnimAutoAimTrackName;

   /// Which bone do we sample for the hitbox center point?
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FName RootBoneName;
   
   /// Setup some metadata to be used by the hit results
   UPROPERTY(EditInstanceOnly, Category = "Combat")
   FCombatAnimationInfo CombatAnimationInfo;

protected:
   // from UAnimationModifier
   virtual void OnApply_Implementation(UAnimSequence* animationSequence) override;
   virtual void OnRevert_Implementation(UAnimSequence* animationSequence) override;
};
