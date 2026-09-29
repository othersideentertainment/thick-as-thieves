// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/OSEAISenseSharedConfigData.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AISense_Hearing.h"

#include "TATAISense_Hearing.generated.h"

class UTATAISenseConfig_Hearing;
struct FTATHearingEventStimSettings;

USTRUCT(BlueprintType)
struct TAT_API FTATAINoiseEvent
{   
   GENERATED_BODY()

   typedef class UTATAISense_Hearing FSenseClass;

   float Age;

   // if not set Instigator's location will be used
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   FVector NoiseLocation;

   // Loudness modifier of the sound.
   // If MaxRange is non-zero, this modifies the range (by multiplication).
   // If there is no MaxRange, then if Square(DistanceToSound) <= Square(HearingRange) * Loudness, the sound is heard, false otherwise.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense", meta = (UIMin = 0, ClampMin = 0))
   float Loudness;
   
   // Actor triggering the sound.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   AActor* Instigator;

   // Gameplay tag identifier for the noise.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   FGameplayTag Tag;

   // Global identifier for this hearing stim, used to reconcile between
   // stims that are locally cached (and Id'd) on AI.
   int32 GlobalId = INDEX_NONE;
      
   FTATAINoiseEvent();
   FTATAINoiseEvent(AActor* inInstigator, const FVector& inNoiseLocation, FGameplayTag inTag);

   // Verifies and calculates derived data
   void Compile();
};

UCLASS(ClassGroup=AI, Config=Game)
class TAT_API UTATAISense_Hearing : public UAISense
{
   GENERATED_BODY()
      
public:
   UTATAISense_Hearing(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   void RegisterEvent(const FTATAINoiseEvent& event);
   void RegisterEventsBatch(const TArray<FTATAINoiseEvent>& events);

   virtual void PostInitProperties() override;

   // part of BP interface. Translates PerceptionEvent to FAINoiseEvent and call RegisterEvent(const FAINoiseEvent& Event)
   virtual void RegisterWrappedEvent(UAISenseEvent& perceptionEvent) override;

   // Report a noise event (TAT version)
   //  @param Tag Gameplay tag identifier for the event, used to pull stim settings from for range / occlusion settings
   //  @param NoiseLocation Location of the noise.
   //  @param Instigator Actor that triggered the noise.
   //  @param SuppressGlyphIndicator Never spawn a glyph indicator for this event, even if one is configured for this tag
   UFUNCTION(BlueprintCallable, Category = "AI|Perception|TAT", meta = (WorldContext="worldContextObject", GameplayTagFilter = "AI.Stim.Hearing"), DisplayName = "Report Noise Event (TAT)")
   static void ReportNoiseEvent(UObject* worldContextObject, FGameplayTag tag, FVector noiseLocation, AActor* instigator, bool suppressGlyphIndicator = false);
   
   UFUNCTION(BlueprintCallable, Category = "AI|Perception|TAT", meta = (WorldContext="worldContextObject", GameplayTagFilter = "AI.Stim.Hearing"), DisplayName = "Report Noise Event with Loudness (TAT)")
   static void ReportNoiseEventWithLoudness(UObject* worldContextObject, FGameplayTag tag, FVector noiseLocation, AActor* instigator, bool suppressGlyphIndicator = false, float loudness = 1.f);

   UFUNCTION(BlueprintCallable, Category = "AI|Perception|TAT", meta = (WorldContext="worldContextObject"), DisplayName = "Reset Global Stim ID For Instigator (TAT)")
   static void ResetGlobalStimIDForInstigator(UObject* worldContextObject, AActor* instigator);
   
protected:
   virtual float Update() override;
   void _RegisterMakeNoiseDelegate();

   void _OnNewListenerImpl(const FPerceptionListener& newListener);
   void _OnListenerUpdateImpl(const FPerceptionListener& updatedListener);
   void _OnListenerRemovedImpl(const FPerceptionListener& updatedListener);
   bool _IsNoiseEventOccluded(const FTATAINoiseEvent& event, const FPerceptionListener& listener, const FTATHearingEventStimSettings& stimSettings) const;
   // Compares actor against player controller (and associated pawn / player state)
   static bool _IsInstigatorPlayer(const AActor* instigator, const APlayerController* localPlayerController);
   
protected:
   UPROPERTY()
   TArray<FTATAINoiseEvent> NoiseEvents;

   // Defaults to 0 to have instant notification. Setting to > 0 will result in delaying 
   // when AI hears the sound based on the distance from the source
   UPROPERTY(Config)
   float SpeedOfSoundSq;

   struct FDigestedHearingProperties
   {
      uint8 AffiliationFlags;
      TWeakObjectPtr<UOSEAISenseSharedConfigData> SharedConfigData;

      FDigestedHearingProperties(const UTATAISenseConfig_Hearing& senseConfig);
      FDigestedHearingProperties();
   };
   TMap<FPerceptionListenerID, FDigestedHearingProperties> DigestedProperties;
};
