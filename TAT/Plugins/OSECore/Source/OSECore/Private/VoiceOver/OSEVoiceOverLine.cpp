// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverLine.h"

//ose
#include "Audio/OSEAkAudioComponentSystemInterface.h"
#include "OSECommon.h"
#include "VoiceOver/OSEVoiceOverEventHandlerInterface.h"

//wwise
#include "AkAudioEvent.h"
#include "AkComponent.h"

//ue5
#include "HAL/PlatformFileManager.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverLine)

int FOSEVoiceOverLineIdentityData::GetRandomLineIndex(FRandomStream& randomStream)
{
   // Lazily create the shuffling array
   if (_lineShuffling.Num() != Lines.Num())
   {
      _lineShuffling.Reset(Lines.Num());
      for (int32 i = 0; i < Lines.Num(); ++i)
      {
         _lineShuffling.Add(i);
      }

      // Reset line index
      // NOTE: for some reason, despite being marked as Transient the _lineIndex value can persist from prior PIE sessions. 
      // This means that, if elements are removed from Lines, the non-zero _lineIndex value may now point out of bounds.
      _lineIndex = 0;
   }

   int index = _lineShuffling[_lineIndex++];

   if (_lineIndex >= _lineShuffling.Num())
   {
      UOSECommon::ShuffleArray(_lineShuffling, randomStream);
      _lineIndex = 0;
   }

   return index;
}

FOSEVoiceOverLineIdentityData* UOSEVoiceOverLine::GetIdentityData(const UAkComponent* voComponent)
{
   check(voComponent && voComponent->Implements<UOSEVoiceOverEventHandlerInterface>());
   FGameplayTag identity = IOSEVoiceOverEventHandlerInterface::Execute_GetVoiceIdentity(voComponent);
   return Identities.Find(identity);
}

const FOSEVoiceOverLineIdentityData* UOSEVoiceOverLine::GetIdentityData(const UAkComponent* voComponent) const
{
   check(voComponent && voComponent->Implements<UOSEVoiceOverEventHandlerInterface>());
   FGameplayTag identity = IOSEVoiceOverEventHandlerInterface::Execute_GetVoiceIdentity(voComponent);
   return Identities.Find(identity);
}

FOSEVoiceOverLineData UOSEVoiceOverLine::ResolveVoiceLineData(FRandomStream& randomStream, const UAkComponent* voComponent)
{
   FOSEVoiceOverLineIdentityData* identityData = GetIdentityData(voComponent);

   if (!identityData || identityData->Lines.Num() == 0)
   {
      return FOSEVoiceOverLineData();
   }

   int index = identityData->GetRandomLineIndex(randomStream);

   return identityData->Lines[index];
}

bool UOSEVoiceOverLine::HasAnyLines(const AActor* actor) const
{
   if (!IsValid(actor))
      return false;

   if (!actor->Implements<UOSEAkAudioComponentSystemInterface>())
      return false;

   UAkComponent* voComponent = IOSEAkAudioComponentSystemInterface::Execute_GetAkComponent(actor, EAkComponentType::Voice);

   if (!IsValid(voComponent))
      return false;

   if (!voComponent->Implements<UOSEVoiceOverEventHandlerInterface>())
      return false;

   const FOSEVoiceOverLineIdentityData* identityData = GetIdentityData(voComponent);
   if (!identityData)
      return false;

   return identityData->Lines.Num() > 0;
}

#if WITH_EDITOR
EDataValidationResult UOSEVoiceOverLine::IsDataValid(class FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   for (const TPair<FGameplayTag, FOSEVoiceOverLineIdentityData>& identityData : Identities)
   {
      if (!identityData.Key.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("VO line %s has entry with invalid tag"), *GetName())));
      }

      for (const FOSEVoiceOverLineData& line : identityData.Value.Lines)
      {
         if (!line.AudioEvent)
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("Invalid event in VO line %s identity %s"), *GetName(), *identityData.Key.ToString())));
         }
      }
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

