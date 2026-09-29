// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Common/TATGameplayTagTableRow.h"

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATCharacterOutfits.generated.h"

class UPaperSprite;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct TAT_API FTATOutfitMetadata
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText OutfitName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText OutfitDescription;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> IconSprite;
};

USTRUCT(BlueprintType, meta=(RowNameTag = "OutfitID"))
struct TAT_API FTATOutfitsMetadataTableRow : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories = "Outfit"))
   FGameplayTag OutfitID;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATOutfitMetadata Metadata;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<USkeletalMesh> OutfitSkeletalMesh;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<USkeletalMesh> OutfitSkeletalMesh1PUpperBody;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<USkeletalMesh> OutfitSkeletalMesh1PLowerBody;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UStaticMesh> OutfitStaticMesh;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Loadout.Slot"))
   FGameplayTag OutfitLoadoutSlotCategory;
};
