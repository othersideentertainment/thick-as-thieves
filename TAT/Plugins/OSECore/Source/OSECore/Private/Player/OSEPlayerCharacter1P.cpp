// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/OSEPlayerCharacter1P.h"

// ose
#include "Character/OSECharacterMovement.h"
#include "Graphics/Mesh/SkeletalMeshComponent1P.h"
#include "Items/ToolVisuals.h"

// ue4
#include "AnimationRuntime.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerCharacter1P)


// Enables / disables lower quality, optimized rendering options for 1P meshes
#ifndef OSE_ENABLE_OPTIMIZED_1P_LIGHTING
#define OSE_ENABLE_OPTIMIZED_1P_LIGHTING (1)
#endif


const FName AOSEPlayerCharacter1P::ComponentName_Mesh1P_UpperBody(TEXT("Mesh1P_UpperBody"));
const FName AOSEPlayerCharacter1P::ComponentName_Mesh1P_LowerBody(TEXT("Mesh1P_LowerBody"));
const FName AOSEPlayerCharacter1P::ComponentName_Mesh3P_Head(TEXT("Mesh3P_Head"));


namespace CameraCVars
{
   namespace FPP
   {
      static int32 UseFirstPersonSocketLocation = 1;
      FAutoConsoleVariableRef CVarUseFirstPersonSocketLocation(
         TEXT("OSE.Camera.1P.UseFirstPersonSocketLocation"),
         UseFirstPersonSocketLocation,
         TEXT("Uses the socket location in the 1P mesh for camera positioning"),
         ECVF_Default);

      static int32 UseSeparateGameCameraSocket = 1;
      FAutoConsoleVariableRef CVarUseSeparateGameCameraSocket(
         TEXT("OSE.Camera.1P.UseSeparateGameCameraSocket"),
         UseSeparateGameCameraSocket,
         TEXT("Uses a separate socket for game camera animation"),
         ECVF_Default);

      static int32 UsePlaceholderCameraAnim = 1;
      FAutoConsoleVariableRef CVarUsePlaceholderCameraAnim(
         TEXT("OSE.Camera.1P.UsePlaceholderCameraAnim"),
         UsePlaceholderCameraAnim,
         TEXT("Placeholder camera animation curves"),
         ECVF_Default);
   }
}

namespace FirstPersonUtils
{
   /// Returns the local socket / bone transform in component space. This is an optimised
   /// version of GetSocketTransform(name, ERelativeTransformSpace::RTS_Component) which
   /// doesn't bother with the conversion to and from component space. This also makes using
   /// the first person mesh possible, as the current rotation of the first person mesh
   /// no longer affects the numerical stability of this transform.
   static FTransform GetLocalSocketXfm(const USkinnedMeshComponent* inMeshComponent, FName inSocketName)
   {
      FTransform resultTransform = FTransform::Identity;

      int32 socketBoneIndex;
      FTransform socketLocalTransform;
      const USkeletalMeshSocket* const socketInfo = inMeshComponent->GetSocketInfoByName(inSocketName, socketLocalTransform, socketBoneIndex);

      if (socketInfo != nullptr)
      {
         if (socketBoneIndex != INDEX_NONE)
         {
            const FTransform boneTransform = inMeshComponent->GetBoneTransform(socketBoneIndex, FTransform::Identity);
            resultTransform = socketLocalTransform * boneTransform;
         }
      }
      else
      {
         const int32 boneIndex = inMeshComponent->GetBoneIndex(inSocketName);
         if (boneIndex != INDEX_NONE)
         {
            resultTransform = inMeshComponent->GetBoneTransform(boneIndex, FTransform::Identity);
         }
      }

      return resultTransform;
   }
}


// Sets default values
AOSEPlayerCharacter1P::AOSEPlayerCharacter1P(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , CameraSocket(TEXT("VB Virtual_Camera_1p"), FRotator(90, 90, 0))
   , GameCameraSocket(TEXT("VB Virtual_Camera"))
{
   // Set default mesh to only tick montages if not rendered.
   GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
   GetMesh()->SetLightAttachmentsAsGroup(true);
   GetMesh()->bEnableUpdateRateOptimizations = true;

   // Create lower body first person mesh
   Mesh1P_LowerBody = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(ComponentName_Mesh1P_LowerBody);
   if(Mesh1P_LowerBody)
   {
      Mesh1P_LowerBody->SetupAttachment(GetMesh());
      Mesh1P_LowerBody->SetRelativeRotation(FRotator::ZeroRotator);
      Mesh1P_LowerBody->SetRelativeLocation(FVector::ZeroVector);
      Mesh1P_LowerBody->SetRelativeScale3D(FVector::OneVector);
      Mesh1P_LowerBody->bUseAttachParentBound = true;
      Mesh1P_LowerBody->LeaderPoseComponent = GetMesh();
      Mesh1P_LowerBody->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
      Mesh1P_LowerBody->bEnableUpdateRateOptimizations = true;
      ConfigureMeshComponent(EMeshPerspective::FirstPerson, Mesh1P_LowerBody);
   }

   // Create upper body first person mesh
   Mesh1P_UpperBody = CreateOptionalDefaultSubobject<USkeletalMeshComponent1P>(ComponentName_Mesh1P_UpperBody);
   if(Mesh1P_UpperBody)
   {
      Mesh1P_UpperBody->SetupAttachment(GetMesh());
      Mesh1P_UpperBody->SetRelativeRotation(FRotator::ZeroRotator);
      Mesh1P_UpperBody->SetRelativeLocation(FVector::ZeroVector);
      Mesh1P_UpperBody->SetRelativeScale3D(FVector::OneVector);
      Mesh1P_UpperBody->LeaderPoseComponent = GetMesh();
      Mesh1P_UpperBody->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
      Mesh1P_UpperBody->bEnableUpdateRateOptimizations = true;
      ConfigureMeshComponent(EMeshPerspective::FirstPerson, Mesh1P_UpperBody);
   }

   _didCreate1PMesh = Mesh1P_LowerBody != nullptr && Mesh1P_UpperBody != nullptr;
   // Create head third person mesh

   Mesh3P_Head = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(ComponentName_Mesh3P_Head);
   if(Mesh3P_Head)
   {
      Mesh3P_Head->SetupAttachment(GetMesh());
      Mesh3P_Head->SetRelativeRotation(FRotator::ZeroRotator);
      Mesh3P_Head->SetRelativeLocation(FVector::ZeroVector);
      Mesh3P_Head->SetRelativeScale3D(FVector::OneVector);
      Mesh3P_Head->LeaderPoseComponent = GetMesh();
      Mesh3P_Head->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
      Mesh3P_Head->bEnableUpdateRateOptimizations = true;
      ConfigureMeshComponent(EMeshPerspective::ThirdPerson, Mesh3P_Head);
   }

   // First person characters by default will try to track the aim rotation,
   // for turn-in-place and free look purposes
   if (UOSECharacterMovement* characterMovementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      bUseControllerRotationYaw = false;
      characterMovementComp->bUseControllerDesiredRotation = true;
      characterMovementComp->RotationRate = FRotator::ZeroRotator;
      characterMovementComp->SetTurnInPlaceEnabled(true);
   }
}

// Overridable native event for when play begins for this actor
void AOSEPlayerCharacter1P::BeginPlay()
{
   Super::BeginPlay();
   if(_didCreate1PMesh)
   {
      Mesh1P_LowerBody->AddTickPrerequisiteComponent(GetCameraComponent());
      Mesh1P_UpperBody->AddTickPrerequisiteComponent(GetCameraComponent());
   }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   if (!CameraSocket.Name.IsNone())
   {
      if (!GetMesh3P()->DoesSocketExist(CameraSocket.Name))
      {
         UE_LOG(LogOSECharacter, Warning, TEXT("[%s] CameraSocket.Name - Invalid bone or socket `%s` on character mesh `%s`"), *GetName(), *(CameraSocket.Name.ToString()), *(GetMesh3P()->GetName()));
      }

      if (_didCreate1PMesh && !GetMesh1P()->DoesSocketExist(CameraSocket.Name))
      {
         UE_LOG(LogOSECharacter, Warning, TEXT("[%s] CameraSocket.Name - Invalid bone or socket `%s` on character mesh `%s`"), *GetName(), *(CameraSocket.Name.ToString()), *(GetMesh1P()->GetName()));
      }
   }

   if (!GameCameraSocket.Name.IsNone())
   {
      if (!GetMesh3P()->DoesSocketExist(GameCameraSocket.Name))
      {
         UE_LOG(LogOSECharacter, Warning, TEXT("[%s] GameCameraSocket.Name - Invalid bone or socket `%s` on character mesh `%s`"), *GetName(), *(GameCameraSocket.Name.ToString()), *(GetMesh3P()->GetName()));
      }

      if (_didCreate1PMesh && !GetMesh1P()->DoesSocketExist(GameCameraSocket.Name))
      {
         UE_LOG(LogOSECharacter, Warning, TEXT("[%s] GameCameraSocket.Name - Invalid bone or socket `%s` on character mesh `%s`"), *GetName(), *(GameCameraSocket.Name.ToString()), *(GetMesh1P()->GetName()));
      }
   }

#endif
}

// Overridable function called whenever this actor is being removed from a level
void AOSEPlayerCharacter1P::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   if(_didCreate1PMesh)
   {
      Mesh1P_LowerBody->RemoveTickPrerequisiteComponent(GetCameraComponent());
      Mesh1P_UpperBody->RemoveTickPrerequisiteComponent(GetCameraComponent());
   }
   Super::EndPlay(EndPlayReason);
}

void AOSEPlayerCharacter1P::SetActorHiddenInGame(bool newHidden)
{
   const bool wasHidden = IsHidden();

   Super::SetActorHiddenInGame(newHidden);

   if (wasHidden != newHidden)
   {
      // visible -> hidden
      if (newHidden)
      {
         _shadowCastingMeshComponents.Reset();

         ForEachComponent<UMeshComponent>(false, [this](UMeshComponent* mesh)
         {
            if (mesh->CastShadow && mesh->bCastHiddenShadow)
            {
               mesh->SetCastShadow(false);
               this->_shadowCastingMeshComponents.Add(mesh);
            }
         });
      }
      // hidden -> visible
      else
      {
         for (const TWeakObjectPtr<UMeshComponent>& meshWeakPtr : _shadowCastingMeshComponents)
         {
            if (UMeshComponent* mesh = meshWeakPtr.Get())
            {
               mesh->SetCastShadow(true);
            }
         }

         _shadowCastingMeshComponents.Reset();
      }
   }
}

void AOSEPlayerCharacter1P::CalcCamera(float deltaTime, FMinimalViewInfo& outResult)
{
   Super::CalcCamera(deltaTime, outResult);

   // Pushes the camera forward as it pitches down
   const float forwardAmt = GetCapsuleComponent()->GetScaledCapsuleRadius();
   const FVector forwardVec = outResult.Rotation.Vector();
   outResult.Location -= forwardVec.GetSafeNormal2D() * FMath::Min(0, (forwardVec.Z * forwardAmt));

   // @TODO: Placeholder! Largely copied from previous AOSEPlayerCharacter1P::TickCamera code
   if (CameraCVars::FPP::UsePlaceholderCameraAnim != 0)
   {
      const FTransform sourceTransform = FTransform(outResult.Rotation, outResult.Location);
      FTransform targetTransform = sourceTransform;

      const FPlayerCameraSocket camSocket = (CameraCVars::FPP::UseSeparateGameCameraSocket) ? GameCameraSocket : CameraSocket;

      const USkeletalMeshComponent* MeshComponent = GetMesh3P();
      UAnimInstance* animInst = MeshComponent->GetAnimInstance();
      if (animInst && !camSocket.Name.IsNone())
      {
         // Updates the camera end transform to be set at a blended value
         // between the default relative transform (identity) and the
         // position and orientation of the specified camera socket on
         // the third person mesh. Used to blend between fully procedural
         // and fully animated camera transforms.
         FTransform camSocketLocalXfm;
         int32 camSocketBoneIdx;
         const USkeletalMeshSocket* meshSocket = MeshComponent->GetSocketInfoByName(camSocket.Name, camSocketLocalXfm, camSocketBoneIdx);

         if ((meshSocket != nullptr) && (camSocketBoneIdx != INDEX_NONE))
         {
            // Reference transform to orient the socket
            const FTransform referenceXfm = camSocket.GetRelativeTransform();

            // World-space socket transform (i.e. a bone)
            FTransform socketXfm = referenceXfm * (camSocketLocalXfm * MeshComponent->GetBoneTransform(camSocketBoneIdx));
            _ModifyCameraTransform(deltaTime, socketXfm);

            // World-space socket transform for the reference pose
            FTransform socketXfmRefPose = FAnimationRuntime::GetComponentSpaceTransformRefPose(MeshComponent->GetSkinnedAsset()->GetRefSkeleton(), camSocketBoneIdx);
            socketXfmRefPose = referenceXfm * (camSocketLocalXfm * (socketXfmRefPose * MeshComponent->GetComponentTransform()));
            const FTransform deltaXfmRefPose = socketXfm.GetRelativeTransform(socketXfmRefPose);
            const FTransform relativeAnim = deltaXfmRefPose;

            targetTransform = relativeAnim * sourceTransform;
            outResult.Rotation = targetTransform.Rotator();
            outResult.Location = targetTransform.GetLocation();
         }
      }
   }

   UpdateFirstPersonMesh(deltaTime, FTransform(outResult.Rotation, outResult.Location));
}

// Utility method to update the first person mesh to match the camera
bool AOSEPlayerCharacter1P::UpdateFirstPersonMesh(float deltaTime, const FTransform& cameraXfm)
{
   if(_didCreate1PMesh == false)
      return false;
   // Update the first person mesh transform to always be relative to
   // the position and orientation of the specified camera socket
   const FPlayerCameraSocket camSocket = CameraSocket;
   if (camSocket.Name.IsNone())
      return false;

   // Whether to use the 1P or 3P camera socket orientation for the 1P mesh
   const USkeletalMeshComponent* const meshComponent = (CameraCVars::FPP::UseFirstPersonSocketLocation) ? GetMesh1P() : GetMesh3P();
   if (!meshComponent->DoesSocketExist(camSocket.Name))
      return false;

   // Reference transform to orient the socket
   const FTransform referenceXfm = camSocket.GetRelativeTransform();

   // Component-space socket transform (i.e. a bone)
   FTransform socketXfm = referenceXfm * FirstPersonUtils::GetLocalSocketXfm(meshComponent, camSocket.Name);
   _ModifyFirstPersonMeshViewTransform(deltaTime, socketXfm);

   // Compute and set the world transform
   const FTransform firstPersonMeshXfm = socketXfm.GetRelativeTransformReverse(cameraXfm);
   GetMesh1P_UpperBody()->SetWorldTransform(firstPersonMeshXfm);

   return true;
}

void AOSEPlayerCharacter1P::ConfigureMeshComponent(const EMeshPerspective meshPerspective, USkeletalMeshComponent* meshComponent)
{
   switch (meshPerspective)
   {
   case EMeshPerspective::ThirdPerson:
   {
      // Visibility
      meshComponent->SetOwnerNoSee(true);
      meshComponent->SetOnlyOwnerSee(false);
      meshComponent->SetReceivesDecals(true);

      // Lighting
      meshComponent->SetCastShadow(true);
      meshComponent->bCastHiddenShadow = true;

#if (OSE_ENABLE_OPTIMIZED_1P_LIGHTING)

      // When this is enabled, light attached child meshes
      // like the third person mesh
      meshComponent->SetLightAttachmentsAsGroup(true);
#endif

   }
   break;

   case EMeshPerspective::FirstPerson:
   {
      // Visibility
      meshComponent->SetOwnerNoSee(false);
      meshComponent->SetOnlyOwnerSee(true);
      meshComponent->SetReceivesDecals(false);

      // Lighting
      meshComponent->SetCastShadow(true);
      meshComponent->bCastHiddenShadow = false;

#if (OSE_ENABLE_OPTIMIZED_1P_LIGHTING)

      // Optimized; disable shadows and use a single shadow sample
      meshComponent->SetCastShadow(false);
      meshComponent->SetSingleSampleShadowFromStationaryLights(true);
#else

      // Higher quality; self-shadowing enabled
      meshComponent->SetCastShadow(true);
      meshComponent->bSelfShadowOnly = true;
#endif

      // Collision
      meshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
      meshComponent->SetGenerateOverlapEvents(false);
      meshComponent->SetCanEverAffectNavigation(false);
   }
   break;
   }
}

//---------------------------------------------------------------------------------------
// IToolHolderInterface
//---------------------------------------------------------------------------------------

USceneComponent* AOSEPlayerCharacter1P::GetToolRoot(const EMeshPerspective meshPerspective) const
{
   switch (meshPerspective)
   {
   case EMeshPerspective::ThirdPerson:
      return GetMesh3P();

   case EMeshPerspective::FirstPerson:
      return GetMesh1P();
   }

   return nullptr;
}

