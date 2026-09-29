// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Lockpicking/TATCombinationLockComponent.h"

// tat
#include "Interactables/TATLockConfig.h"
#include "Lockpicking/TATCombinationHelpers.h"
#include "Lockpicking/TATLockpickingSettings.h"
#include "Variation/TATActorDerivedSeed.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawnTypes.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "Online/TATGameState.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Character.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCombinationLockComponent)


namespace CombinationHelpers
{
   static FTATSharedClueFormatParams CreateCombinationParams(int16 combination)
   {
      FTATClueFormatParams params;
      params.Add(TEXT("Combination"), FormatCombination(combination));
      return MakeSharedClueFormatParams(MoveTemp(params));
   }
}

UTATCombinationLockComponent::UTATCombinationLockComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATCombinationLockComponent::BeginPlay()
{
   Super::BeginPlay();

   // Initialize seed deterministically without replication
   // (could also evaluate on demand (or authority-only) if worried about exploits)
   _combination = CombinationHelpers::GenerateCombination(_GenerateSeed());

   if(GetOwner()->HasAuthority() && !_clueSet.IsNull() && UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _clueSceneRequirement))
   {
      if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
      {
         if (UTATMapVariationMgrComponent* variationMgr = gameState->GetMapVariationMgr())
         {
            FTATPendingClueSource clueRequest;
            clueRequest.ClueSet = _clueSet;
            clueRequest.Location = UTATDummyClueLocation::Get();
            clueRequest.ExtraFormatParams = CombinationHelpers::CreateCombinationParams(_combination);
            variationMgr->AuthorityAddPendingClues(clueRequest);
         }
      }
   }
}

#if WITH_EDITOR
void UTATCombinationLockComponent::CheckForErrors()
{
   Super::CheckForErrors();

   // TODO: probably do check on classes in IsDataValid rather than just map check, but not if owner is abstract
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (_combinationMode == ETATCombinationLockMode::FromName && _combinationName.IsNone())
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(GetOwner(), FText::FromString(GetOwner()->GetActorNameOrLabel())))
            ->AddToken(FTextToken::Create(INVTEXT("Combination lock uses FromName mode, but name is empty")));
      }
   }
}
#endif

bool UTATCombinationLockComponent::TryAddToPrompt(FInteractPrompt& prompt, FLockInteractContext lockContext) const
{
   if(lockContext.bIsLockedInCurrentDirection)
   {
      prompt.PressAction = UTATLockpickingSettings::GetLockpickingSettingsRef().CombinationLockPrompt;
      return true;
   }
   
   return false;
}

bool UTATCombinationLockComponent::TryHandleInteractStart(TScriptInterface<ILockpickableInterface> lockable,
   ACharacter* interactingCharacter, FLockInteractContext lockContext, FInteractStartResult& outResult)
{
   if(lockContext.bIsLockedInCurrentDirection)
   {
      FGameplayEventData eventData;
      eventData.OptionalObject = lockable.GetObject();
      eventData.OptionalObject2 = this;

      FGameplayTag eventTag = UTATLockpickingSettings::GetLockpickingSettingsRef().CombinationLockEvent;

      UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(interactingCharacter, eventTag, eventData);
      return true;
   }

   return false;
}

bool UTATCombinationLockComponent::IsValidCombination(int32 combination) const
{
   return _combination == combination;
}

FText UTATCombinationLockComponent::GetCombinationText() const
{
   return CombinationHelpers::FormatCombination(_combination);
}

int32 UTATCombinationLockComponent::_GenerateSeed() const
{
   switch(_combinationMode)
   {
   case ETATCombinationLockMode::Unique:
      return TATActorDerivedSeed::GetDerivedSeedForActor(GetOwner(), TEXT("Combination"));
   case ETATCombinationLockMode::FromName:
      return CombinationHelpers::MakeSeedForName(_combinationName.Name, TATActorDerivedSeed::GetMapSeed(this));
   }

   checkNoEntry();
   return 0;
}

