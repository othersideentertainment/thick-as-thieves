// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Lockpicking/TATLockpickingTypes.h"

// tat
#include "Lockpicking/TATLockpickingSettings.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockpickingTypes)
DEFINE_LOG_CATEGORY_STATIC(LogTATLockpickingTypes, Log, All);

void FTATLockpickTrackLayer::Clear()
{
   _sections.Empty();
}

FTATLockpickTrackLayer FTATLockpickTrackLayer::CreateDefinitionLayer(const FTATLockpickMinigameTrackDefinition& trackDefinition)
{
   FTATLockpickTrackLayer layer;

   // Sort the 'DefinedSections' for this track to ensure we can add sections in sequential order to the stack
   TArray<FTATLockpickTrackSectionDefinition> sortedSections = trackDefinition.DefinedSections;
   sortedSections.Sort([](const FTATLockpickTrackSectionDefinition& a, const FTATLockpickTrackSectionDefinition& b)
   {
      return a.StartTimeSeconds < b.StartTimeSeconds;
   });

   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
   const FTATLockpickTrackSectionTypeConfig& defaultSectionConfig = lockpickSettings.TrackSectionConfigs.FindChecked(trackDefinition.DefaultSectionType);

   // There should be a default section for every non-default, plus one default section at the very end
   layer._sections.Reserve(sortedSections.Num() * 2 + 1);

   float currentIterationTime = 0.0f;
   for (const FTATLockpickTrackSectionDefinition& section : sortedSections)
   {
      // Check to make sure there's no Defined section at the very beginning, and if there's not created a Default section
      if (section.StartTimeSeconds > currentIterationTime)
      {
         FTATLockpickTrackSection& defaultSection = layer._sections.Emplace_GetRef();
         defaultSection.SectionType = trackDefinition.DefaultSectionType;
         defaultSection.TrackSpeed = defaultSectionConfig.TrackSpeed;
         defaultSection.InteractedSectionDurationSeconds = defaultSectionConfig.InteractedSectionDurationSeconds;
         defaultSection.StartTimeSeconds = currentIterationTime;
         defaultSection.EndTimeSeconds = section.StartTimeSeconds;
      }

      const FTATLockpickTrackSectionTypeConfig& definedSectionConfig = lockpickSettings.TrackSectionConfigs.FindChecked(section.SectionType);

      FTATLockpickTrackSection& definedSection = layer._sections.Emplace_GetRef();
      definedSection.SectionType = section.SectionType;
      definedSection.TrackSpeed = definedSectionConfig.TrackSpeed;
      definedSection.StartTimeSeconds = section.StartTimeSeconds;
      definedSection.EndTimeSeconds = section.EndTimeSeconds;

      if (section.OverrideInteractedSectionDurationSeconds)
      {
         definedSection.InteractedSectionDurationSeconds = section.InteractedSectionDurationSeconds;
      }
      else
      {
         definedSection.InteractedSectionDurationSeconds = definedSectionConfig.InteractedSectionDurationSeconds;
      }

      currentIterationTime = section.EndTimeSeconds;
   }

   // If we haven't covered up until the end of this track, add one last Default section to fill it in
   if (currentIterationTime < trackDefinition.DurationSeconds)
   {
      FTATLockpickTrackSection& defaultSection = layer._sections.Emplace_GetRef();
      defaultSection.SectionType = trackDefinition.DefaultSectionType;
      defaultSection.TrackSpeed = defaultSectionConfig.TrackSpeed;
      defaultSection.InteractedSectionDurationSeconds = defaultSectionConfig.InteractedSectionDurationSeconds;
      defaultSection.StartTimeSeconds = currentIterationTime;
      defaultSection.EndTimeSeconds = trackDefinition.DurationSeconds;
   }

   return layer;
}

const FTATLockpickTrackSection& FTATLockpickTrackLayer::operator[](int32 index) const
{
   check(_sections.IsValidIndex(index));
   return _sections[index];
}

void FTATLockpickTrackLayer::MarkInteracted(int32 index)
{
   check(_sections.IsValidIndex(index));
   _sections[index].InteractedWith = true;
}

void FTATLockpickTrackLayerManager::Clear()
{
   _definitionLayer.Clear();
   _interactionLayer.Clear();
   _UpdateCachedData(true);
}

void FTATLockpickTrackLayerManager::Build(const FTATLockpickMinigameTrackDefinition& trackDefinition)
{
   // Create the Definition layer
   _definitionLayer = FTATLockpickTrackLayer::CreateDefinitionLayer(trackDefinition);
   _definitionSectionIndex = _definitionLayer.HasSections() ? 0 : INDEX_NONE;

   // Create the Interaction layer, which will start out empty
   _interactionLayer = FTATLockpickTrackLayer();
   _interactionSectionIndex = INDEX_NONE;

   _UpdateCachedData(true);
}

void FTATLockpickTrackLayerManager::AddInteractionTime(float interactionTime)
{
   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();

   // Find the Definition section this interaction lies on
   int32 definitionIndex = 0;
   while (_definitionLayer.IsValidIndex(definitionIndex) 
      && interactionTime >= _definitionLayer[definitionIndex].EndTimeSeconds)
   {
      definitionIndex++;
   }

   // Build an Interaction section based on the Definition section's interaction effect
   if (_definitionLayer.IsValidIndex(definitionIndex))
   {
      _definitionLayer.MarkInteracted(definitionIndex);
      const FTATLockpickTrackSection& definitionSection = _definitionLayer[definitionIndex];
      const FTATLockpickTrackSectionTypeConfig& sectionConfig = lockpickSettings.TrackSectionConfigs.FindChecked(definitionSection.SectionType);
      const FTATLockpickTrackSectionTypeConfig& interactedConfig = lockpickSettings.TrackSectionConfigs.FindChecked(sectionConfig.InteractedSectionType);

      FTATLockpickTrackSection& interactedSection = _interactionLayer._sections.Emplace_GetRef();
      interactedSection.SectionType = sectionConfig.InteractedSectionType;
      interactedSection.TrackSpeed = interactedConfig.TrackSpeed;
      interactedSection.InteractedSectionDurationSeconds = interactedConfig.InteractedSectionDurationSeconds;
      interactedSection.StartTimeSeconds = interactionTime;
      interactedSection.EndTimeSeconds = interactedSection.StartTimeSeconds + interactedSection.TrackSpeed * sectionConfig.InteractedSectionDurationSeconds;

      // Ensure that the end time doesn't pass the end time for this whole track
      interactedSection.EndTimeSeconds = FMath::Min(interactedSection.EndTimeSeconds, _durationSeconds);
   }

   if (_interactionSectionIndex == INDEX_NONE)
   {
      _interactionSectionIndex = 0;
   }

   _UpdateCachedData(false);
}

void FTATLockpickTrackLayerManager::UpdateTrackTime(float time)
{
   _currentTime = time;

   // Increment '_interactionSectionIndex' until we find a section that hasn't been passed
   while (_interactionLayer.IsValidIndex(_interactionSectionIndex) 
      && time >= _interactionLayer[_interactionSectionIndex].EndTimeSeconds)
   {
      _interactionSectionIndex++;
   }
   if (_interactionLayer.IsValidIndex(_interactionSectionIndex))
   {
      // If the start time for this section has been passed, its our current section
      if (time >= _interactionLayer[_interactionSectionIndex].StartTimeSeconds)
      {
         _currentSection = &_interactionLayer[_interactionSectionIndex];

         // Exit early with '_currentSection' as Interaction sections take priority over Definition sections
         return;
      }
   }
   else
   {
      _interactionSectionIndex = INDEX_NONE;
   }

   // Increment '_definitionSectionIndex' until we find a section that hasn't been passed
   while (_definitionLayer.IsValidIndex(_definitionSectionIndex)
      && time >= _definitionLayer[_definitionSectionIndex].EndTimeSeconds)
   {
      _definitionSectionIndex++;
   }
   if (_definitionLayer.IsValidIndex(_definitionSectionIndex))
   {
      // If the start time for this section has been passed, its our current section
      if (time >= _definitionLayer[_definitionSectionIndex].StartTimeSeconds)
      {
         _currentSection = &_definitionLayer[_definitionSectionIndex];
      }
   }
   else
   {
      // If an Interaction section wasn't found (would have return'ed if so)
      // And a Definition section wasn't found, we must be at the end of the track
      _definitionSectionIndex = INDEX_NONE;
      _currentSection = nullptr;
   }
}

bool FTATLockpickTrackLayerManager::AreSectionsInteractedWith(FGameplayTag sectionType) const
{
   for (int i = 0; i < _definitionLayer.GetSectionCount(); ++i)
   {
      const FTATLockpickTrackSection& section = _definitionLayer[i];
      if (section.SectionType == sectionType && !section.InteractedWith)
      {
         return false;
      }
   }

   return true;
}

bool FTATLockpickTrackLayerManager::IsAnySectionInteractedWith(FGameplayTag sectionType) const
{
   for (int i = 0; i < _definitionLayer.GetSectionCount(); ++i)
   {
      const FTATLockpickTrackSection& section = _definitionLayer[i];
      if (section.SectionType == sectionType && section.InteractedWith)
      {
         return true;
      }
   }

   return false;
}

bool FTATLockpickTrackLayerManager::HasSectionType(FGameplayTag sectionType) const
{
   for (int i = 0; i < _definitionLayer.GetSectionCount(); ++i)
   {
      const FTATLockpickTrackSection& section = _definitionLayer[i];
      if (section.SectionType == sectionType)
      {
         return true;
      }
   }

   return false;
}

void FTATLockpickTrackLayerManager::_UpdateCachedData(bool reset)
{
   _durationSeconds = _definitionLayer.GetEndTime();
   _sectionCount = _definitionLayer.GetSectionCount() + _interactionLayer.GetSectionCount();

   if (reset)
   {
      _currentTime = 0.0f;
      _currentSection = nullptr;
   }

   UpdateTrackTime(_currentTime);
}

FTATLockpickTrackLayerManager::FTATConstSectionIterator& FTATLockpickTrackLayerManager::FTATConstSectionIterator::operator++()
{
   _currentSectionStartTime = GetNextSectionStartTime();

   const FTATLockpickTrackLayer& interactionLayer = _container._interactionLayer;
   if (interactionLayer.IsValidIndex(_interactionSectionIndex))
   {
      // Increment '_interactionSectionIndex' if we've passed the end time of the current index
      // Multiple sections may have been passed, increment until one is found that hasn't
      while (_currentSectionStartTime >= interactionLayer[_interactionSectionIndex].EndTimeSeconds)
      {
         _interactionSectionIndex++;
         if (!interactionLayer.IsValidIndex(_interactionSectionIndex))
         {
            _interactionSectionIndex = INDEX_NONE;
            break;
         }
      }
   }

   const FTATLockpickTrackLayer& definitionLayer = _container._definitionLayer;
   if (definitionLayer.IsValidIndex(_definitionSectionIndex))
   {
      // Increment '_definitionSectionIndex' if we've passed the end time of the current index
      // Multiple sections may have been passed, increment until one is found that hasn't
      while (_currentSectionStartTime >= definitionLayer[_definitionSectionIndex].EndTimeSeconds)
      {
         _definitionSectionIndex++;
         if (!definitionLayer.IsValidIndex(_definitionSectionIndex))
         {
            _definitionSectionIndex = INDEX_NONE;
            break;
         }
      }
   }

   // Update the value of '_currentSection' based on how our indices might have changed
   _UpdateCurrentIndex();

   return *this;
}

const FTATLockpickTrackSection& FTATLockpickTrackLayerManager::FTATConstSectionIterator::operator* () const
{
   check(_currentSection);
   return *_currentSection;
}

void FTATLockpickTrackLayerManager::FTATConstSectionIterator::_UpdateCurrentIndex()
{
   _currentSection = nullptr;

   const FTATLockpickTrackLayer& interactionLayer = _container._interactionLayer;
   if (interactionLayer.IsValidIndex(_interactionSectionIndex))
   {
      // If the start time for this section has been passed, its our current section
      const FTATLockpickTrackSection& section = interactionLayer[_interactionSectionIndex];
      if (_currentSectionStartTime >= section.StartTimeSeconds)
      {
         _currentSection = &section;
         return;
      }
   }

   const FTATLockpickTrackLayer& definitionLayer = _container._definitionLayer;
   if (definitionLayer.IsValidIndex(_definitionSectionIndex))
   {
      // If the start time for this section has been passed, its our current section
      const FTATLockpickTrackSection& section = definitionLayer[_definitionSectionIndex];
      if (_currentSectionStartTime >= section.StartTimeSeconds)
      {
         _currentSection = &section;
      }
   }
}

float FTATLockpickTrackLayerManager::FTATConstSectionIterator::GetCurrentSectionStartTime() const
{
   return _currentSectionStartTime;
}

float FTATLockpickTrackLayerManager::FTATConstSectionIterator::GetNextSectionStartTime() const
{
   // By default, the next section start time is the end time of our current section. But, it is possible that an
   // Interaction section bisects a Definition section, in which case we use the start time of that Interaction section

   check(_currentSection);
   float endTime = _currentSection->EndTimeSeconds;

   const FTATLockpickTrackLayer& interactionLayer = _container._interactionLayer;
   if (interactionLayer.IsValidIndex(_interactionSectionIndex))
   {
      const FTATLockpickTrackSection& section = interactionLayer[_interactionSectionIndex];
      if (section.StartTimeSeconds > _currentSectionStartTime)
      {
         endTime = FMath::Min(endTime, section.StartTimeSeconds);
      }
   }

   return endTime;
}

FTATLockpickTrackSectionDefinition::FTATLockpickTrackSectionDefinition()
{
   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
   SectionType = lockpickSettings.InitialDefinedTrackSectionType;
}

FTATLockpickMinigameTrackDefinition::FTATLockpickMinigameTrackDefinition()
{
   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
   DefaultSectionType = lockpickSettings.DefaultTrackSectionType;
}

bool FTATLockpickMinigameVariation::HasValidData() const
{
   FDataValidationContext context;
   return HasValidData(context);
}

bool FTATLockpickMinigameVariation::HasValidData(FDataValidationContext& context) const
{
   const int32 initialIssueCount = context.GetIssues().Num();
   if (Tracks.IsEmpty())
   {
      context.AddError(FText::FromString(TEXT("Lockpick minigame variation detected track with non-positive duration!")));
   }
   else
   {
      const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();

      for (TArray<FTATLockpickMinigameTrackDefinition>::TConstIterator itTrack = Tracks.CreateConstIterator(); itTrack; ++itTrack)
      {
         if (itTrack->DurationSeconds <= 0.f)
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected track %d with non-positive duration!"),
               itTrack.GetIndex())));
         }
         
         if (!lockpickSettings.TrackSectionConfigs.Contains(itTrack->DefaultSectionType))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected track %d with default section type %s not defined in LockpickSettings!"),
               itTrack.GetIndex(),
               *itTrack->DefaultSectionType.ToString())));
         }

         for (TArray<FTATLockpickTrackSectionDefinition>::TConstIterator itSection = itTrack->DefinedSections.CreateConstIterator(); itSection; ++itSection)
         {
            if (!lockpickSettings.TrackSectionConfigs.Contains(itSection->SectionType))
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected section %d on track %d with section type %s not defined in LockpickSettings!"),
                  itSection.GetIndex(),
                  itTrack.GetIndex(),
                  *itTrack->DefaultSectionType.ToString())));
            }
            
            if (itSection->StartTimeSeconds >= itSection->EndTimeSeconds)
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected section %d on track %d with a start time after its end time!"),
                  itSection.GetIndex(),
                  itTrack.GetIndex())));
            }
            
            if (itSection->StartTimeSeconds < 0.f || itSection->StartTimeSeconds >= itTrack->DurationSeconds)
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected section %d on track %d with a start time outside the bounds of the track duration!"),
                  itSection.GetIndex(),
                  itTrack.GetIndex())));
            }
            
            if (itSection->EndTimeSeconds > itTrack->DurationSeconds)
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("Lockpick minigame variation detected section %d on track %d with an end time outside the bounds of the track duration!"),
                  itSection.GetIndex(),
                  itTrack.GetIndex())));
            }
         }
      }
   }
   return (initialIssueCount == context.GetIssues().Num());
}

int32 FTATLockpickMinigameVariation::GetTrackCount() const
{
   return Tracks.Num();
}

void FTATLockpickMinigameVariation::ValidateTrackIndex(int32& index) const
{
   if (!Tracks.IsValidIndex(index))
   {
      index = INDEX_NONE;
   }
}

float FTATLockpickMinigameVariation::GetTrackArcAngle(int32 trackIndex) const
{
   if (Tracks.IsValidIndex(trackIndex))
   {
      return Tracks[trackIndex].TrackArcAngle;
   }
   else
   {
      // log error
      return 0.0f;
   }
}

const FTATLockpickMinigameTrackDefinition& FTATLockpickMinigameVariation::GetTrackDefinition(int32 trackIndex) const
{
   check(Tracks.IsValidIndex(trackIndex));
   return Tracks[trackIndex];
}

#if WITH_EDITOR
EDataValidationResult UTATLockpickMinigameVariationDataAsset::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   for (int i = 0; i < Variations.Num(); i++)
   {
      const FTATLockpickMinigameVariation& minigameVariation = Variations[i];

      // Check for invalid data
      if (!minigameVariation.HasValidData(context))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Variation entry %d has invalid data!"), i)));
      }
   }
   
   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

bool UTATLockpickMinigameVariationDataAsset::GetLockpickMinigameVariationForComplexityLevel(int32 complexityLevel, FTATLockpickMinigameVariation& lockpickMinigameVariation) const
{
   for (const FTATLockpickMinigameVariation& minigameVariation : Variations)
   {
      if (minigameVariation.DifficultyLevel == complexityLevel)
      {
         lockpickMinigameVariation = minigameVariation;
         return true;
      }
   }

   UE_LOG(LogTATLockpickingTypes, Error, TEXT("GetLockpickMinigameVariationForComplexityLevel() could not find variation for difficulty level %d!"), complexityLevel);
   return false;
}
