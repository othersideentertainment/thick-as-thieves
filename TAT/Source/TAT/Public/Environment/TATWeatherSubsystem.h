// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat 
#include "Settings/TATMatchSettingsBase.h"

// ue
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "NativeGameplayTags.h"

#include "TATWeatherSubsystem.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Indoors);

// private struct for use by UTATWeatherSubsystem to store polling state for actors registered with the subsystem
USTRUCT()
struct TAT_API FTATWeatherActorPollingState 
{
   GENERATED_BODY()

   TWeakObjectPtr<AActor> TrackedActor { nullptr };
   float LastUpdateTime { .0f };
   float NextUpdateInterval { 0.5f };
   bool IsInside { false };
};

///
UCLASS()
class TAT_API UTATWeatherSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

public:
   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   // From UWorldSubsystem
   virtual void OnWorldBeginPlay(UWorld& InWorld) override;
   // From UTickableWorldSubsystem
   virtual bool IsTickable() const override;
   

   // From UObject
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATWeatherSubsystem, STATGROUP_Tickables); }

   /// Registers an actor in the weather subsystem.
   /// If enablePolling is true, the actor is checked periodically to keep their state up to date (for actors that move around).
   /// If enablePolling is false, the actor is checked exactly once at registration time (for actors that do not move).
   /// Once registered, you can use FindWeatherTagsForActor to find weather-related state tags for as long as the actor is registered.
   void RegisterActor(AActor* actor, bool enablePolling);

   /// Unregisters an actor from the weather subsystem, removing any existing state data.
   void UnregisterActor(AActor* actor);

private:
   UFUNCTION()
   void _OnMatchSettingsUpdated(UTATMatchSettingsBase* newMatchSettings);
   void _HandleWeatherMetaTagsChanged();
   
   static void _ChangeTagsOnActor(AActor* actor, const FGameplayTagContainer& container, bool add);
   static void _ChangeTagsOnActor(UAbilitySystemComponent* abilitySystemComponent,
                                 const FGameplayTagContainer& container,
                                 bool add);
   void _UpdateActorState(AActor* actor, bool isInside) const;

   float _GetRandomPollingIntervalSeconds() const { return FMath::FRandRange(_pollIntervalRangeSeconds.Min, _pollIntervalRangeSeconds.Max); }

   float _lastTickWorldTimeSeconds = 0.0f;

   // Cached off values from the weather settings.
   FGameplayTag _currentWeatherType;
   FGameplayTag _visibilityIndoorsTag;
   FGameplayTag _visibilityOutdoorsTag;
   FGameplayTag _soundCarryIndoorsTag;
   FGameplayTag _soundCarryOutdoorsTag;

   FGameplayTagContainer _indoorTags;
   FGameplayTagContainer _outdoorTags;


   FFloatInterval _pollIntervalRangeSeconds = FFloatInterval(.1f, .2f);
   float _targetTickRateSeconds = 0.125f;
   int32 _maxActorsToPollPerTick = 50;

   UPROPERTY(Transient)
   TArray<FTATWeatherActorPollingState> _polledStates;
   int _indexToTickIfBreaking { INDEX_NONE };
   float _maxFrameTimeUntilCutoff { 0.1f };
};
