// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"

#include "TATInhibitableInterface.generated.h"

class ATATInhibitorActor;

UENUM(BlueprintType)
enum class ETATInhibitorPlacementType : uint8
{
   Invalid = 0,
   Point,
   Sphere,
   Box,
};

/// The physical parameters of an actor that is being inhibited.
/// This is generally provided by an inhibitable actor.
/// For example, a door might provide a box shape that represents the doorway area, while a lamp might use a point or sphere shape.
/// Inhibitor actors use this placement info to determine where physical colliders and/or VFX should be placed.
USTRUCT(BlueprintType)
struct FTATInhibitorPlacementInfo
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inhibitor Placement Info")
   ETATInhibitorPlacementType Type = ETATInhibitorPlacementType::Invalid;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inhibitor Placement Info")
   FVector WorldLocation = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inhibitor Placement Info")
   FRotator WorldRotation = FRotator::ZeroRotator;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inhibitor Placement Info", Meta = (EditCondition = "Type == ETATInhibitorPlacementType::Sphere"))
   float SphereRadius = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inhibitor Placement Info", Meta = (EditCondition = "Type == ETATInhibitorPlacementType::Box"))
   FVector BoxExtent = FVector::ZeroVector;

   static FTATInhibitorPlacementInfo Make(const FVector& worldLocation, const FRotator& worldRotation = FRotator::ZeroRotator)
   {
      FTATInhibitorPlacementInfo result{};
      result.Type = ETATInhibitorPlacementType::Point;
      result.WorldLocation = worldLocation;
      result.WorldRotation = worldRotation;
      return result;
   }

   static FTATInhibitorPlacementInfo Make(UBoxComponent* boxComponent)
   {
      FTATInhibitorPlacementInfo result{};
      if (boxComponent != nullptr)
      {
         result.Type = ETATInhibitorPlacementType::Box;
         result.WorldLocation = boxComponent->GetComponentLocation();
         result.BoxExtent = boxComponent->GetScaledBoxExtent();
         result.WorldRotation = boxComponent->GetComponentRotation();
      }
      return result;
   }

   static FTATInhibitorPlacementInfo Make(USphereComponent* sphereComponent)
   {
      FTATInhibitorPlacementInfo result{};
      if (sphereComponent != nullptr)
      {
         result.Type = ETATInhibitorPlacementType::Sphere;
         result.WorldLocation = sphereComponent->GetComponentLocation();
         result.SphereRadius = sphereComponent->GetScaledSphereRadius();
         result.WorldRotation = sphereComponent->GetComponentRotation();
      }
      return result;
   }

   bool IsValid() const
   {
      if (Type == ETATInhibitorPlacementType::Point)  { return true; }
      if (Type == ETATInhibitorPlacementType::Sphere) { return SphereRadius > 0.0f; }
      if (Type == ETATInhibitorPlacementType::Box)    { return BoxExtent.X > 0 && BoxExtent.Y > 0 && BoxExtent.Z > 0; }
      return false;
   }
};

/// Interface for an actor that can be targeted and inhibited in some way.
UINTERFACE(BlueprintType, MinimalAPI, Category = "TAT")
class UTATInhibitableInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATInhibitableInterface
{
   GENERATED_BODY()

public:

   /// Checks if this actor can be inhibited at this time by a specific inhibitor type
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inhibitable Interface")
   bool CanBeInhibitedBy(FGameplayTag inhibitorType) const;
   virtual bool CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const { return true; }

   /// Gets he type/category of this object in terms of things that can be inhibited in the world.
   /// This allows inhibitors to decide what they can inhibit (eg. a tool that can inhibit this actor only if it has the type "Inhibitable.Light")
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inhibitable Interface")
   FGameplayTag GetInhibitableType() const;
   virtual FGameplayTag GetInhibitableType_Implementation() const { return FGameplayTag::EmptyTag; }

   /// Gets the World location and area of an inhibitor effect placed on this actor
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inhibitable Interface")
   FTATInhibitorPlacementInfo GetInhibitorPlacementInfo() const;
   virtual FTATInhibitorPlacementInfo GetInhibitorPlacementInfo_Implementation() const { return FTATInhibitorPlacementInfo{}; }

   /// Called when an inhibitor is spawned on this actor to give the actor a chance to react
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inhibitable Interface")
   void OnInhibitorActivated(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount);
   virtual void OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount) {}

   /// Called when an inhibitor on this actor is deactivated to give the actor a chance to react.
   /// If allInhibitorsRemoved is true, there are no remaining inhibitors on this actor and it should return to its normal state.
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inhibitable Interface")
   void OnInhibitorDeactivated(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved);
   virtual void OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved) {}

};
