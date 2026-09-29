// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Components/BoxComponent.h"
#include "UObject/Interface.h"

#include "TATWardableInterface.generated.h"

/// World location and extents of a ward placed on an actor
USTRUCT(BlueprintType)
struct FTATWardPlacementInfo
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Location")
   FVector WorldLocation = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Location")
   FVector BoxExtent = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Location")
   FRotator Rotation = FRotator::ZeroRotator;

   FTATWardPlacementInfo() = default;

   explicit FTATWardPlacementInfo(UBoxComponent* boxComponent)
   {
      if (boxComponent != nullptr)
      {
         WorldLocation = boxComponent->GetComponentLocation();
         BoxExtent = boxComponent->GetScaledBoxExtent();
         Rotation = boxComponent->GetComponentRotation();
      }
   }

   FTATWardPlacementInfo(const FVector& worldLocation, const FVector& boxExtent, const FRotator& rotation)
      : WorldLocation(worldLocation)
      , BoxExtent(boxExtent)
      , Rotation(rotation)
   {
   }

   bool IsValid() const
   {
      return BoxExtent.X > 0 && BoxExtent.Y > 0 && BoxExtent.Z > 0;
   }
};

UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools")
class UTATWardableInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface for an actor that can be targeted and blocked by a ward-type tool.
class TAT_API ITATWardableInterface
{
   GENERATED_BODY()

public:

   /// Checks if a ward can be activated at this time
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Tools")
   bool CanActivateWard() const;
   virtual bool CanActivateWard_Implementation() const { return true; }

   /// Gets the world location and size of the ward that will be placed on this actor
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Tools")
   FTATWardPlacementInfo GetWardPlacementInfo() const;
   virtual FTATWardPlacementInfo GetWardPlacementInfo_Implementation() const { return FTATWardPlacementInfo{}; }

   /// Called when the ward is activated to give the actor a chance to react
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Tools")
   void OnWardActivated(AActor* wardActor, APawn* instigator);
   virtual void OnWardActivated_Implementation(AActor* wardActor, APawn* instigator) {}

   /// Called when the ward is deactivated to give the actor a chance to react
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Tools")
   void OnWardDeactivated(AActor* wardActor);
   virtual void OnWardDeactivated_Implementation(AActor* wardActor) {}

};


