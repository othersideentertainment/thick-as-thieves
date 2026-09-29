// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FOSEGenericGraphEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

void FOSEGenericGraphEditorStyle::Initialize()
{
   const FVector2D Icon20x20(20.0f, 20.0f);
   const FVector2D Icon40x40(40.0f, 40.0f);
   const FVector2D Icon64x64(64.0f, 64.0f);

   if (StyleSet.IsValid())
   {
      return;
   }

   StyleSet = MakeShareable(new FSlateStyleSet("OSEGenericGraphEditorStyle"));

   StyleSet->SetContentRoot(FPaths::ProjectPluginsDir() / TEXT("OSEGenericGraph/Resources"));

   StyleSet->Set("OSEGenericGraphEditor.AutoArrange", new IMAGE_BRUSH("AutoArrangeIcon", Icon40x40));
   StyleSet->Set("OSEGenericGraphEditor.AutoArrange.Small", new IMAGE_BRUSH( "AutoArrangeIcon", Icon20x20 ) );

   FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FOSEGenericGraphEditorStyle::Shutdown()
{
   if (StyleSet.IsValid())
   {
      FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
      ensure(StyleSet.IsUnique());
      StyleSet.Reset();
   }
}

const FName& FOSEGenericGraphEditorStyle::GetStyleSetName()
{
   return StyleSet->GetStyleSetName();
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
