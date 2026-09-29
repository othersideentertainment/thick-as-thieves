// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/Modifiers/OSECameraModifier.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraModifier)


UOSECameraModifier::UOSECameraModifier(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
}

// Allows any custom initialization. Called immediately after creation.
void UOSECameraModifier::AddedToCamera(APlayerCameraManager* Camera)
{
   ResetVelocity();

   Super::AddedToCamera(Camera);
}

// Enables this modifier.
void UOSECameraModifier::EnableModifier()
{
   ResetVelocity();

   Super::EnableModifier();
}

// Directly modifies variables in the owning camera. This is a convenient location for this,
// as it provides a place to update camera velocities without necessarily having an impact
// on blueprint or native modifiers that may inherit from us.
bool UOSECameraModifier::ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV)
{
   CameraVelocityVector = UpdateCameraVelocity(DeltaTime, InOutPOV.Location);

   // The base class eventually calls the other modify camera method, update post proc, etc.
   return Super::ModifyCamera(DeltaTime, InOutPOV);
}

/// Allows modifying the camera in native code
void UOSECameraModifier::ModifyCamera(float deltaTime, FVector viewLocation, FRotator viewRotation, float FOV, FVector& newViewLocation, FRotator& newViewRotation, float& newFOV)
{
   Super::ModifyCamera(deltaTime, viewLocation, viewRotation, FOV, newViewLocation, newViewRotation, newFOV);
}

/// Allows modifying the post process in native code
void UOSECameraModifier::ModifyPostProcess(float deltaTime, float& postProcessBlendWeight, FPostProcessSettings& postProcessSettings)
{
   Super::ModifyPostProcess(deltaTime, postProcessBlendWeight, postProcessSettings);
}

// Updates the camera velocity given the current position and time elapsed.
// The computed velocity is returned.
FVector UOSECameraModifier::UpdateCameraVelocity(const float DeltaTime, const FVector& Position)
{
   if (DeltaTime <= KINDA_SMALL_NUMBER)
      return CameraVelocityVector;
      
   // Update the position
   const FVector LocationCurr = Position;
   const FVector LocationPrev = LastCameraPosition.IsSet() ? LastCameraPosition.GetValue() : LocationCurr;
   LastCameraPosition = LocationCurr;

   // Update the instant velocities
   return (LocationCurr - LocationPrev) / DeltaTime;
}

// We keep track of the last camera position in order to calculate an instant velocity. This resets it,
// typically when the modifier is toggled or first added
void UOSECameraModifier::ResetVelocity()
{
   LastCameraPosition.Reset();
   CameraVelocityVector = FVector::ZeroVector;
}

// Internal utility function to add a modifier
UOSECameraModifier* UOSECameraModifier::Add_Internal(
   APlayerController* const InPC,
   const TSubclassOf<UOSECameraModifier>& InModifierClass,
   UOSECameraModifier* const InModifierObject)
{
   // Remove an existing modifier if specified
   if (InModifierObject != nullptr)
   {
      Remove_Internal(InPC, InModifierObject);
   }

   if ((InPC != nullptr) && (InModifierClass != nullptr))
   {
      if (auto CameraMgr = InPC->PlayerCameraManager)
      {
         return Cast<UOSECameraModifier>(CameraMgr->AddNewCameraModifier(InModifierClass));
      }
   }

   return nullptr;
}

// Internal utility function to remove a modifier
bool UOSECameraModifier::Remove_Internal(APlayerController* const InPC, UOSECameraModifier* const InModifierObject)
{
   if ((InPC != nullptr) && (InModifierObject != nullptr))
   {
      if (auto CameraMgr = InPC->PlayerCameraManager)
      {
         if (CameraMgr->RemoveCameraModifier(InModifierObject))
         {
            return true;
         }
      }
   }

   return false;
}

UCameraModifier* UOSECameraModifier::FindModifierForComponent_Internal(UActorComponent* component, TSubclassOf<UCameraModifier> modifierClass)
{
   APawn* pawn = Cast<APawn>(component->GetOwner());
   if (!pawn || !pawn->IsLocallyControlled()) return nullptr;

   APlayerController* controller = pawn->GetController<APlayerController>();
   if (controller == nullptr) return nullptr;

   APlayerCameraManager* cameraManager = controller->PlayerCameraManager;
   if (cameraManager == nullptr) return nullptr;

   return cameraManager->FindCameraModifierByClass(modifierClass);
}

