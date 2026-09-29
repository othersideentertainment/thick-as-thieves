// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Character/OSECharacterMovement.h"

#include "TATCharacterMovement.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ETATCustomMovementType : uint8
{
   /// Default custom movement type is none
   None = 0,

   /// Follows a levitator with horizontal air control
   LevitatePassenger = 128
};

namespace TAT
{
   namespace MovementUtils
   {
      /// Helper function to get our custom movement type given the specified movement mode.
      /// This is used internally by the character movement component, but is helpful to
      /// expose when we want to query it without state.
      FORCEINLINE ETATCustomMovementType GetCustomMovementType(EMovementMode movementMode, uint8 customMode)
      {
         return (movementMode != EMovementMode::MOVE_Custom) ? ETATCustomMovementType::None : (ETATCustomMovementType)customMode;
      }
   }
}

enum class ETATCharacterMovementModifierOp : uint8
{
   ClampMax, // If the value is greater than this, set it to the modifier
};

enum class ETATCharacterMovementModifierStat : uint8
{
   None = 0,
   WalkSpeed = 1 << 0,
   SprintSpeed = 1 << 1,
   CrouchSpeed = 1 << 2,
   Acceleration = 1 << 3,
   Friction = 1 << 4,
   BrakingDeceleration = 1 << 5,
};
ENUM_CLASS_FLAGS(ETATCharacterMovementModifierStat);

struct TAT_API FTATCharacterMovementModifier
{
   /// Tag supplied by the caller for this modifier. Used for easy removal of all modifiers with this tag.
   FName ModifierTag;

   /// What movement values to apply this for
   ETATCharacterMovementModifierStat Stat = ETATCharacterMovementModifierStat::None;

   /// Math operation to apply for this modifier
   ETATCharacterMovementModifierOp Op = ETATCharacterMovementModifierOp::ClampMax;

   /// Value to use with the operation
   float Value = 0.0f;
};

struct TAT_API FTATCharacterMovementModifierStack
{
   TArray<FTATCharacterMovementModifier, TInlineAllocator<8>> Mods;

   void Add(const FTATCharacterMovementModifier& mod) { Mods.Add(mod); }
   void Remove(FName tag) { Mods.RemoveAll([tag](const FTATCharacterMovementModifier& mod) { return mod.ModifierTag == tag; }); }
   float Eval(float baseValue, ETATCharacterMovementModifierStat stat) const;
   void Reset() { Mods.Reset(); }
};

// A TAT-level character movement component subclass
UCLASS()
class TAT_API UTATCharacterMovement : public UOSECharacterMovement
{
   GENERATED_BODY()


public:
   UTATCharacterMovement();

   // From UObject
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UCharacterMovement
   virtual void InitializeComponent() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   virtual float GetMaxSpeed() const override;
   virtual float GetMaxAcceleration() const override;
   virtual void UpdateProxyAcceleration() override;

   // Returns MovementMode string
   virtual FString GetMovementName() const override;

   ETATCustomMovementType GetTATCustomMovementType() const { return TAT::MovementUtils::GetCustomMovementType(MovementMode, CustomMovementMode); }
   void SetTATCustomMovementType(ETATCustomMovementType customType) { SetMovementMode(MOVE_Custom, static_cast<uint8>(customType)); }

   bool IsLevitatePassenger() const { return GetTATCustomMovementType() == ETATCustomMovementType::LevitatePassenger; }

   void AddMovementModifier(FName tag, ETATCharacterMovementModifierStat stat, ETATCharacterMovementModifierOp op, float value)
   {
      _movementModifierStack.Add({ tag, stat, op, value });
   }

   void RemoveMovementModifier(FName tag)
   {
      _movementModifierStack.Remove(tag);
   }

protected:
   virtual float GetBaseMaxSpeed() const override;
   virtual void OnMovementModeChanged(EMovementMode previousMovementMode, uint8 previousCustomMode) override;

   virtual void PhysCustom(float deltaTime, int32 iterations) override;
   virtual void PhysCustomLevitatePassenger(float deltaTime, int32 iterations);

   virtual bool ServerExceedsAllowablePositionError(float clientTimeStamp, float deltaTime, const FVector& accel, const FVector& clientWorldLocation, const FVector& relativeClientLocation, UPrimitiveComponent* clientMovementBase, FName clientBaseBoneName, uint8 clientMovementMode) override;
   virtual bool ServerShouldUseAuthoritativePosition(float clientTimeStamp, float deltaTime, const FVector& accel, const FVector& clientWorldLocation, const FVector& relativeClientLocation, UPrimitiveComponent* clientMovementBase, FName clientBaseBoneName, uint8 clientMovementMode) override;

   virtual void OnUnableToFollowBaseMove(const FVector& deltaPosition, const FVector& oldLocation, const FHitResult& moveOnBaseHit) override;

   virtual bool GetWantsToMantle() const override;

   virtual bool CanScrambleInCurrentState() const override;
   virtual bool CanMantleInCurrentState() const override;
   virtual void PopulatePhysicalMaterialContext(FOSEMovementMaterialContext& newContext) const override;

   // From UOSECharacterMovement
   virtual void _ComputeFrictionAndBrakingDeceleration(float& inOutFriction, float& inOutBrakingDeceleration) const override;

private:
   UFUNCTION()
   void _OnOwnerAbilitiesInitialized();

protected:
   // The maximum LevitatePassenger horizontal speed
   UPROPERTY(Category = "Character Movement: Levitate Passenger", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float MaxLevitatePassengerSpeed;

   UPROPERTY(Category = "Character Movement: Levitate Passenger", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
   float BrakingDecelerationLevitatePassenger;
   
   /// Should we try to perform a mantle whenever we are scrambling up a wall
   /// If off, the player will need to manually press the jump input again to trigger
   /// the mantle at the top of the wall
   UPROPERTY(Category = "Character Movement: Scrambling", EditAnywhere)
   bool TryToAutoMantleDuringScramble = false;

private:
   FTATCharacterMovementModifierStack _movementModifierStack;

   UPROPERTY(Transient)
   TWeakObjectPtr<UAbilitySystemComponent> _ownerAbilitySystemComponent;

   FVector _velocityLastFrame = FVector::ZeroVector;
   FVector _simulatedProxyAcceleration = FVector::ZeroVector;
   FVector _simulatedProxyInputDirEstimate = FVector::ZeroVector;
};
