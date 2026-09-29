// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"


#include "TATWorldMapTypes.generated.h"

class UPaperSprite;

// Defines a visual representation of an actor represented on the map screen
USTRUCT(BlueprintType)
struct TAT_API FTATMapSpriteEntry
{
   GENERATED_BODY()

public:
   // Unique identifier so TATMapActorComponent can reference sprite data
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag Tag;

   // Sprite to show on the map
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSoftObjectPtr<UPaperSprite> Sprite;

   // Color used to tint the sprite
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FLinearColor Color = FLinearColor::White;

   // Size of the sprite
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float SpriteSize = 50.f;

   FORCEINLINE bool operator==(FGameplayTag tag) const { return Tag == tag; }
};

// Data describing how an associated UTATMapActorComponent should appear on the map screen
USTRUCT(BlueprintType)
struct TAT_API FTATMapRepresentationData
{
   GENERATED_BODY()

public:

public:
   // If true, only appears on the map when a set distance away from the local player
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool OnlyShowWhenCloseToPlayer = false;

   // If true, will stay on map after it's initial appearance
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool StayVisibleAfterDiscovered = false;
   
   // True if the sprite should rotate to represent the actor's facing direction.
   UPROPERTY(EditDefaultsOnly)
   bool ShowFacingDirection = false;
    
   // True if this actor's location should be represented as a general area, rather than a precise location.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool IsGeneralArea = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool ShowGeneralAreaOnQuestLootObtained = false;
    
   // Radius (in slate-units) used to scale the sprite representation such that it encompasses a general area (if IsGeneralArea == true).
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "IsGeneralArea || ShowGeneralAreaOnQuestLootObtained", EditConditionHides, ClampMin = "0.0", UIMin = "0.0"))
   float GeneralAreaRadius = 0.f;
   FVector2D GeneralAreaOffset = FVector2D::ZeroVector;

   // Color to tint the radius
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "IsGeneralArea || ShowGeneralAreaOnQuestLootObtained", EditConditionHides))
   FLinearColor GeneralAreaColor = FLinearColor::White;

   // Color to tint the radius for remote player actors
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "IsGeneralArea || ShowGeneralAreaOnQuestLootObtained", EditConditionHides))
   FLinearColor AlternativeGeneralAreaColor = FLinearColor::White;

   // The frequency (in seconds) at which the general area updates on the map
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float UpdateGeneralAreaFrequency = 10.0f;
    
   // Tag used to reference the default FTATMapRepresentationSpriteEntry in MapSpriteTable
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "MapSprite"))
   FGameplayTag DefaultSpriteTag;

   // Tag used to reference a different sprite for remote player actors
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "MapSprite"))
   FGameplayTag AlternativeRemoteSpriteTag;
};

// Data asset used to store a tag-identified collection of sprites used for representing actors on the map screen
UCLASS(Blueprintable)
class TAT_API UTATMapSpriteDataAsset : public UDataAsset
{
   GENERATED_BODY()

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:
   // Collection of tag-identified sprite representations used to show the associated actor on the map
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "MapSprite", TitleProperty = "Tag"))
   TArray<FTATMapSpriteEntry> SpriteTable;
};
