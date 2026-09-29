// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Developer/TATDataTableMap.h"
#include "Thiefsign/TATThiefsignTypes.h"

// ue
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TATThiefsignSettings.generated.h"

class ATATCharacterBase;
class UDataTable;

struct FStreamableHandle;

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignCharacterConfig
{
   GENERATED_BODY()

   // Bone/Socket name where the first person hand thiefsign animation symbol will be spawned.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FName SymbolSpawnSocket1P = NAME_None;

   // Vertical offset from SymbolSpawnSocket1P to display thiefsign symbol at for the local player.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float SymbolSpawnSocket1PZOffset = 0.0f;

   // World-space size of the thiefsign symbol from the perspective of the local player.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector2D Symbol1PSize = FVector2D::ZeroVector;

   // Bone/Socket name where the third person hand thiefsign animation symbol will be spawned.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FName SymbolSpawnSocket3P = NAME_None;

   // Vertical offset from SymbolSpawnSocket3P to display thiefsign symbol at for observing players.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float SymbolSpawnSocket3PZOffset = 0.0f;

   // World-space size of the thiefsign symbol from the perspective of observing player.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector2D Symbol3PSize = FVector2D::ZeroVector;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Thiefsign Settings"))
class TAT_API UTATThiefsignSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // For Blueprint
   UFUNCTION(BlueprintPure, Category = "TAT|Thiefsign")
   static UTATThiefsignSettings* GetThiefsignSettings() { return GetMutableDefault<UTATThiefsignSettings>(); }

   // For C++
   static const UTATThiefsignSettings& Get() { return *GetDefault<UTATThiefsignSettings>(); }
   static UTATThiefsignSettings& GetMutable() { return *GetMutableDefault<UTATThiefsignSettings>(); }

   // Returns true of SymbolsTable has been loaded into memory
   // If startLoadIfUnloaded is true, start an async load if SymbolsTable is not yet loaded
   bool AreSymbolsLoaded(bool startLoadIfUnloaded);

   // Adds a delegate callback which will trigger once SymbolsTable has finished async loading
   void CallOrRegisterSymbolsLoadedDelegate(const FSimpleMulticastDelegate::FDelegate& loadedDelegate);

   // Removes all bound delegates for a given object
   void UnregisterSymbolsLoadedDelegates(const UObject* contextObject);

   // Retrieves Thiefsign info based on the provided identifier tag, if SymbolsTable has loaded
   const FTATThiefsignInfo* FindThiefsignInfo(FGameplayTag thiefsignIdentifier);

   // The data table from which all Thiefsign symbols are loaded
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Thiefsign", meta = (RequiredAssetDataTags = "RowStructure=/Script/TAT.TATThiefsignInfo"))
   TSoftObjectPtr<UDataTable> SymbolsTable;

   // Default thiefsign character configuration, used if character is not specified in CharacterConfigs override.
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign")
   FTATThiefsignCharacterConfig DefaultCharacterConfig;

   // Contains data pertaining to thiefsign ability of individual characters.
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign")
   TMap<TSoftClassPtr<ATATCharacterBase>, FTATThiefsignCharacterConfig> CharacterConfigs;

   /// The gameplay cue tag for the VFX that should play when a player uses ther hand-UX thiefsign
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Hand-UX", meta = (Categories = "GameplayCue.Thiefsign"))
   FGameplayTag HandGameplayCueTag = FGameplayTag::EmptyTag;

   // Tag identifier for hand-UX animation contained in character's TATCharacterAnimationMapping
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Hand-UX", meta = (Categories = "Animation.Character"))
   FGameplayTag HandAnimationTag = FGameplayTag::EmptyTag;

   /// The gameplay cue tag for the VFX that should play when a player uses ther decal-UX thiefsign
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX", meta = (Categories = "GameplayCue.Thiefsign"))
   FGameplayTag DecalGameplayCueTag = FGameplayTag::EmptyTag;

   // Tag identifier for decal-UX animation contained in character's TATCharacterAnimationMapping
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX", meta = (Categories = "Animation.Character.Thiefsign"))
   FGameplayTag DecalAnimationTag = FGameplayTag::EmptyTag;

   // How long to keep hand-UX thiefsign VFX alive for
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Hand-UX")
   float HandSymbolLifespan = 0.0f;

   // How far away a decal-able surface must be from a player's look direction to use decal-UX instead of hand-UX thiefsign
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX")
   float DecalMaxSurfaceDistance = 0.0f;

   // The depth of the bounding box of spawned decal-UX Thiefsign actors, should be smaller to prevent streaking if decal is on edge of surface
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX")
   float DecalBoundingBoxDepth = 16.0f;

   // What profile should decal-UX thiefsign use for traces to determine if surface is decal-able
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX")
   FCollisionProfileName DecalTraceCollisionProfile;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Hand-UX")
   FTATThiefsignVFXConfig HandVFXConfig;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "TAT|Thiefsign|Decal-UX")
   FTATThiefsignVFXConfig DecalVFXConfig = FTATThiefsignVFXConfig(ETATThiefsignVFXConfigShowType::HideNiagara);

   // Returns thiefsign config for specified character.
   UFUNCTION(BlueprintCallable)
   const FTATThiefsignCharacterConfig& FindCharacterConfig(TSubclassOf<ATATCharacterBase> characterClass) const;

private:
   // Triggers all callback functions in _symbolsLoadListeners when SymbolsTable has finished loading
   void _OnTableLoaded();

   TSharedPtr<FStreamableHandle> _loadingHandle;

   FSimpleMulticastDelegate _onSymbolsLoadedDelegate;

   // Lookup map to make finding thiefsign data table entries by gameplay tag fast
   TTATDataTableMap<FTATThiefsignInfo> _thiefsignDataTableMap;
};
