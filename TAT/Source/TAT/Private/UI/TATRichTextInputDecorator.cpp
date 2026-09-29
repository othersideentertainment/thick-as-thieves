// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/TATRichTextInputDecorator.h"

// tat
#include "Developer/TATProjectSettings.h"

// ose
#include "Input/OSEInputFunctionLibrary.h"
#include "Player/OSEPlayerController.h"

// ue
#include "CommonInputSubsystem.h"
#include "CommonUIUtils.h"
#include "InputAction.h"
#include "Components/RichTextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Misc/DefaultValueHelper.h"
#include "Widgets/Layout/SScaleBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRichTextInputDecorator)

struct FTATInputBrushPair
{
   TOptional<FSlateBrush> PrimaryBrush;
   TOptional<FSlateBrush> ChordBrush;

   bool IsValid() const { return PrimaryBrush.IsSet(); }
};

// NOTE: Much of this code is adapted from RichTextBlockImageDecorator
//       it was originally a subclass, but needed to be able to show
//       multiple icons for choorded inputs, and so has been rewritten
//       to create its own slate widget and decorator.

class STATRichInlineInputImage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STATRichInlineInputImage)
	{}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& inArgs, FTATInputBrushPair brushPair, const FTextBlockStyle& textStyle, TOptional<int32> width, TOptional<int32> height, EStretch::Type stretch)
	{
		check(brushPair.IsValid());
      _inputBrushes = brushPair;

		const TSharedRef<FSlateFontMeasure> fontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		float iconHeight = FMath::Min((float)fontMeasure->GetMaxCharacterHeight(textStyle.Font, 1.0f), _inputBrushes.PrimaryBrush->ImageSize.Y);
      if (CommonUIUtils::ShouldDisplayMobileUISizes())
      {
         iconHeight *= UTATProjectSettings::Get().RichTextIconMobileScale;
      }

		if (height.IsSet())
		{
			iconHeight = static_cast<float>(height.GetValue());
		}

		float iconWidth = iconHeight;
		if (width.IsSet())
		{
			iconWidth = static_cast<float>(width.GetValue());
		}

      auto makeBoxForBrush = [iconHeight, iconWidth, stretch](const FSlateBrush* brush)
         {
            return SNew(SHorizontalBox)
               + SHorizontalBox::Slot()
               [
                  SNew(SBox)
                     .HeightOverride(iconHeight)
                     .WidthOverride(iconWidth)
                     [
                        SNew(SScaleBox)
                           .Stretch(stretch)
                           .StretchDirection(EStretchDirection::DownOnly)
                           .VAlign(VAlign_Center)
                           [
                              SNew(SImage)
                                 .Image(brush)
                           ]
                     ]
               ];
         };

	   if(_inputBrushes.ChordBrush)
	   {
	      ChildSlot
         [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            [
               makeBoxForBrush(_inputBrushes.ChordBrush.GetPtrOrNull())
            ]
            +SHorizontalBox::Slot()
            [
               makeBoxForBrush(_inputBrushes.PrimaryBrush.GetPtrOrNull())
            ]
         ];
	   }
      else
      {
         ChildSlot
         [
            makeBoxForBrush(_inputBrushes.PrimaryBrush.GetPtrOrNull())
         ];
      }
	}

private:
   // Have to hold onto the Brushes somewhere, since the slate widget takes the brush by
   // stable pointer. Ideally this would be in a UPROPERTY, but none of the decorator uobjects
   // have a lifetime that aligns with a single one of these. It should be safe in practice,
   // since other copies of the brushes will keep it alive in the source data.
   FTATInputBrushPair _inputBrushes;
};

/** 
 * Add an image inline with the text.
 * Usage: Before image <img id="MyId"/>, after image.
 * 
 * A width and height can be specified.
 * By default the width and the height is the same size as the font height.
 * Use "desired" to use the same size as the image brush.
 * Usage: Before image <img id="MyId" height="40" width="desired"/>, after image.
 * 
 * A stretch type can be specified. See EStretch.
 * By default the stretch type is ScaleToFit.
 * Usage: Before image <img id="MyId" stretch="ScaleToFitY"/>, after image.
 */
class FTATRichInlineInputImage : public FRichTextDecorator
{
public:
	FTATRichInlineInputImage(URichTextBlock* inOwner, UTATRichTextInputDecorator* inDecorator)
		: FRichTextDecorator(inOwner)
		, _decorator(inDecorator)
	{
	}

	virtual bool Supports(const FTextRunParseResults& runParseResult, const FString& text) const override
	{
		if (runParseResult.Name == TEXT("img") && runParseResult.MetaData.Contains(TEXT("id")))
		{
		   const FTextRange& idRange = runParseResult.MetaData[TEXT("id")];
		   const FString tagId = text.Mid(idRange.BeginIndex, idRange.EndIndex - idRange.BeginIndex);
		   if (_decorator && _decorator->FindKey(FName(tagId)).IsValid())
		   {
		      return true;
		   }
			return false;
		}

		return false;
	}

protected:
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& runInfo, const FTextBlockStyle& textStyle) const override
	{
		constexpr bool bWarnIfMissing = true;
		TArray<FTATInputBrushPair> brushes = _decorator->FindImageBrushes(*runInfo.MetaData[TEXT("id")], bWarnIfMissing);
	   if (brushes.Num() > 0)
	   {
	      auto horizontalContainer = SNew(SHorizontalBox);

         for (const FTATInputBrushPair& brush : brushes)
         {
            TOptional<int32> width;
            if (const FString* widthString = runInfo.MetaData.Find(TEXT("width")))
            {
               int32 widthTemp;
               if (FDefaultValueHelper::ParseInt(*widthString, widthTemp))
               {
                  width = widthTemp;
               }
               else if (FCString::Stricmp(GetData(*widthString), TEXT("desired")) == 0)
               {
                  width = FMath::TruncToInt32(brush.PrimaryBrush->ImageSize.X);
               }
            }

            TOptional<int32> height;
            if (const FString* heightString = runInfo.MetaData.Find(TEXT("height")))
            {
               int32 heightTemp;
               if (FDefaultValueHelper::ParseInt(*heightString, heightTemp))
               {
                  height = heightTemp;
               }
               else if (FCString::Stricmp(GetData(*heightString), TEXT("desired")) == 0)
               {
                  height = FMath::TruncToInt32(brush.PrimaryBrush->ImageSize.Y);
               }
            }

            EStretch::Type stretch = EStretch::ScaleToFit;
            if (const FString* stretchString = runInfo.MetaData.Find(TEXT("stretch")))
            {
               const UEnum* stretchEnum = StaticEnum<EStretch::Type>();
               int64 stretchValue = stretchEnum->GetValueByNameString(*stretchString);
               if (stretchValue != INDEX_NONE)
               {
                  stretch = static_cast<EStretch::Type>(stretchValue);
               }
            }
            horizontalContainer->AddSlot().AttachWidget(SNew(STATRichInlineInputImage, brush, textStyle, width, height, stretch));
         }
			return horizontalContainer;
		}
	   
	   // fallback to directly outputting the key.
	   const FKey key = _decorator->FindKey(*runInfo.MetaData[TEXT("id")]);
	   FSlateFontInfo fontInfo = UTATProjectSettings::GetTATSettings()->RichTextInputFont;
		return SNew(STextBlock).Text(key.GetDisplayName(false)).Font(fontInfo);
	}

private:
	UTATRichTextInputDecorator* _decorator;
};

TSharedPtr<ITextDecorator> UTATRichTextInputDecorator::CreateDecorator(URichTextBlock* inOwner)
{
   _controller = inOwner->GetOwningPlayer<AOSEPlayerController>();
   _inputSubsystem = UCommonInputSubsystem::Get(inOwner->GetOwningLocalPlayer());
   return MakeShareable(new FTATRichInlineInputImage(inOwner, this));
}

TArray<FTATInputBrushPair> UTATRichTextInputDecorator::FindImageBrushes(FName tagOrId, bool warnIfMissing) const
{
   auto findBrushImage = [this](const FKey& key) -> TOptional<FSlateBrush>
   {
      if(!key.IsValid()) return NullOpt;

      FSlateBrush brush;
      if(UCommonInputPlatformSettings::Get()->TryGetInputBrush(brush, key, _inputSubsystem->GetCurrentInputType(), _inputSubsystem->GetCurrentGamepadName()))
      {
         return brush;
      }
      return NullOpt;
   };
   
   if (const FTATRichInputRow* row = _FindInputRow(tagOrId, warnIfMissing))
   {
      if (!IsValid(_controller) || !IsValid(_inputSubsystem))
      {
         return {};
      }

      auto lookupKeyImages = [this, &findBrushImage](const UInputAction* inputAction) -> TArray<FTATInputBrushPair>
      {
         const TArray<FOSEKeyAndChordPair> keys = UOSEInputFunctionLibrary::GetKeysForInputAction(_controller, inputAction, _controller->GetCurrentInputHardwareType());
         TArray<FTATInputBrushPair> results;
         for (const FOSEKeyAndChordPair& pair : keys)
         {
            FTATInputBrushPair brushPair {findBrushImage(pair.Key), findBrushImage(pair.Chord)};
            if (brushPair.IsValid())
            {
               results.Add(brushPair);
            }
         }
         return results;
      };
      
      TArray<FTATInputBrushPair> results = lookupKeyImages(row->InputAction);
      if (results.Num() > 0)
      {
         return results;
      }
      results = lookupKeyImages(row->InputAction);

      // use fallbacks if there are multiple logical inputs (e.g. look)
      for (const UInputAction* fallbackAction : row->FallbackActions)
      {
         results = lookupKeyImages(fallbackAction);
         if (results.Num() > 0)
         {
            return results;
         }
      }
   }
   
   return {};
}

FKey UTATRichTextInputDecorator::FindKey(const FName tagOrId) const
{
   if (IsValid(_controller) == false)
      return FKey();
   if (const FTATRichInputRow* row = _FindInputRow(tagOrId, false))
   {
      FKey chordedKey;
      FKey outputKey = UOSEInputFunctionLibrary::GetKeyAndChordForInputAction(_controller, row->InputAction, _controller->GetCurrentInputHardwareType(), chordedKey);
      if (outputKey.IsValid())
         return outputKey;
      // Try the fallbacks
      for (const TObjectPtr<UInputAction>& action : row->FallbackActions)
      {
         outputKey = UOSEInputFunctionLibrary::GetKeyAndChordForInputAction(_controller, action, _controller->GetCurrentInputHardwareType(), chordedKey);
         if (outputKey.IsValid())
         {
            return outputKey;
         }
      }
   }
   return FKey();
}

const FTATRichInputRow* UTATRichTextInputDecorator::_FindInputRow(FName tagOrId, bool warnIfMissing) const
{
   if (_inputSet)
   {
      return _inputSet->FindRow<FTATRichInputRow>(tagOrId, TEXT("_FindInputAction"), warnIfMissing);
   }
	
   return nullptr;
}
