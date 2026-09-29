// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "Engine/DeveloperSettings.h"

#include "OSECoreEditorSettings.generated.h"

USTRUCT(BlueprintType)
struct OSECOREEDITOR_API FOSECustomClassIcon
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   TSoftClassPtr<UObject> Class;

   // generally larger 64x64 sized textures
   UPROPERTY(EditDefaultsOnly)
   TSoftObjectPtr<UTexture2D> ThumbnailImage;

   // generally small 16x16 sized textures
   UPROPERTY(EditDefaultsOnly)
   TSoftObjectPtr<UTexture2D> ClassImage;
};

UCLASS(Config = EditorSettings, DefaultConfig, Meta = (DisplayName = "[OSE] Editor Settings"))
class OSECOREEDITOR_API UOSECoreEditorSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // static get
   static const UOSECoreEditorSettings& Get() { return *GetDefault<UOSECoreEditorSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Class Icons")
   TArray<FOSECustomClassIcon> CustomClassIcons;
};
