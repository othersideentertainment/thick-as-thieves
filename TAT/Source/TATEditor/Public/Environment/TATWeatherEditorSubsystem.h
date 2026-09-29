// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "EditorWorldExtension.h"

// ue4
#include "TATWeatherEditorSubsystem.generated.h"

class ATATWeatherManager;
class ATATWeatherPreset;

UCLASS(BlueprintType, Transient)
class TATEDITOR_API UTATWeatherEditorWorldExtension : public UEditorWorldExtension
{
   GENERATED_BODY()

public:
   // from UEditorWorldExtension
   virtual void Init() override;
   virtual void Shutdown() override;

   void SetEditorDefaultWeatherPresetForLevel();
   void SetWeatherPreset(TSubclassOf<ATATWeatherPreset> newWeatherPreset);
   ATATWeatherPreset* GetWeatherPresetPreviewActor() const { return _weatherPreview; }
   bool UpdateSceneDepthTexture(bool forceUpdate);

protected:
   // from UEditorWorldExtension
   virtual void EnteredSimulateInEditor() override;
   virtual void LeftSimulateInEditor(UWorld* simulateWorld) override;

   ATATWeatherManager* _GetWeatherManager(UWorld* world) const;

   /// The current weather preset actor being previewed in the editor
   UPROPERTY(Transient)
   TObjectPtr<ATATWeatherPreset> _weatherPreview;

   /// The class of the current weather preset actor being previewed in the editor.
   /// This is used so we can temporarily destroy the preview actor during PIE/Simulate and restore it to the correct type.
   UPROPERTY(Transient)
   TSubclassOf<ATATWeatherPreset> _weatherPreviewClass;

private:
   UFUNCTION()
   void _OnSceneDepthTextureUpdatedInEditor(UTexture* depthmapTexture, const FBox& sceneDepthBounds, bool textureUpdated);
};

UCLASS()
class TATEDITOR_API UTATWeatherEditorSubsystem : public UEditorSubsystem
{
   GENERATED_BODY()

public:
   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   /// Gets the weather editor world extension for previewing weather presets for a specific world
   UTATWeatherEditorWorldExtension* GetWeatherEditorWorldExtension(UWorld* world, bool autoCreateIfMissing = true) const;

   //DECLARE_MULTICAST_DELEGATE_OneParam(FEditorWorldChanged)

private:
   void _OnEditorWorldAdded(UWorld* world);
   void _OnEditorWorldDestroyed(UWorld* world);
};
