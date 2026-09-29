// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Engine/DataAsset.h"

// ue
#include "GameplayTagContainer.h"

#include "TATPostMatchFeedbackTipTypes.generated.h"

class ATATPlayerState;
class UPaperSprite;
struct FMatchPersistentData;

USTRUCT(BlueprintType)
struct TAT_API FTATPostMatchFeedbackTipEntry
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UPaperSprite> TipSprite;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText TipText;
};

UCLASS(BlueprintType)
class TAT_API UTATPostMatchFeedbackTipDataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere)
   TArray<FTATPostMatchFeedbackTipEntry> Tips;

   UFUNCTION(BlueprintCallable)
   bool SelectFeedbackTip(const FMatchPersistentData& matchPersistentData, const ATATPlayerState* playerState, FTATPostMatchFeedbackTipEntry& postMatchFeedbackTip) const;
};
