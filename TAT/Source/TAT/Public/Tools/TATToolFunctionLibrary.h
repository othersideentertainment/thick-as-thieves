// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Tools/TATMeleeWeaponToolComponent.h"

// ue5
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATToolFunctionLibrary.generated.h"

class UAnimMontage;
class UTATMeleeWeaponToolComponent;
class UToolComponent;
class UToolSetComponent;

USTRUCT(BlueprintType)
struct TAT_API FTATLineOfSightTraceParams
{
   GENERATED_BODY()

   /// A target only has line of sight if they're within this distance.
   /// Only enabled if greater than zero.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params", Meta = (UIMin = 0))
   float MaxDistance = 1500.0f;

   /// If enabled and the source actor is a character, checks if the character is conscious (if they are not, they don't have line of sight)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   bool RequireSourceActorConscious = true;

   /// If enabled, uses the AI sight interface to limit the max trace distance.
   /// This factors in sense systems such as light and shadow.
   /// Only works if the source actor is an NPC.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   bool UseAISightToLimitMaxDistance = true;

   /// The source actor only has line of sight on the target if their viewing angle is less than this angle.
   /// If the source actor is looking directly at the target actor, the angle would be 0, the opposite direction would have an angle of 180.
   /// Only enabled if greater than zero.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params", Meta = (UIMin = 0, UIMax = 180, ForceUnits = "degrees"))
   float MaxAngleDeg = 30.0f;

   /// Use a frustum check instead of a dot product check when checking the viewing angle.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   bool UseFrustumCheckForViewAngle = true;

   /// When UseFrustumCheckForViewAngle is enabled, only do a frustum check if the distance to the target is greater than this value.
   /// If the distance is less than this, use a dot product instead (frustum checks can be finicky for line of sight at short distances).
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params", Meta = (UIMin = 0, ClampMin = 0, EditCondition = "UseFrustumCheckForViewAngle"))
   float MinDistanceForFrustumCheck = 400.0f;

   /// What trace channel to use when doing a line-of-sight line trace.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   FCollisionProfileName TraceProfile;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   bool TraceComplex = false;

   /// If specified, the source actor only has line of sight if they have this tag.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   FGameplayTag RequireSourceTag;

   /// If specified, the source actor only has line of sight if the target actor has this tag.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   FGameplayTag RequireTargetTag;

   /// If the target is an interactable, find the interactable's highlight component and target that component specifically.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Of Sight Trace Params")
   bool AllowNonCharacterInteractableTarget = true;
};

UCLASS()
class TAT_API UTATToolFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "TAT|Combat")
   static UAnimMontage* ChooseRandomMontageForAttack(const FTATMeleeWeaponAttack& attackInfo, int32 poolIndex, int32& nextPoolIndex);
   
   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   static FGameplayTag ExtractToolUsageTypeFromContainer(const FGameplayTagContainer& tagContainer);

   static void TryAdjustWorldActorBoxFillPlacement(const AActor* actor, const FTATWorldActorBoxFillExtentConstraints& extentConstraints, FTATWorldActorBoxFillAdjustedTransform& outNewExtents);

   /// Gets all axis-aligned bounding box(s) of the specified character.
   /// This can return more than one bounding box in exactly one edge case: the character is unconscious or lying down.
   ///   In this case, we return both possible bounds - one for falling forward, one for falling backwards.
   /// TODO: This function is a giant hack to work around the fact that we don't replicate enough state to determine the bounding box of a knocked out character on the server and should be removed in the future.
   static int32 GetTargetCharacterBounds(AActor* actor, TArray<FBox, TInlineAllocator<2>>& outBounds);

   UFUNCTION(BlueprintCallable, Category = "TAT|Tools")
   static USceneComponent* FindFirstInteractableComponentInActor(AActor* actor);

   /// Checks if the source actor can see the target actor. Does a number of state checks followed by a trace.
   /// Returns true if the source actor has a line of sight to the target.
   UFUNCTION(BlueprintCallable, Category = "TAT|Tools")
   static bool PerformLineOfSightTrace(FHitResult& outHitResult, AActor* sourceActor, AActor* targetActor, const FTATLineOfSightTraceParams& params, bool debug = false, float debugDrawDuration = -1.0f);

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   static UToolComponent* GetEquippedToolComponentFromActor(AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   static UTATMeleeWeaponToolComponent* GetEquippedMeleeWeaponToolFromActor(AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Tools")
   static UToolSetComponent* GetToolSetComponentFromActor(AActor* actor);
};
