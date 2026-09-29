// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/VoiceOver/OSETrimVoiceOverLineDataCommandlet.h"

// ose
#include "Tools/OSEAssetRegistryUtl.h"
#include "VoiceOver/OSEVoiceOverLine.h"

// ue
#include "AkAudioEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSETrimVoiceOverLineDataCommandlet)
DEFINE_LOG_CATEGORY_STATIC(LogOSETrimVoiceOverLineDataCommandlet, Log, All);

static FString ignoreEmptyLinesSwitch = TEXT("IgnoreEmptyLines");
static FString trimUnaccompaniedTTSSwitch = TEXT("TrimAccompaniedTTSLines");
static FString dryRunSwitch = TEXT("dryrun");

int UOSETrimVoiceOverLineDataCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> tokens, switches;
   ParseCommandLine(*fullCommandLine, tokens, switches);
   
   for (const FString& searchPath : _volFilterPackagePaths)
   {
      UE_LOG(LogOSETrimVoiceOverLineDataCommandlet, Verbose, TEXT("Searching for VOLs within %s..."), *searchPath);
   }

   TArray<UOSEVoiceOverLine*> voiceLines = OSEAssetRegistryUtl::FindAndLoadAssets<UOSEVoiceOverLine>(_volFilterPackagePaths);
   TArray<UPackage*> modifiedPackages;
   _TrimLines(voiceLines, modifiedPackages, switches);

   if (switches.Contains(dryRunSwitch))
   {
      return 0;
   }

   const bool noSourceControl = switches.Contains(TEXT("NoSourceControl"));
   return OSEAssetRegistryUtl::SavePackages(modifiedPackages, noSourceControl) ? 0 : 1;
}

void UOSETrimVoiceOverLineDataCommandlet::_TrimLines(TArray<UOSEVoiceOverLine*> voiceOverLines, TArray<UPackage*>& modifiedPackages, const TArray<FString>& switches)
{
   struct FOSERemovedVOLLineData
   {
      FOSERemovedVOLLineData(FGameplayTag identity, FOSEVoiceOverLineData lineData) 
         : Identity(identity), LineData(lineData) {}

      FGameplayTag Identity;
      FOSEVoiceOverLineData LineData;

      // for sorting
      FORCEINLINE bool operator<(const FOSERemovedVOLLineData& other) const { return Identity < other.Identity; }
   };

   const bool dryRun = switches.Contains(dryRunSwitch);

   int32 totalLinesRemoved = 0;
   TMap<UOSEVoiceOverLine*, TArray<FOSERemovedVOLLineData>> removedLinesPerVOL;
   for (UOSEVoiceOverLine* voiceOverLine : voiceOverLines)
   {
      check(voiceOverLine);

      int32 linesRemovedForVOL = 0;
      for (auto& identityIterator : voiceOverLine->Identities)
      {
         const FGameplayTag identity = identityIterator.Key;
         FOSEVoiceOverLineIdentityData& identityData = identityIterator.Value;
         
         for (int32 lineIndex = identityData.Lines.Num() - 1; lineIndex >= 0; lineIndex--)
         {
            if (_ShouldTrimLine(identityData, lineIndex, switches))
            {
               // Add to existing (or create new) array of removed lines per VOL
               const FOSEVoiceOverLineData& removedLine = identityData.Lines[lineIndex];
               TArray<FOSERemovedVOLLineData>& removedLines = removedLinesPerVOL.FindOrAdd(voiceOverLine);
               removedLines.Emplace(identity, removedLine);

               // Remove line and update running counts
               if (!dryRun)
               {
                  identityData.Lines.RemoveAt(lineIndex);
               }
               lineIndex--;
               totalLinesRemoved++;
            }
         }
      }

      TArray<FOSERemovedVOLLineData>* removedLines = removedLinesPerVOL.Find(voiceOverLine);
      if (removedLines)
      {
         // Sort by identity tag
         removedLines->Sort();
         
         UE_LOG(LogOSETrimVoiceOverLineDataCommandlet, Verbose, TEXT("Trimming %d lines from %s:")
            , removedLines->Num()
            , *voiceOverLine->GetName());

         for (const FOSERemovedVOLLineData& removedLineData : *removedLines)
         {
            UE_LOG(LogOSETrimVoiceOverLineDataCommandlet, Verbose, TEXT("* %s under %s")
               , *GetNameSafe(removedLineData.LineData.AudioEvent)
               , *removedLineData.Identity.ToString());
         }

         if (!dryRun)
         {
            // Return package to caller for saving
            modifiedPackages.Add(voiceOverLine->GetPackage());
         }
      }
      UE_CLOG(removedLines != nullptr, LogOSETrimVoiceOverLineDataCommandlet, VeryVerbose, TEXT("%d lines removed from %s")
         , removedLines->Num()
         , *voiceOverLine->GetName());
   }
   
   UE_LOG(LogOSETrimVoiceOverLineDataCommandlet, Log, TEXT("TOTAL: %d lines %s across %d VOLs")
      , totalLinesRemoved
      , dryRun ? TEXT("would be removed") : TEXT("removed")
      , removedLinesPerVOL.Num());
}

bool UOSETrimVoiceOverLineDataCommandlet::_ShouldTrimLine(const FOSEVoiceOverLineIdentityData& identityData, int32 lineIndex, const TArray<FString>& switches) const
{
   const bool trimEmptyLines = !switches.Contains(ignoreEmptyLinesSwitch);
   const bool trimAccompaniedTTSLines = switches.Contains(trimUnaccompaniedTTSSwitch);

   if (!ensure(identityData.Lines.IsValidIndex(lineIndex)))
   {
      return false;
   }

   // Remove lines without assigned ak event
   const FOSEVoiceOverLineData& line = identityData.Lines[lineIndex];
   if (!line.AudioEvent)
   {
      return trimEmptyLines;
   }

   // Remove TTS lines if accompanied by other lines (assumed to be non-TTS)
   if (trimAccompaniedTTSLines && identityData.Lines.Num() > 1)
   {
      if (line.AudioEvent->GetName().Contains(TEXT("debugTTS")))
      {
         return true;
      }
   }

   return false;
}
