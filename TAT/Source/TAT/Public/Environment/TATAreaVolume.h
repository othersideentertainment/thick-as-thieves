// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATAreaVolume.generated.h"

// ose
class UOSEShapeCollisionTrackerComponent;

USTRUCT(BlueprintType)
struct TAT_API FTATAreaInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText Name;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FGameplayTag Tag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   class UPaperSprite* Sprite = nullptr;

#if WITH_EDITOR
   // If either name/tag is assigned, the other should be too. 
   // This allows TATAreaVolume child classes that don't populate this data (like killzone area volumes) to avoid generating map check errors.
   bool IsDataValid() const 
   { 
      const bool nameValid = !Name.IsEmpty();
      const bool tagValid = Tag.IsValid();
      return nameValid == tagValid;
   }
#endif // WITH_EDITOR
};

UCLASS()
class TAT_API ATATAreaVolume : public AActor
{
   GENERATED_BODY()
   
public:   
   ATATAreaVolume();

   // From UObject
#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

   UFUNCTION(BlueprintPure)
   const FTATAreaInfo& GetAreaInfo() const { return Info; }

   UFUNCTION(BlueprintPure)
   UOSEShapeCollisionTrackerComponent* GetShapeCollisionTrackerComponent() const { return _shapeCollisionTrackerComponent; }

protected:
   // from AActor
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

protected:
   UPROPERTY(EditInstanceOnly, Category = "TAT Area")
   FTATAreaInfo Info;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UOSEShapeCollisionTrackerComponent* _shapeCollisionTrackerComponent;
};
