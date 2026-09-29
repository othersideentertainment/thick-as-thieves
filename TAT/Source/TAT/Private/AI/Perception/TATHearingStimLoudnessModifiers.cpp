// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Perception/TATHearingStimLoudnessModifiers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHearingStimLoudnessModifiers)

DEFINE_LOG_CATEGORY_STATIC(LogTATHearingStimLoudnessModifiers, Log, All);

float UTATHearingStimLoudnessModifiers::GetLoudnessMultiplierForMaterial(UPhysicalMaterial* material) const
{
   if(const FTATHearingStimLoudnessModifier* foundSetting = _PhysicalMaterialToLoudnessModifier.Find(material))
   {
      return foundSetting->Multiplier;
   }
   if(material && material->IsRooted() == false)
   {
      UE_LOG(LogTATHearingStimLoudnessModifiers, Warning, TEXT("[TAT Loudness Modifier] Can't find modifier for %s - defaulting to 1.f "), *GetNameSafe(material));
#if WITH_EDITOR
      if(GEngine)
      {
         // unique line per material
         GEngine->AddOnScreenDebugMessage(
            static_cast<uint64>(reinterpret_cast<uintptr_t>(material)),
            15.0f,
            FColor::Yellow,
            FString::Format(TEXT("[TAT Loudness Modifier] Can't find modifier for {0} - defaulting to 1.f "), {*GetNameSafe(material) })
         );
      }
#endif
   }
   return 1.f;
}
