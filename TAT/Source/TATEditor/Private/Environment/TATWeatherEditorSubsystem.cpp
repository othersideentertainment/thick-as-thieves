// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherEditorSubsystem.h"

// tat
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherEditorUtilities.h"
#include "GameFramework/TATWorldSettings.h"
#include "Environment/TATWeatherManager.h"
#include "Environment/TATWeatherPreset.h"

// ue
#include "Engine/TextureRenderTarget2D.h"
#include "NiagaraParameterCollection.h"
#include "Materials/MaterialParameterCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherEditorSubsystem)
DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherEditorSubsystem, Log, All);

namespace WeatherEditorSubsystemHelpers
{
   UEditorWorldExtensionCollection* GetEditorWorldExtensionCollection(UWorld* world, bool createIfNeeded = true)
   {
      if (world != nullptr && GEditor != nullptr)
      {
         if (UEditorWorldExtensionManager* extManager = GEditor->GetEditorWorldExtensionsManager())
         {
            return extManager->GetEditorWorldExtensions(world, createIfNeeded);
         }
      }
      return nullptr;
   }

   template<typename T>
   T* GetEditorWorldExtension(UWorld* world)
   {
      UEditorWorldExtensionCollection* extCollection = GetEditorWorldExtensionCollection(world);
      return (extCollection != nullptr) ? Cast<T>(extCollection->FindExtension(T::StaticClass())) : nullptr;
   }

   template<typename T>
   T* GetEditorWorldExtension(UEditorWorldExtensionCollection* extCollection)
   {
      return (extCollection != nullptr) ? Cast<T>(extCollection->FindExtension(T::StaticClass())) : nullptr;
   }
}

//
// UTATWeatherEditorSubsystem
//

void UTATWeatherEditorSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   GEditor->OnWorldAdded().AddUObject(this, &UTATWeatherEditorSubsystem::_OnEditorWorldAdded);
   GEditor->OnWorldDestroyed().AddUObject(this, &UTATWeatherEditorSubsystem::_OnEditorWorldDestroyed);
}

UTATWeatherEditorWorldExtension* UTATWeatherEditorSubsystem::GetWeatherEditorWorldExtension(UWorld* world, bool autoCreateIfMissing) const
{
   UTATWeatherEditorWorldExtension* worldExt = WeatherEditorSubsystemHelpers::GetEditorWorldExtension<UTATWeatherEditorWorldExtension>(world);

   if (worldExt == nullptr && autoCreateIfMissing)
   {
      if (UEditorWorldExtensionCollection* extCollection = WeatherEditorSubsystemHelpers::GetEditorWorldExtensionCollection(world))
      {
         UE_LOG(LogTATWeatherEditorSubsystem, Verbose, TEXT("Creating new weather editor world extension for world %p '%s'"), world, *world->GetName());
         worldExt = NewObject<UTATWeatherEditorWorldExtension>(extCollection);
         check(worldExt != nullptr);
         extCollection->AddExtension(worldExt);
      }
   }

   return worldExt;
}

void UTATWeatherEditorSubsystem::_OnEditorWorldAdded(UWorld* world)
{
   UE_LOG(LogTATWeatherEditorSubsystem, Verbose, TEXT("UTATWeatherEditorSubsystem::_OnEditorWorldAdded(%p)"), world);
   check(world != nullptr);

   // Set the default preset for this world if needed
   constexpr bool autoCreateIfMissing = true;
   if (UTATWeatherEditorWorldExtension* worldExt = GetWeatherEditorWorldExtension(world, autoCreateIfMissing))
   {
      worldExt->AddToRoot(); // don't GC until its world is destroyed
      worldExt->SetEditorDefaultWeatherPresetForLevel();
   }
}

void UTATWeatherEditorSubsystem::_OnEditorWorldDestroyed(UWorld* world)
{
   UE_LOG(LogTATWeatherEditorSubsystem, Verbose, TEXT("UTATWeatherEditorSubsystem::_OnEditorWorldDestroyed(%p)"), world);
   check(world != nullptr);

   // Destroy any existing editor world extension set up for this world
   constexpr bool createIfNeeded = false;
   if (UEditorWorldExtensionCollection* extCollection = WeatherEditorSubsystemHelpers::GetEditorWorldExtensionCollection(world, createIfNeeded))
   {
      if (UTATWeatherEditorWorldExtension* worldExt = WeatherEditorSubsystemHelpers::GetEditorWorldExtension<UTATWeatherEditorWorldExtension>(extCollection))
      {
         worldExt->RemoveFromRoot(); // allow GC now that its world is going away
         extCollection->RemoveExtension(worldExt);
      }
   }
}

//
// UTATWeatherEditorWorldExtension
//

void UTATWeatherEditorWorldExtension::Init()
{
   Super::Init();

   UWorld* world = GetWorld();
   check(world != nullptr);
   if (ATATWeatherManager* weatherManager = _GetWeatherManager(world))
   {
      weatherManager->OnUpdateSceneDepthTexture.AddDynamic(this, &UTATWeatherEditorWorldExtension::_OnSceneDepthTextureUpdatedInEditor);
   }
}

void UTATWeatherEditorWorldExtension::Shutdown()
{
   if (ATATWeatherManager* weatherManager = _GetWeatherManager(GetWorld()))
   {
      weatherManager->OnUpdateSceneDepthTexture.RemoveDynamic(this, &UTATWeatherEditorWorldExtension::_OnSceneDepthTextureUpdatedInEditor);
   }

   SetWeatherPreset(nullptr);
   ensure(_weatherPreview == nullptr);
   ensure(_weatherPreviewClass == nullptr);

   Super::Shutdown();
}

void UTATWeatherEditorWorldExtension::SetEditorDefaultWeatherPresetForLevel()
{
   UWorld* world = GetWorld();
   if (world == nullptr)
   {
      return;
   }

   if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
   {
      SetWeatherPreset(worldSettings->EditorDefaultWeatherPreset.LoadSynchronous());
   }
   else
   {
      // If we don't have a weather manager, don't show a preset at all
      SetWeatherPreset(nullptr);
   }
}

void UTATWeatherEditorWorldExtension::SetWeatherPreset(TSubclassOf<ATATWeatherPreset> newWeatherPreset)
{
   // Destroy the old preview actor before replacing it
   if (_weatherPreview != nullptr)
   {
      DestroyTransientActor(_weatherPreview);
      _weatherPreview = nullptr;
      _weatherPreviewClass = nullptr;
   }

   if (newWeatherPreset == nullptr)
   {
      return;
   }

   _weatherPreview = CastChecked<ATATWeatherPreset>(SpawnTransientSceneActor(newWeatherPreset, TEXT("PREVIEW_WeatherPreset")));
   check(_weatherPreview != nullptr);
   _weatherPreviewClass = newWeatherPreset;

   // If we don't have a valid scene depth texture, generate one now.
   // Even if it was already up to date, this will pass the scene depthmap params to the preset actor for init purposes
   constexpr bool forceUpdate = false;
   UpdateSceneDepthTexture(forceUpdate);
}

bool UTATWeatherEditorWorldExtension::UpdateSceneDepthTexture(bool forceUpdate)
{
   // If we have a weather manager, make sure the scene depthmap is up to date
   if (ATATWeatherManager* weatherManager = _GetWeatherManager(GetWorld()))
   {
      // If we don't have a valid scene depth texture, generate one now.
      // Even if it was already up to date, this will pass the scene depthmap params to the preset actor for init purposes
      return weatherManager->UpdateSceneDepthTextureInternal(forceUpdate, _weatherPreview);
   }
   return false;
}

void UTATWeatherEditorWorldExtension::EnteredSimulateInEditor()
{
   Super::EnteredSimulateInEditor();

   // Get rid of the weather preview when entering simulate/PIE
   if (_weatherPreview != nullptr)
   {
      DestroyTransientActor(_weatherPreview);
      _weatherPreview = nullptr;
   }
}

void UTATWeatherEditorWorldExtension::LeftSimulateInEditor(UWorld* simulateWorld)
{
   Super::LeftSimulateInEditor(simulateWorld);

   // If we got rid of the weather preview for simulate/PIE, restore it now
   if (_weatherPreview == nullptr && _weatherPreviewClass != nullptr)
   {
      SetWeatherPreset(_weatherPreviewClass);
   }
}

ATATWeatherManager* UTATWeatherEditorWorldExtension::_GetWeatherManager(UWorld* world) const
{
   if (world == nullptr)
   {
      return nullptr;
   }

   // Always check world settings first
   ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   if (worldSettings != nullptr && worldSettings->WeatherManager != nullptr)
   {
      return worldSettings->WeatherManager;
   }

   // Try to still work properly even if world settings isn't aware of the weather manager.
   // Ideally we could cache this instead of searching every time, but it's probably best to expect this be moved/deleted/etc at any random time in the editor.
   return WeatherEditorUtilities::FindFirstActorOfTypeInPersistentLevel<ATATWeatherManager>(world);
}

void UTATWeatherEditorWorldExtension::_OnSceneDepthTextureUpdatedInEditor(UTexture* depthmapTexture, const FBox& sceneDepthBounds, bool textureUpdated)
{
   // Reapply material and niagara depthmap params every time the depthmap is updated to keep it up to date in the editor
   UMaterialParameterCollection* mpc = UTATWeatherSettings::Get().MaterialParameterCollection.LoadSynchronous();
   UNiagaraParameterCollection* npc = UTATWeatherSettings::Get().NiagaraParameterCollection.LoadSynchronous();
   if (mpc || npc)
   {
      float worldSize = 0.0f;
      float worldHeight = 0.0f;
      FVector origin = FVector::ZeroVector;
      ATATWeatherManager::CalcDepthmapParameters(sceneDepthBounds, worldSize, worldHeight, origin);
      ensure(worldSize > 0 && worldHeight > 0);
      UTATWeatherUtilities::SetWeatherDepthmapParameters(GetWorld(), mpc, npc, depthmapTexture, worldSize, worldHeight, origin);
   }
}
