// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/OSECharacterUtils.h"

// ue5
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECharacterUtils)

//---------------------------------------------------------------------------------------
// FOSECharacterUtils::ScopeAllowInput
//---------------------------------------------------------------------------------------

FOSECharacterUtils::ScopeAllowInput::ScopeAllowInput(APawn* inPawn, EMoveInput inMoveInput, ELookInput inLookInput)
   : ScopeAllowInput(inPawn == nullptr ? nullptr : inPawn->Controller, inMoveInput, inLookInput)
{
}

FOSECharacterUtils::ScopeAllowInput::ScopeAllowInput(AController* inController, EMoveInput inMoveInput, ELookInput inLookInput)
   : _controller(inController)
   , _initialMoveInput(EMoveInput::Allowed)
   , _initialLookInput(ELookInput::Allowed)
{
   if (_controller.IsValid())
   {
      _initialMoveInput = _controller->IsMoveInputIgnored() ? EMoveInput::NotAllowed : EMoveInput::Allowed;
      _initialLookInput = _controller->IsLookInputIgnored() ? ELookInput::NotAllowed : ELookInput::Allowed;

      _controller->SetIgnoreMoveInput(inMoveInput == EMoveInput::NotAllowed ? true : false);
      _controller->SetIgnoreLookInput(inLookInput == ELookInput::NotAllowed ? true : false);
   }
}

FOSECharacterUtils::ScopeAllowInput::~ScopeAllowInput()
{
   if (_controller.IsValid())
   {
      _controller->SetIgnoreMoveInput(_initialMoveInput == EMoveInput::NotAllowed ? true : false);
      _controller->SetIgnoreLookInput(_initialLookInput == ELookInput::NotAllowed ? true : false);
   }
}

//---------------------------------------------------------------------------------------
// FOSEPhysicalAnimationParams
//---------------------------------------------------------------------------------------

void FOSEPhysicalAnimationParams::ApplyProperties(UPhysicalAnimationComponent* physAnim)
{
   check(physAnim);

   // apply to the physical animation component
   physAnim->ApplyPhysicalAnimationProfileBelow(BodyName, ProfileName, IncludeSelf, ClearNotFound);
   physAnim->SetStrengthMultiplyer(StrengthMultiplier);
}

//---------------------------------------------------------------------------------------
// FOSEBonePhysicsSimulationParams
//---------------------------------------------------------------------------------------

void FOSEBonePhysicsSimulationParams::ApplyEnabled(USkeletalMeshComponent* mesh)
{
   check(mesh);

   for(FName bone : BoneNamesBelow)
   {
      // apply to the mesh
      mesh->SetAllBodiesBelowSimulatePhysics(bone, Active, IncludeSelf);
   }
}

void FOSEBonePhysicsSimulationParams::SetBlendWeight(USkeletalMeshComponent* mesh, float blendWeight)
{
   check(mesh);

   const bool skipCustomPhysicsType = false; // .... expose?
   for (FName bone : BoneNamesBelow)
   {
      // apply to the mesh
      mesh->SetAllBodiesBelowPhysicsBlendWeight(bone, blendWeight, skipCustomPhysicsType, IncludeSelf);
   }
}

//---------------------------------------------------------------------------------------
// FOSERagdollParams
//---------------------------------------------------------------------------------------

void FOSERagdollParams::ApplyProperties(UPhysicalAnimationComponent* physAnim)
{
   // pass-through
   PhysicalAnimationParams.ApplyProperties(physAnim);
}

void FOSERagdollParams::SetBlendWeight(USkeletalMeshComponent* mesh, float blendWeight)
{
   // pass-through
   BoneSimulationParams.SetBlendWeight(mesh, blendWeight);
}

void FOSERagdollParams::ApplyEnabled(USkeletalMeshComponent* mesh)
{
   // pass-through
   BoneSimulationParams.ApplyEnabled(mesh);
}

