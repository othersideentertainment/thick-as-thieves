// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"

#include "TATLockpickingTypes.generated.h"

class FDataValidationContext;

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickTrackSection - Data pertaining to sections on a spawned instance of a lockpicking minigame track
//------------------------------------------------------------------------------------------------------------------------
struct FTATLockpickTrackSection
{
   // Controls how quickly the current track moves when the lockpick is in this section.
   float TrackSpeed = 1.0f;

   // Start time of this section on its track.
   float StartTimeSeconds = 0.0f;

   // End time of this section on its track.
   float EndTimeSeconds = 0.0f;

   // Duration of that section that is created when this section is interacted with.
   float InteractedSectionDurationSeconds = 0.0f;

   // Determines the type of this track section.
   FGameplayTag SectionType = FGameplayTag::EmptyTag;

   // Whether this section has been interacted with
   bool InteractedWith = false;
};

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickTrackLayer - A collection of related track sections, can be created from a track definition or from a player's interactions with a particular definition
//------------------------------------------------------------------------------------------------------------------------
struct FTATLockpickTrackLayer
{
   // Clear out all sections
   void Clear();

   // A Definition layer is all the sections that make up an un-interacted lockpick track
   static FTATLockpickTrackLayer CreateDefinitionLayer(const FTATLockpickMinigameTrackDefinition& trackDefinition);

   FORCEINLINE int32 GetSectionCount() const { return _sections.Num(); }
   FORCEINLINE bool HasSections() const { return GetSectionCount() != 0; }
   FORCEINLINE float GetEndTime() const { return HasSections() ? _sections.Last().EndTimeSeconds : -1.0f; }

   FORCEINLINE bool IsValidIndex(int32 index) const { return _sections.IsValidIndex(index); }

   const FTATLockpickTrackSection& operator[](int32 index) const;
   void MarkInteracted(int32 index);

private:
   TArray<FTATLockpickTrackSection> _sections;

   friend struct FTATLockpickTrackLayerManager;
};

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickTrackLayerManager - A combination of a Definition layer and Interaction layer, which together make up all the sections of a track
//------------------------------------------------------------------------------------------------------------------------
struct FTATLockpickTrackLayerManager
{
   // Clear out all layers and reset cached data
   void Clear();

   // Creates a Definition layer from the supplied 'trackDefinition' as well as an (initially empty) Interaction layer
   void Build(const FTATLockpickMinigameTrackDefinition& trackDefinition);

   // Adds an interaction time to the Interaction layer
   void AddInteractionTime(float interactionTime);

   // Pass the current track time so we can update our internal section indices;
   void UpdateTrackTime(float time);

   FORCEINLINE const FTATLockpickTrackSection* GetCurrentSection() const { return _currentSection; }
   FORCEINLINE bool IsCurrentSectionValid() const { return _currentSection != nullptr; }
   FORCEINLINE bool IsAtEnd() const { return !IsCurrentSectionValid(); }
   FORCEINLINE float GetDurationSeconds() const { return _durationSeconds; }
   FORCEINLINE int32 GetSectionCount() const { return _sectionCount; }
   FORCEINLINE bool IsValid() const { return GetSectionCount() > 0; }

   bool AreSectionsInteractedWith(FGameplayTag sectionType) const;
   bool IsAnySectionInteractedWith(FGameplayTag sectionType) const;
   bool HasSectionType(FGameplayTag sectionType) const;

   // Iterates over all layers of a given track, splitting sections in lower layers when those in higher layers overlap them
   struct FTATConstSectionIterator
   {
      FTATConstSectionIterator(const FTATLockpickTrackLayerManager& container)
         : _container(container)
      {
         _UpdateCurrentIndex();
      }

      FTATConstSectionIterator& operator++();

      const FTATLockpickTrackSection& operator* () const;

      FORCEINLINE explicit operator bool() const
      {
         return _currentSection != nullptr;
      }

      float GetCurrentSectionStartTime() const;
      float GetNextSectionStartTime() const;

   private:
      const FTATLockpickTrackLayerManager& _container;

      int32 _definitionSectionIndex = 0;
      int32 _interactionSectionIndex = 0;

      const FTATLockpickTrackSection* _currentSection = nullptr;
      float _currentSectionStartTime = 0.0f;

      void _UpdateCurrentIndex();
   };
   FTATConstSectionIterator CreateConstIterator() const { return FTATConstSectionIterator(*this); }

private:
   // Layer containing all the sections from the track section, pre-defined in the minigame variation struct
   FTATLockpickTrackLayer _definitionLayer;

   // The index of the currently tracked definition section which is active at the time matching '_currentTime'
   int32 _definitionSectionIndex = INDEX_NONE;

   // Layer containing all the sections created from times the player interacted with sections on the Definition layer
   FTATLockpickTrackLayer _interactionLayer;

   // The index of the currently tracked interaction section which is active at the time matching '_currentTime'
   int32 _interactionSectionIndex = INDEX_NONE;

   // The currently active section, pulled from either the Interaction layer (which has priority) or the Definition layer
   const FTATLockpickTrackSection* _currentSection = nullptr;

   // The currently tracked progress time of this track, helping to find '_currentSection'
   float _currentTime = 0.0f;

   // The full duration of this track
   float _durationSeconds = -1.0f;

   // The total number of sections between the Definition and Interaction layers
   int32 _sectionCount = 0;

   void _UpdateCachedData(bool reset);
};

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickTrackSectionDefinition - Stores and manages data regarding a section of a track on a lockpicking variation instance
//------------------------------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FTATLockpickTrackSectionDefinition
{
   GENERATED_BODY()

   FTATLockpickTrackSectionDefinition();

   // Gameplay tag defining the type of this section, determining its visuals and how it is interacted with 
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag SectionType = FGameplayTag::EmptyTag;

   // Corresponds to a time where the pressure point range begins along its track
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
   float StartTimeSeconds = 0.f;

   // Corresponds to a time where the pressure point range end along its track
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
   float EndTimeSeconds = 0.f;

   // If true, use 'InteractedSectionDurationSeconds'. Otherwise, pull the interacted section duration from LockpickingSettings
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (InlineEditConditionToggle))
   bool OverrideInteractedSectionDurationSeconds = false;

   // The duration of sections created when this section is interacted with
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0", EditCondition = "OverrideInteractedSectionDurationSeconds", DisplayName = "Override Interacted Effect Duration Seconds"))
   float InteractedSectionDurationSeconds = 0.0f;
};

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickMinigameTrackDefinition - Stores and manages data regarding a track of a lockpicking variation instance
//------------------------------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLockpickMinigameTrackDefinition
{
   GENERATED_BODY()

   FTATLockpickMinigameTrackDefinition();

   // Controls how long it takes the cursor to move along the track (without the effects of interactions)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
   float DurationSeconds = 0.f;

   // How far should the visualization for this track extend in a ring?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0", UIMax = "360", ClampMax = "360"))
   float TrackArcAngle = 270.0f;

   // Any time between 0.0f and 'DurationSeconds' that is not covered by 'DefinedSections' will have sections created with this type
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag DefaultSectionType = FGameplayTag::EmptyTag;

   // User-defined sections on a track
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TArray<FTATLockpickTrackSectionDefinition> DefinedSections;
};

//------------------------------------------------------------------------------------------------------------------------
// FTATLockpickMinigameVariation - Stores and manages all data regarding a specific lockpicking variation instance
//------------------------------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLockpickMinigameVariation
{
   GENERATED_BODY()

   // Validates data
   bool HasValidData() const;
   bool HasValidData(FDataValidationContext& context) const;

   // How many tracks are contained in this lockpick minigame variation?
   int32 GetTrackCount() const;

   // Sets 'index' to INDEX_NONE if it does not exist in this variation
   void ValidateTrackIndex(int32& index) const;

   // Retrieves the maximum arc of a given track.
   float GetTrackArcAngle(int32 trackIndex) const;

   // Retrieves the user-entered data pertaining to the layout of a lockpick track.
   const FTATLockpickMinigameTrackDefinition& GetTrackDefinition(int32 trackIndex) const;

   // Corresponds to a lock's LockLevel (see FTATLockConfig)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0", UIMax = "6", ClampMax = "6"))
   int DifficultyLevel = 1;

   // Data defining the number of tracks and the pressure points found on each of them, ordered outer-most to inner
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TArray<FTATLockpickMinigameTrackDefinition> Tracks;
};
//------------------------------------------------------------------------------------------------------------------------
// UTATLockpickMinigameVariationDataAsset - Stores all lockpicking variation instances
//------------------------------------------------------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class TAT_API UTATLockpickMinigameVariationDataAsset : public UDataAsset
{
   GENERATED_BODY()
         
public:
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty="Difficulty {DifficultyLevel}"))
   TArray<FTATLockpickMinigameVariation> Variations;

   UFUNCTION(BlueprintPure, Category = "TAT|Lockpicking")
   bool GetLockpickMinigameVariationForComplexityLevel(int32 complexityLevel, FTATLockpickMinigameVariation& lockpickMinigameVariation) const;
};
