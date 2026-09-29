// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"

#include "OSEAimAssistComponent.generated.h"

class AHUD;
class AOSEPlayerController;
class APawn;
class UPrimitiveComponent;

//---------------------------------------------------------------------------------------
// FAimAssistTarget
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FAimAssistTarget
{
   GENERATED_BODY()

   friend class UOSEAimAssistComponent;

public:
   UPROPERTY()
   TWeakObjectPtr<AActor> BoundsActor;

   UPROPERTY()
   TWeakObjectPtr<USceneComponent> BoundsComponent;

   UPROPERTY()
   TWeakObjectPtr<USceneComponent> CenterComponent;

   UPROPERTY()
   FVector RawCenter = FVector(ForceInit);

   UPROPERTY()
   FVector RawExtents = FVector(ForceInit);

   UPROPERTY()
   FRotator RawRotator = FRotator(ForceInit);

   UPROPERTY()
   bool UseRawCenterAndExtents = false;

   UPROPERTY()
   bool SkipVisibility = false;

   // What is the local offset of the center from the center component
   UPROPERTY()
   FVector CenterOffset = FVector(ForceInit);

   // how big is the "inner box" in relation to the object bounding box?
   UPROPERTY()
   float InnerBoxSizeMultiplier = 1.0f;

   // how much bigger is the outside "start aim assist here" box than the inside actor bounds box?
   UPROPERTY()
   float OuterBoxSizeMultiplier = 1.2f;

   const FVector& GetCenterLoc() const { return _centerLoc; }
   const FBox2D& GetScreenSpaceBoxInner() const { return _screenSpaceBoxInner; }
   const FBox2D& GetScreenSpaceBoxOuter() const { return _screenSpaceBoxOuter; }
   bool IsTarget() const { return _isTarget; }
   float GetAimAssistStrength() const { return _aimAssistStrength; }
   const FVector2D& GetInnerClosestPoint() const { return _innerClosestPoint; }
   const FVector2D& GetOuterClosestPoint() const { return _outerClosestPoint; }
   const FVector2D& GetScreenSpaceCenterLoc() const { return _screenSpaceCenterLoc; }

   FString GetDebugName() const;

private:
   // cache data when we start tracking this target in the aim assist component
   void _CacheData(const AOSEPlayerController& owningPC, float minBoxSize);
   void _CacheCenterLoc(const AOSEPlayerController& owningPC, USceneComponent& comp);
   void _CacheCenterLocOffset(const AOSEPlayerController& owningPC, USceneComponent& comp, const FVector& offset);
   void _CacheCenterLoc(const AOSEPlayerController& owningPC, AActor& actor);
   void _CacheCenterLoc(const AOSEPlayerController& owningPC, FVector center);
   void _CacheIsOnScreen(const AOSEPlayerController& owningPC);
   void _SetAimAssistStrength(float strength) { _aimAssistStrength = strength; }

private:
   //
   // populated in _CacheData
   //

   // center-ish world xfm of our target
   FVector _centerLoc = FVector::ZeroVector;
   // screen space box that encompasses this target
   FBox2D _screenSpaceBoxInner;
   FBox2D _screenSpaceBoxOuter;
   FVector2D _screenSpaceCenterLoc = FVector2D::ZeroVector;
   bool _isCenterProjectedOnScreen = false;

   //
   // populated when finding our aim assist target:
   //

   // this is our aim assist target
   bool _isTarget = false;
   // this is a normalized value for how close we are between the center and outer boxes
   float _aimAssistStrength = 0.0f;
   FVector2D _innerClosestPoint = FVector2D::ZeroVector;
   FVector2D _outerClosestPoint = FVector2D::ZeroVector;
};

//---------------------------------------------------------------------------------------
// UOSEAimAssistComponent
//---------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSECORE_API UOSEAimAssistComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEAimAssistComponent();

   // from UActorComponent
   virtual void BeginPlay() override;

   // called by UOSEPlayerController to process aim assist info.
   // returns the delta rotation to add to the input.
   virtual FRotator ProcessAimAssist(const AOSEPlayerController& owningPC, const FRotator& inputRot, float deltaTime);

   // called by exernal systems to add aim assist info to be processed next frame
   void AddAimAssistTarget(const FAimAssistTarget& info);

   // HUD lets us draw some stuff for debugging
#if !UE_BUILD_SHIPPING
   void DebugDrawHUD(AHUD& hud);
#endif

protected:
   /// How strong is the aim assist magnetism?  0 is only player input and 1 is only aim assist input.  A value between
   /// the two will give us a balance of aim assist along w/ player input.
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float MagnetStrength = 0.5f;

   /// Minimum box size in world space, even when targets are far away or tiny
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float MinimumBoxSizeWorld = 5.0f;

   /// Degrees per second that the aim assit should track to the target at
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float ConvergenceSpeed = 720.0f;

private:
   void _FilterAimAssistTargets(APawn& owningPawn, const FTransform& pawnEyesXfm);
   FAimAssistTarget* _FindAimAssistTarget(const AOSEPlayerController& owningPC, const FTransform& pawnEyesXfm);
   const AOSEPlayerController& _GetOwningPlayerController() const;

private:
   TArray<FAimAssistTarget> _aimAssistTargets;

#if !UE_BUILD_SHIPPING
   TArray<FAimAssistTarget> _debugDrawAimTargets;
#endif
};

OSECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogAimAssist, Log, All);
