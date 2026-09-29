// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "OSECameraModifier.generated.h"


//--------------------------------------------------------------------------------------------------
/// Base class for camera modifiers with common functionality.
/// Specifically, tracking the velocity of the camera and handling camera cuts.
//--------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class OSECORE_API UOSECameraModifier : public UCameraModifier
{
   GENERATED_BODY()

public:

   UOSECameraModifier(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

   /// Allows any custom initialization. Called immediately after creation.
   virtual void AddedToCamera(class APlayerCameraManager* Camera) override;

   /// Enables this modifier.
   virtual void EnableModifier() override;

   /// Directly modifies variables in the owning camera. This is a convenient location for this,
   /// as it provides a place to update camera velocities without necessarily having an impact
   /// on blueprint or native modifiers that may inherit from us.
   virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;

   /// Returns the most up-to-date camera velocity vector
   UFUNCTION(BlueprintCallable, Category = "CameraModifier|OSE")
   virtual FVector GetVelocityVector() const { return CameraVelocityVector; }

   /// Returns the scalar velocity in the direction of the rotation
   UFUNCTION(BlueprintCallable, Category = "CameraModifier|OSE")
   virtual float GetVelocityForward(const FRotator& Rotation) const { return CameraVelocityVector | Rotation.RotateVector(FVector::ForwardVector); }

protected:

   /// Allows modifying the camera in native code
   virtual void ModifyCamera(float deltaTime, FVector viewLocation, FRotator viewRotation, float FOV, FVector& newViewLocation, FRotator& newViewRotation, float& newFOV) override;

   /// Allows modifying the post process in native code
   virtual void ModifyPostProcess(float deltaTime, float& postProcessBlendWeight, FPostProcessSettings& postProcessSettings) override;

   /// Updates the camera velocity given the current position and time elapsed.
   /// The computed velocity is returned.
   virtual FVector UpdateCameraVelocity(const float DeltaTime, const FVector& Position);

   /// We keep track of the last camera position in order to calculate an instant velocity. This resets it,
   /// typically when the modifier is toggled or first added
   virtual void ResetVelocity();

private:

   /// @brief Internal utility function to add a modifier
   /// @param InPC The player controller. The modifier is added to the controller's camera manager
   /// @param InModifierClass The class template to use for adding the modifier
   /// @param InModifierObject Optional. The allocated modifier to remove
   /// @return Returns a pointer to the added modifier, or nullptr if unsuccessful
   static UOSECameraModifier* Add_Internal(
      class APlayerController* const InPC,
      const TSubclassOf<UOSECameraModifier>& InModifierClass,
      UOSECameraModifier* const InModifierObject);

   /// @brief Internal utility function to remove a modifier
   /// @param InPC The player controller. The modifier is removed from the controller's camera manager
   /// @param InModifierObject The allocated modifier to remove
   /// @return Returns true if the modifier was removed, false otherwise
   static bool Remove_Internal(class APlayerController* const InPC, UOSECameraModifier* const InModifierObject);

   static UCameraModifier* FindModifierForComponent_Internal(UActorComponent* component, TSubclassOf<UCameraModifier> modifierClass);

   TOptional<FVector> LastCameraPosition;   // Optional; if we have the last camera position we can update velocity
   FVector CameraVelocityVector;         // The world-space velocity vector

public:

   /// @brief Utility function to add a modifier of a specific type
   /// @tparam T The modifier type. Must be a subclass of UOSECameraModifier
   /// @param InPC The player controller. The modifier is added to the controller's camera manager
   /// @param InModifierClass The class template to use for adding the modifier
   /// @param InModifierObject Optional. The allocated modifier to remove
   /// @return Returns a pointer to the added modifier, or nullptr if unsuccessful
   template <class T>
   FORCEINLINE static T* Add(class APlayerController* const InPC, const TSubclassOf<T>& InModifierClass, T* const InModifierObject)
   {
      return Cast<T>(Add_Internal(InPC, InModifierClass, InModifierObject));
   }

   /// @brief Utility function to remove a modifier of a specific type
   /// @tparam T The modifier type. Must be a subclass of UOSECameraModifier
   /// @param InPC The player controller. The modifier is removed from the controller's camera manager
   /// @param InModifierObject The allocated modifier to remove
   /// @return Returns true if the modifier was removed, false otherwise
   template <class T>
   FORCEINLINE static bool Remove(class APlayerController* const InPC, T* const InModifierObject)
   {
      return Remove_Internal(InPC, InModifierObject);
   }

   template <class T>
   FORCEINLINE static T* FindModifierForComponent(UActorComponent* component)
   {
      return Cast<T>(FindModifierForComponent_Internal(component, T::StaticClass()));
   }
};
