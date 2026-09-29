// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/RichTextBlockImageDecorator.h"

#include "TATRichTextInputDecorator.generated.h"


class UCommonInputSubsystem;
class AOSEPlayerController;
class UInputAction;

struct FTATInputBrushPair;

USTRUCT(Blueprintable, BlueprintType)
struct FTATRichInputRow : public FTableRowBase
{
   GENERATED_USTRUCT_BODY()

public:

   UPROPERTY(EditAnywhere, Category = Input)
   TObjectPtr<UInputAction> InputAction = nullptr;

   UPROPERTY(EditAnywhere, Category = Input)
   TArray<TObjectPtr<UInputAction>> FallbackActions;
};

// A rich text decorator that shows images for inputs
//
// It has a mapping from name to enhanced InputAction
// Then does a secondary lookup by the key name (The
// latter is slightly different from the lookup used
// by other UI, but keeping the brush used by the base
// class for now.
//
// Does not refresh itself when the input type changes,
// so call RefreshTextLayout when that occurs.
UCLASS()
class TAT_API UTATRichTextInputDecorator : public URichTextBlockDecorator
{
   GENERATED_BODY()

public:
   virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* inOwner) override;
   TArray<FTATInputBrushPair> FindImageBrushes(FName tagOrId, bool warnIfMissing) const;
   FKey FindKey(const FName tagOrId) const;

private:
   const FTATRichInputRow* _FindInputRow(FName tagOrId, bool warnIfMissing) const;
   
   UPROPERTY(EditAnywhere, Category=Appearance, meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATRichInputRow"))
   TObjectPtr<class UDataTable> _inputSet;

   UPROPERTY(Transient)
   TObjectPtr<AOSEPlayerController> _controller;

   UPROPERTY(Transient)
   TObjectPtr<UCommonInputSubsystem> _inputSubsystem;
};
