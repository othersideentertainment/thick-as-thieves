// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/TATAISense_Hearing.h"

// tat
#include "AI/Perception/TATAIPerceptionSystem.h"
#include "AI/Perception/TATAISenseConfig_Hearing.h"
#include "AI/Perception/TATAISenseEvent_Hearing.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/Perception/TATNoisePropagationSubsystem.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Developer/TATProjectSettings.h"
#include "Player/TATPlayerController.h"
#include "AI/TATAISettings.h"
#include "AI/Perception/TATHearingStimSourceReactor.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ose
#include "AI/Perception/OSEAISenseSharedConfigData.h"
#include "Character/OSETeamInterface.h"
#include "OSECommon.h"

// ue4 
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAISense_Hearing)

DEFINE_LOG_CATEGORY_STATIC(LogTATAISense_Hearing, Log, All);

namespace TATAINoiseCVars
{
   static int DrawStimDebug = 0;
   FAutoConsoleVariableRef CVarDebugDrawStimDebug(
      TEXT("TAT.Perception.DrawHearingStims"),
      DrawStimDebug,
      TEXT("Draw debug for audio stims"),
      ECVF_Default);
}
//----------------------------------------------------------------------
// FTATAINoiseEvent
//----------------------------------------------------------------------
FTATAINoiseEvent::FTATAINoiseEvent()
   : Age(0.0f)
   , NoiseLocation(FAISystem::InvalidLocation)
   , Loudness(1.0f) // in TAT we're going to start without a loudness multiplier and see if we need it, we'd like consistency
   , Instigator(nullptr)
   , Tag()
{
}

FTATAINoiseEvent::FTATAINoiseEvent(AActor* inInstigator, const FVector& inNoiseLocation, FGameplayTag inTag)
   : Age(0.f)
   , NoiseLocation(inNoiseLocation)
   , Loudness(1.0f) // in TAT we're going to start without a loudness multiplier and see if we need it, we'd like consistency
   , Instigator(inInstigator)
   , Tag(inTag)
{
   Compile();
}

void FTATAINoiseEvent::Compile()
{
   if (!FAISystem::IsValidLocation(NoiseLocation) && Instigator)
   {
      NoiseLocation = Instigator->GetActorLocation();
   }
}

//----------------------------------------------------------------------
// FDigestedHearingProperties
//----------------------------------------------------------------------
UTATAISense_Hearing::FDigestedHearingProperties::FDigestedHearingProperties(const UTATAISenseConfig_Hearing& senseConfig)
{
   AffiliationFlags = senseConfig.DetectionByAffiliation.GetAsFlags();
   SharedConfigData = senseConfig.SharedConfigData;
}

UTATAISense_Hearing::FDigestedHearingProperties::FDigestedHearingProperties() :
   AffiliationFlags(-1)
{

}

//----------------------------------------------------------------------
// UTATAISense_Hearing
//----------------------------------------------------------------------
UTATAISense_Hearing::UTATAISense_Hearing(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      OnNewListenerDelegate.BindUObject(this, &UTATAISense_Hearing::_OnNewListenerImpl);
      OnListenerUpdateDelegate.BindUObject(this, &UTATAISense_Hearing::_OnListenerUpdateImpl);
      OnListenerRemovedDelegate.BindUObject(this, &UTATAISense_Hearing::_OnListenerRemovedImpl);
   }
}

void UTATAISense_Hearing::PostInitProperties()
{
   Super::PostInitProperties();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      _RegisterMakeNoiseDelegate();
   }
}

void UTATAISense_Hearing::_RegisterMakeNoiseDelegate()
{
   // TAT TODO: Does this fight w/ the engine impl?  May not matter, I don't thik we use AActor::MakeNoise anywhere
   AActor::SetMakeNoiseDelegate(FMakeNoiseDelegate::CreateStatic(&UAIPerceptionSystem::MakeNoiseImpl));
}

void UTATAISense_Hearing::ReportNoiseEvent(UObject* worldContextObject,
                                           const FGameplayTag tag,
                                           const FVector noiseLocation,
                                           AActor* instigator,
                                           const bool suppressGlyphIndicator)
{
   ReportNoiseEventWithLoudness(worldContextObject, tag, noiseLocation, instigator, suppressGlyphIndicator, 1.f);
}

void UTATAISense_Hearing::ReportNoiseEventWithLoudness(
   UObject* worldContextObject,
   const FGameplayTag tag,
   const FVector noiseLocation,
   AActor* instigator,
   const bool suppressGlyphIndicator,
   const float loudness)
{
   if(FMath::IsNearlyZero(loudness))
   {
      UE_LOG(
         LogTATAISense_Hearing,
         Verbose,
         TEXT("[Noise Stim Event] Blocked Stim Tag: [%s], Location: [%s], Instigator: [%s] as Loudness is zero"),
         *tag.ToString(),
         *noiseLocation.ToString(),
         *AActor::GetDebugName(instigator)
      );
      return;
   }
   
   const UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull);
   // Spawn a glyph indicator if one is configured for this noise stim
   if (!suppressGlyphIndicator)
   {
      if (world != nullptr && !world->IsNetMode(NM_Client))
      {
         if (UTATThiefVisionSubsystem* thiefVisionSubsystem = world->GetSubsystem<UTATThiefVisionSubsystem>())
         {
            const FTATNoiseStimGlyphSettings* glyphSettings = UTATProjectSettings::Get().NoiseStimGlyphIndicatorSettings.Find(tag);
            if (glyphSettings != nullptr && glyphSettings->IsEnabledAndValid())
            {
               thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(
                  glyphSettings->IndicatorType,
                  FTransform(noiseLocation),
                  glyphSettings->DeduplicateDistance
               );
            }
         }
      }
   }

   FTATAINoiseEvent event(instigator, noiseLocation, tag);
   event.Loudness = loudness;
   if (UTATAIPerceptionSystem* perceptionSystem = Cast<UTATAIPerceptionSystem>(UAIPerceptionSystem::GetCurrent(worldContextObject)))
   {
      event.GlobalId = perceptionSystem->GenerateGlobalIdForHearingStim(event);
      perceptionSystem->OnEvent(event);

      if(ITATHearingStimSourceReactor* stimSourceReactor = Cast<ITATHearingStimSourceReactor>(instigator))
      {
         stimSourceReactor->HandleReactToOwnStim(tag, loudness);         
      }
   }
   UE_LOG(
      LogTATAISense_Hearing,
      Verbose,
      TEXT("[Noise Stim Event] Tag: [%s], Location: [%s], Instigator: [%s], Global ID: [%i]"),
      *tag.ToString(),
      *noiseLocation.ToString(),
      *AActor::GetDebugName(instigator),
      event.GlobalId
   );
   
   if (instigator && !world->IsNetMode(NM_DedicatedServer))
   {
      // Check if stim instigator is the player
      ATATPlayerController* localPlayerController = UOSECommon::GetLocalPlayerController<ATATPlayerController>(worldContextObject);
      bool isPlayerInstigator = _IsInstigatorPlayer(instigator, localPlayerController);

      // Uncomment if we want to include stims indirectly instigated by the player (i.e. object-thrown-by-player)
      //isPlayerInstigator |= _IsInstigatorPlayer(instigator->GetInstigator(), localPlayerController);

      if (isPlayerInstigator)
      {
         // notify local player
         localPlayerController->HandleLocalPlayerGeneratedNoiseStim(event);
      }
   }
}

void UTATAISense_Hearing::ResetGlobalStimIDForInstigator(UObject* worldContextObject, AActor* instigator)
{
   if(instigator == nullptr)
      return;
   if (UTATAIPerceptionSystem* perceptionSystem = Cast<UTATAIPerceptionSystem>(UAIPerceptionSystem::GetCurrent(worldContextObject)))
   {
      perceptionSystem->ResetGlobalIDForInstigator(instigator);
   }
}

void UTATAISense_Hearing::_OnNewListenerImpl(const FPerceptionListener& newListener)
{
   UAIPerceptionComponent* listenerPtr = newListener.Listener.Get();
   check(listenerPtr);
   const UTATAISenseConfig_Hearing* senseConfig = Cast<const UTATAISenseConfig_Hearing>(listenerPtr->GetSenseConfig(GetSenseID()));
   check(senseConfig);
   const FDigestedHearingProperties propertyDigest(*senseConfig);
   DigestedProperties.Add(newListener.GetListenerID(), propertyDigest);
}

void UTATAISense_Hearing::_OnListenerUpdateImpl(const FPerceptionListener& updatedListener)
{
   // @todo add updating code here
   const FPerceptionListenerID listenerID = updatedListener.GetListenerID();

   if (updatedListener.HasSense(GetSenseID()))
   {
      const UTATAISenseConfig_Hearing* senseConfig = Cast<const UTATAISenseConfig_Hearing>(updatedListener.Listener->GetSenseConfig(GetSenseID()));
      check(senseConfig);
      FDigestedHearingProperties& propertiesDigest = DigestedProperties.FindOrAdd(listenerID);
      propertiesDigest = FDigestedHearingProperties(*senseConfig);
   }
   else
   {
      DigestedProperties.Remove(listenerID);
   }
}

void UTATAISense_Hearing::_OnListenerRemovedImpl(const FPerceptionListener& updatedListener)
{
   DigestedProperties.FindAndRemoveChecked(updatedListener.GetListenerID());
}

// static
bool UTATAISense_Hearing::_IsInstigatorPlayer(const AActor* instigator, const APlayerController* localPlayerController)
{
   if (instigator == nullptr)
      return false;
   // local player controller can be null when simulating.
   if(localPlayerController == nullptr)
      return false;

   check(localPlayerController->IsLocalPlayerController());
   return instigator == localPlayerController
      || instigator == localPlayerController->GetPawn()
      || instigator == localPlayerController->PlayerState;
}

float UTATAISense_Hearing::Update()
{
   AIPerception::FListenerMap& listenersMap = *GetListeners();
   UAIPerceptionSystem* perceptionSystem = GetPerceptionSystem();
   const float speedOfSoundSqScalar = SpeedOfSoundSq > 0.f ? 1.f / SpeedOfSoundSq : 0.f;

   const UWorld* world = GetWorld();
   check(world);
   if (const UTATNoisePropagationSubsystem* propagationSubsystem = world->GetSubsystem<UTATNoisePropagationSubsystem>())
   {
      for (AIPerception::FListenerMap::TIterator listenerIt(listenersMap); listenerIt; ++listenerIt)
      {
         FPerceptionListener& listener = listenerIt->Value;

         // skip listeners not interested in this sense
         if (!listener.HasSense(GetSenseID()))
            continue;

         UAIPerceptionComponent* listenerPtr = listener.Listener.Get();
         check(listenerPtr);
         const UTATAISenseConfig_Hearing* senseConfig = Cast<const UTATAISenseConfig_Hearing>(listenerPtr->GetSenseConfig(GetSenseID()));
         check(senseConfig);

         const FDigestedHearingProperties& propDigest = DigestedProperties[listener.GetListenerID()];

         for (const FTATAINoiseEvent& event : NoiseEvents)
         {
            const float clampedLoudness = FMath::Max(0.f, event.Loudness);
            const float distToSoundSquared = FVector::DistSquared(event.NoiseLocation, listener.CachedLocation);
            FTATHearingEventStimSettings stimSettings;

            // Will be comprised of two parts:
            // 1. The stim settings tag.
            // 2. A global id as the number component.
            FName stimDataRowName;

            // Note: this implementation assumes sense configs can have unique hearing event stim settings, which in TAT just isn't the case.
            // So we're iterating over _every_ row for each noise event for every listener.
            // If we have a global stim settings file we could do this lookup once per noise event.
            UTATPerceptionFunctionLibrary::GetStimInfoByGameplayTag(
               UTATAISettings::GetHearingStimSettings(),
               event.Tag,
               stimSettings,
               stimDataRowName
            );
            
            if (!stimSettings.IsValid())
            {
               UE_LOG(
                  LogTATAISense_Hearing,
                  Error,
                  TEXT("Event %s does not exist in the hearing stim table!"),
                  *event.Tag.ToString()
               );
               continue;
            }

            float stimMaxRange = stimSettings.MaxRange;
            //MODIFY MAX RANGE OF STIM BASED OFF GAMEPLAY TAG SETTINGS FOR LISTENER
            if(propDigest.SharedConfigData.IsValid())
            {
               stimMaxRange *= propDigest.SharedConfigData->CalculateRangePerceptionModifiers(listenerPtr);
            }

            const float effectiveMaxRange = stimMaxRange * clampedLoudness;
            const bool inRange = stimMaxRange > 0.f && distToSoundSquared <= FMath::Square(effectiveMaxRange);
#if ENABLE_DRAW_DEBUG
            FColor debugColor = inRange ? FColor::Green : FColor::Red;
            if(TATAINoiseCVars::DrawStimDebug)
            {
               DrawDebugSphere(world, event.NoiseLocation, effectiveMaxRange, 6, debugColor, false, 2.f);   
            }
#endif
            if (inRange == false)
            {
               continue;
            }
#if ENABLE_DRAW_DEBUG
            if(TATAINoiseCVars::DrawStimDebug)
            {
               DrawDebugLine(world, event.NoiseLocation, listener.CachedLocation, debugColor, false, 2.f);
            }
#endif
            // Potentially limit based on LOS, depending on stim settings, and compute the perceived location in case it bounces
            FVector noisePerceivedLocation = FVector::ZeroVector;
            if (propagationSubsystem->IsNoiseEventHeardByListener(
               event.NoiseLocation,
               listener.GetBodyActor(),
               listener.CachedLocation,
               stimSettings,
               effectiveMaxRange,
               noisePerceivedLocation
            ) == false)
            {
               // Noise is occluded for this listener, skip it
               continue;
            }

            // filter out teams that mismatch the perception configuration
            const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(
               listener.GetBodyActor(),
               event.Instigator
            );
            if (!FOSEAISenseAffiliationFilter::ShouldSenseAttitude(attitude, propDigest.AffiliationFlags))
            {
               continue;
            }

            // filter out teams that mismatch the specific hearing stim configuration 
            // (loud combat noises should be heard by anyone, but a guard landing from a fall should not cause a noise stim that AI are interested in)
            const uint8 stimAffiliationTags = stimSettings.HeardBy.GetAsFlags();
            if (!FOSEAISenseAffiliationFilter::ShouldSenseAttitude(attitude, stimAffiliationTags))
            {
               continue;
            }

            // calculate delay and fake it with Age
            const float delay = FMath::Sqrt(distToSoundSquared * speedOfSoundSqScalar);

            // Cache the global Id of this stim in the FAIStimulus. There's not a great
            // place to include it (with engine mods) so we stuff it into the Tag.
            stimDataRowName.SetNumber(event.GlobalId);
         
            // pass over to listener to process
            perceptionSystem->RegisterDelayedStimulus(
               listener.GetListenerID(),
               delay,
               event.Instigator,
               FAIStimulus(
                  *this,
                  clampedLoudness,
                  noisePerceivedLocation,
                  listener.CachedLocation,
                  FAIStimulus::SensingSucceeded,
                  stimDataRowName
               )
            );
         }
      }
   }
   else
   {
      UE_LOG(LogTATAISense_Hearing, Warning, TEXT("Expected to find UTATNoisePropagationSubsystem, will assume all stims are occluded"));
   }

   NoiseEvents.Reset();

   // return decides when next tick is going to happen
   return SuspendNextUpdate;
}

void UTATAISense_Hearing::RegisterEvent(const FTATAINoiseEvent& event)
{
   NoiseEvents.Add(event);

   RequestImmediateUpdate();
}

void UTATAISense_Hearing::RegisterEventsBatch(const TArray<FTATAINoiseEvent>& events)
{
   NoiseEvents.Append(events);

   RequestImmediateUpdate();
}

void UTATAISense_Hearing::RegisterWrappedEvent(UAISenseEvent& perceptionEvent)
{
   UTATAISenseEvent_Hearing* hearingEvent = Cast<UTATAISenseEvent_Hearing>(&perceptionEvent);
   ensure(hearingEvent);
   if (hearingEvent)
   {
      RegisterEvent(hearingEvent->GetNoiseEvent());
   }
}

