// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSECoreCheats.h"

// ue
#include "Components/MaterialBillboardComponent.h"

#include "TATGlyphComponent.generated.h"

class IConsoleVariable;
class UMaterialInstanceDynamic;

///
/// Glyph component base class.
/// This class is just responsible for rendering a glyph and interpolating its opacity when the glyph's visibility changes.
///
UCLASS(BlueprintType, Blueprintable, DontCollapseCategories, HideCategories=(Replication, ComponentReplication, Streaming, Mobile, LOD, HLOD, Cooking, ComponentTick), Meta = (BlueprintSpawnableComponent))
class TAT_API UTATGlyphComponent : public UMaterialBillboardComponent
{
   GENERATED_BODY()

public:
   UTATGlyphComponent(const FObjectInitializer& objectInitializer);

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   //// From UActorComponent
   virtual void OnRegister() override;
   virtual void OnUnregister() override;
   virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction *thisTickFunction) override;

public:
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void SetGlyphTexture(UTexture* glyphTexture);

   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void SetGlyphColors(FLinearColor primaryColor, FLinearColor secondaryColor);

   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   UMaterialInstanceDynamic* GetOrCreateDynamicMaterialInstance();

   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   float GetOpacityFadeSeconds() const { return _opacityFadeSeconds; }

   // Returns the default colors used by the glyph material
   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   bool GetGlyphColorParamDefaultValues(FLinearColor& outPrimaryColor, FLinearColor& outSecondaryColor) const;

   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   bool GetGlyphTextureDefaultValue(UTexture*& outTexture) const;

   static bool IsGlyphDebugModeEnabled();

   static void SetGlyphDebugMode(bool enabled, bool setFromCheat = false);

   /// Set the glyph to be visible or invisible.
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   virtual void SetGlyphVisibility(bool newVisible);

   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   FORCEINLINE bool IsGlyphVisible() const { return _isVisible; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGlyphVisibilityChanged, bool, isVisible);
   UPROPERTY(BlueprintAssignable)
   FGlyphVisibilityChanged OnGlyphVisibilityChanged;

protected:
   void _SetGlyphVisible(bool shouldBeVisible, bool instant = false);

   UMaterialInstanceDynamic* _ConstructDynamicMaterialInstance();

   virtual void _DrawDebugGlyph(const FVector& playerLocation);

#if OSE_CHEATS_ENABLED
private:
   FDelegateHandle _debugCvarChangedHandle;
   FTSTicker::FDelegateHandle _debugDrawTickHandle;

   void _OnDebugCvarChanged(IConsoleVariable* cvar);

   /// Tick function that calls _DrawDebugGlyph every frame
   bool _AutoDrawDebugGlyphTick(float worldDeltaSeconds);

protected:
   /// The debug draw ticking function is only run when this is true.
   /// Can be set to false in subclasses to handle calling _DrawDebugGlyph manually.
   bool _enableAutoDrawDebugGlyphTick = true;
#endif

protected:
   /// Time in seconds for the glyph to fade from visible <-> invisible
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph", meta = (UIMin = "0.1", ClampMin = "0.1"))
   float _opacityFadeSeconds = 1.f;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph")
   FName _materialOpacityParamName;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph")
   FName _materialGlyphTextureParamName;

   /// Name of the "primary color" parameter in the glyph material
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph")
   FName _materialGlyphPrimaryColorParamName;

   /// Name of the "secondary color" parameter in the glyph material
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph")
   FName _materialGlyphSecondaryColorParamName;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph")
   FMaterialSpriteElement _glyphMaterialElement;

private:
   UPROPERTY(Transient)
   bool _isVisible = false;

   UPROPERTY(Transient)
   UMaterialInstanceDynamic* _glyphDynamicMaterialInstance = nullptr;
};
