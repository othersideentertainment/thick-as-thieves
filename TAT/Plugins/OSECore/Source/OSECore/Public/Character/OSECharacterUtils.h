// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Engine/DataAsset.h"

#include "OSECharacterUtils.generated.h"

class AController;
class APawn;
class UCurveFloat;
class UInputAction;
class USkeletalMeshComponent;
class UPhysicalAnimationComponent;

struct OSECORE_API FOSECharacterUtils
{
    enum class EMoveInput : uint8
    {
        NotAllowed,
        Allowed,
    };

    enum class ELookInput : uint8
    {
        NotAllowed,
        Allowed,
    };

    class ScopeAllowInput
    {
    public:

        ScopeAllowInput(class APawn* inPawn, EMoveInput inMoveInput, ELookInput inLookInput);
        ScopeAllowInput(class AController* inController, EMoveInput inMoveInput, ELookInput inLookInput);
        ~ScopeAllowInput();

    private:

        TWeakObjectPtr<AController> _controller;
        EMoveInput _initialMoveInput;
        ELookInput _initialLookInput;
    };
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPhysicalAnimationParams
{
   GENERATED_BODY()

   //
   // Construct params
   //

   // feeds into ApplyPhysicalAnimationProfileBelow
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay)
   FName BodyName;

   // feeds into ApplyPhysicalAnimationProfileBelow
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FName ProfileName;
   
   // feeds into ApplyPhysicalAnimationProfileBelow
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   bool IncludeSelf = true;
   
   // feeds into ApplyPhysicalAnimationProfileBelow
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay)
   bool ClearNotFound = true;

   // What is the "Strength Multiplier" of this physics animation vs the character animation?
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay)
   float StrengthMultiplier = 1.0f;

   // is this a valid struct?  profilename is the one piece of required data here
   bool IsValid() const { return !ProfileName.IsNone(); }

   // apply properties
   void ApplyProperties(UPhysicalAnimationComponent* physAnim);
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEBonePhysicsSimulationParams
{
   GENERATED_BODY()

   //
   // Construct params
   //

   // feeds into SetAllBodiesBelowSimulatePhysics
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   TArray<FName> BoneNamesBelow;
   
   // feeds into SetAllBodiesBelowSimulatePhysics
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   bool IncludeSelf = false;

   //
   // Runtime state
   //

   // replicated but not exposed to blueprints
   UPROPERTY()
   bool Active = false;

   // apply properties
   void ApplyEnabled(USkeletalMeshComponent* mesh);
   void SetBlendWeight(USkeletalMeshComponent* mesh, float blendWeight);
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSERagdollParams
{
   GENERATED_BODY()

   // The properties to setup on the PhysicalAnimation component
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FOSEPhysicalAnimationParams PhysicalAnimationParams;
   
   // The properties to setup on the Mesh component
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FOSEBonePhysicsSimulationParams BoneSimulationParams;

   //
   // Construct params
   //

   // how long does this last?  -1 means stay on until it's turned off
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay)
   float Duration = INDEX_NONE;

   // how should we blend authored animation vs physical?  the curve is evaluated at 0-1 between duration start/end
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay)
   UCurveFloat* BlendWeightOverTime = nullptr;
   
   //
   // Runtime state
   //

   // replicated but not exposed to blueprints
   UPROPERTY()
   float ServerEndTime = INDEX_NONE;

   //
   // Pass-through / util Functions
   //

   // is this a valid struct?  profile name is the one piece of required data here
   bool IsValid() const { return !PhysicalAnimationParams.ProfileName.IsNone(); }

   // apply properties
   void ApplyProperties(UPhysicalAnimationComponent* physAnim);

   // update the blend weight
   void SetBlendWeight(USkeletalMeshComponent* mesh, float blendWeight);
   
   // turn it on/off
   void ApplyEnabled(USkeletalMeshComponent* mesh);
};

//--------------------------------------------------------------------------------------------------
// A data asset used to define the input actions for an OSECharacterBase
//--------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSECORE_API UOSECharacterInputActionsAsset : public UDataAsset
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, Category = "Input|Enhanced")
   UInputAction* Move = nullptr;
   UPROPERTY(EditDefaultsOnly, Category = "Input|Enhanced")
   UInputAction* LookMouseKeyboard = nullptr;
   UPROPERTY(EditDefaultsOnly, Category = "Input|Enhanced")
   UInputAction* LookGamepad = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Input|Enhanced")
   UInputAction* Run = nullptr;
   UPROPERTY(EditDefaultsOnly, Category = "Input|Enhanced")
   UInputAction* RunToggle = nullptr;
};
