// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/UnifiedStealthSystem/TATStealthScoreComponent.h"

// tat
#include "AI/Perception/TATHearingTypes.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"
#include "AI/TATAISettings.h"
#include "AI/UnifiedStealthSystem/TATUnifiedStealthSettings.h"
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "OSELightDetectionInterface.h"
#include "Items/ToolSetComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagAssetInterface.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStealthScoreComponent)

namespace CVars
{
   static int32 DebugDrawStealthScore = 0;
   static FAutoConsoleVariableRef CVarDebugDrawStealthScore(
      TEXT("tat.StealthScore.DebugDraw"),
      DebugDrawStealthScore,
      TEXT("Debug drawing for stealth score"),
      ECVF_Default);
}

namespace StealthScoreHelpers
{
   static uint8 FloatToU8(const float score)
   {
      return FMath::Clamp(score, 0, 1) * 0xFF;
   }

   static float U8ToFloat(const uint8 score)
   {
      return static_cast<float>(score) / 255.0f;
   }
}

UTATStealthScoreComponent::UTATStealthScoreComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   SetIsReplicatedByDefault(true);
}

void UTATStealthScoreComponent::BeginPlay()
{
   Super::BeginPlay();

   if(GetOwner()->HasAuthority())
   {
      SetComponentTickEnabled(true);
      _toolSetComponent = UTATToolFunctionLibrary::GetToolSetComponentFromActor(GetOwner());
   }
}

void UTATStealthScoreComponent::TickComponent(const float deltaTime, const ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   check(GetOwner()->HasAuthority());
   const UTATUnifiedStealthSettings& settings = UTATUnifiedStealthSettings::Get();

   if(_CalculateStealthScore(settings, deltaTime, _authorityStealthScore))
   {
      const float preReplicatedScore = _replicatedStealthInterpScore;
      _replicatedStealthInterpScore = StealthScoreHelpers::FloatToU8(_authorityStealthScore);
      if(preReplicatedScore != _replicatedStealthInterpScore)
      {
         _HandleStealthScoreChanged();
      }
   }

#if ENABLE_DRAW_DEBUG
   if (CVars::DebugDrawStealthScore)
   {
      DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(), 150, 10, FMath::Lerp(FLinearColor::Black, FLinearColor::Yellow, GetStealthDetectionScore()).ToFColorSRGB());
   }
#endif // ENABLE_DRAW_DEBUG

   _UpdateStealthyEffect();
}

void UTATStealthScoreComponent::HandleOwnStimReaction(const FGameplayTag& stimTag, float loudness)
{
   // No point sending a message if the loudness is zero.
   if (FMath::IsNearlyZero(loudness))
      return;
   
   FTATHearingEventStimSettings stimSettings;
   FName stimDataRowName;
   UTATPerceptionFunctionLibrary::GetStimInfoByGameplayTag(
              UTATAISettings::GetHearingStimSettings(),
              stimTag,
              stimSettings,
              stimDataRowName
           );

   if(stimSettings.IsValid() == false)
      return;
   
   // We could do some tag base rate limiting here, for instance only send the message _if_ the loudness is greater than
   // the last event sent if it's within N seconds of the last event.
   // For now, send every event, the message is unreliable and the data sent _should_ be small.
   ClientHearingStimTriggered(stimTag, loudness);
}

float UTATStealthScoreComponent::GetStealthDetectionScore() const
{
   return StealthScoreHelpers::U8ToFloat(_replicatedStealthInterpScore);
}

void UTATStealthScoreComponent::_HandleStealthScoreChanged() const
{
   OnStealthScoreChanged.Broadcast();
}

bool UTATStealthScoreComponent::_CalculateStealthScore(const UTATUnifiedStealthSettings& settings, const float deltaTime, float& interpScore) const
{
   float actualScore = 0.f;
   const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(GetOwner());
   if(tagAssetInterface == nullptr)
      return false;

   const IOSELightDetectionInterface* lightDetectionInterface = Cast<IOSELightDetectionInterface>(GetOwner());
   if(lightDetectionInterface == nullptr)
      return false;
#if ENABLE_DRAW_DEBUG
   const bool isDebugRelevant = CVars::DebugDrawStealthScore != 0 && GetOwner()->GetInstigatorController() == GetWorld()->GetFirstPlayerController(); 
   FString debugString;
#endif
   
   // grab the current light intensity value
   const float lightIntensityValue = lightDetectionInterface->GetActualLightIntensityFromLightSources();
   if(settings.ShouldUseSteppedLighting)
   {
      for (const FTATUnifiedStealthLightValueSetting& lightValueSetting : settings.LightValueSettings)
      {
         if(lightIntensityValue > lightValueSetting.MinValueForModifier &&
            lightIntensityValue <= lightValueSetting.MaxValueForModifier)
         {
            actualScore += lightValueSetting.Value;
#if ENABLE_DRAW_DEBUG
            if(isDebugRelevant)
            {
               debugString += FString::Printf(TEXT("Light Intensity %f > %f && < %f: %f\n"), lightIntensityValue, lightValueSetting.MinValueForModifier, lightValueSetting.MaxValueForModifier, lightValueSetting.Value);
            }
#endif
         }
      }
   }
   else
   {
      actualScore = 1.f - lightIntensityValue;
   }
   
   FGameplayTagContainer tagsOnCharacter;
   tagAssetInterface->GetOwnedGameplayTags(tagsOnCharacter);
   float multiplierToUse = 1.f;
   // calculate the aggregate score by going over all the tags applied to the target and adding up the score
   for (const FTATUnifiedStealthTagSetting& tagSetting : settings.TagSettings)
   {
      if(tagSetting.RequiredQuery.Matches(tagsOnCharacter))
      {
         actualScore += tagSetting.ValueToAdd;
         multiplierToUse *= tagSetting.MultiplierToTotalScore;
#if ENABLE_DRAW_DEBUG
         if(isDebugRelevant)
         {
            debugString += FString::Printf(TEXT("%s : %f\n"), *tagSetting.RequiredQuery.GetDescription(), tagSetting.ValueToAdd);
         }
#endif
      }         
   }

   // check if we have a tool equipped or not
   if (_toolSetComponent)
   {
      const int toolModifier = _toolSetComponent->GetCurrentTool() ? settings.DeBuffForSomethingEquipped : settings.BuffForNothingEquipped;
      actualScore += toolModifier;
#if ENABLE_DRAW_DEBUG
      if(isDebugRelevant)
      {
         debugString += FString::Printf(TEXT("Has Tool? %s : %i\n"),_toolSetComponent->GetCurrentTool()? TEXT("Yes") : TEXT("No"), toolModifier);
      }
#endif
   }
   
   actualScore *= multiplierToUse;
   const float speedMultiplier = actualScore > 0 ? settings.IncreasingSpeedMultiplier : settings.DecreasingSpeedMultiplier;
   
   // clamp the calculated score between the max negative debuff value and the max positive buff value      
   actualScore = FMath::Clamp(actualScore, 0.f, 1.f);
   interpScore = FMath::FInterpTo(interpScore, actualScore, deltaTime, speedMultiplier);
   
#if ENABLE_DRAW_DEBUG
   if(isDebugRelevant)
   {
      debugString += FString::Printf(TEXT("Final Score : %f\nModifiers : %f"),interpScore, actualScore);
      GEngine->AddOnScreenDebugMessage(27, 1, FColor::Red, debugString);
   }
#endif
   return true;
}

void UTATStealthScoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   
   DOREPLIFETIME(UTATStealthScoreComponent, _replicatedStealthInterpScore);
}

void UTATStealthScoreComponent::_UpdateStealthyEffect()
{
   if(!IsValid(_stealthyEffect))
   {
      return;
   }
   const bool shouldHaveEffect = _authorityStealthScore > _stealthyEffectThreshold;
   if(shouldHaveEffect == _stealthyEffectHandle.IsValid())
   {
      return;
   }

   if(shouldHaveEffect)
   {
      if(UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner(), false))
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         _stealthyEffectHandle = asc->ApplyGameplayEffectToSelf(_stealthyEffect.GetDefaultObject(), 0.0f, effectContext);
      }
   }
   else
   {
      if(UAbilitySystemComponent* asc = _stealthyEffectHandle.GetOwningAbilitySystemComponent())
      {
         asc->RemoveActiveGameplayEffect(_stealthyEffectHandle, 1);
      }
      _stealthyEffectHandle.Invalidate();
   }
}

void UTATStealthScoreComponent::ClientHearingStimTriggered_Implementation(FGameplayTag stimTag, float loudness)
{
   FTATHearingEventStimSettings stimSettings;
   FName stimDataRowName;
   UTATPerceptionFunctionLibrary::GetStimInfoByGameplayTag(
              UTATAISettings::GetHearingStimSettings(),
              stimTag,
              stimSettings,
              stimDataRowName
           );
   if (stimSettings.IsValid() == false)
      return;
   
   const float modifiedRange = stimSettings.MaxRange * loudness;
   OnOwnHearingStimEvent.Broadcast(stimSettings.Severity, modifiedRange);   
}

void UTATStealthScoreComponent::OnRep_ReplicatedStealthScore()
{
   _HandleStealthScoreChanged();
}
