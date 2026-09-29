// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATLockConfig.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Interactables/TATLockPicker.h"
#include "Items/TATItemInventorySystemInterface.h"
#include "Items/TATItemInventoryComponent.h"
#include "Items/TATItemInfo.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Lockpicking/TATAlternateLockInterface.h"
#include "Lockpicking/TATLockpickingSettings.h"
#include "Variation/TATActorDerivedSeed.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue4
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockConfig)
DEFINE_LOG_CATEGORY_STATIC(LogTATLockConfig, Log, All);

#define LOCTEXT_NAMESPACE "TATLockConfig"

namespace LockConfigHelpers
{
   static void TryExecuteCueOnActor(FGameplayTag tag, AActor* actor)
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
      {
         asc->ExecuteGameplayCue(tag);
      }
   }
}

void FTATLockConfig::RandomizeLockLevel(const AActor* actor)
{
   if (actor != nullptr)
   {
      FRandomStream actorRandomStream;
      actorRandomStream.Initialize(TATActorDerivedSeed::GetDerivedSeedForActor(actor, TEXT("LockConfig")));
      const int baseLockLevel = actorRandomStream.RandRange(MinLockLevel, MaxLockLevel);
      
      const ETATDifficulty difficulty = TATDifficulty::GetDifficultyForMatch(actor->GetWorld());
      const int difficultyLevelAdjustment = UTATLockpickingSettings::GetLockpickingSettings()->DifficultyLevelOffset[static_cast<int32>(difficulty)];

      _finalLockLevel = baseLockLevel + difficultyLevelAdjustment;
      _isInitialized = true;
   }
   else
   {
      _finalLockLevel = 1; // default to min value
      UE_LOG(LogTATLockConfig, Warning, TEXT("Could not randomize lock config, actor is null"));
   }
}

int FTATLockConfig::GetFinalLockLevel() const
{
   ensure(_isInitialized);
   return _finalLockLevel;
}

bool FTATLockConfig::CanActorLockpick(const AActor* actor)
{
   if (const ITATLockPicker* lockPicker = Cast<ITATLockPicker>(actor))
   {
      return lockPicker->CanLockPick();
   }
   return false;
}

bool FTATLockConfig::DoesCharacterHaveKey(const ACharacter* character) const
{
   if (!KeyTag.IsValid() || character == nullptr)
   {
      return false;
   }
   if(character->IsA<ATATCharacterAIBase>())
   {
      return true;
   }
   // Could change representation of key ownership later
   if (auto inventoryHolder = Cast<ITATItemInventorySystemInterface>(character))
   {
      if (UTATItemInventoryComponent* inventory = inventoryHolder->GetTATItemInventory())
      {
         bool val = inventory->HasItemOfCategory(KeyTag);
         return val;
      }
   }
   return false;
}

bool FTATLockConfig::DoesCharacterHaveIncorrectKey(const ACharacter* character) const
{
   if (!IncorrectKeyTags.IsValid() || character == nullptr)
   {
      return false;
   }

   // Could change representation of key ownership later
   if (auto inventoryHolder = Cast<ITATItemInventorySystemInterface>(character))
   {
      if (UTATItemInventoryComponent* inventory = inventoryHolder->GetTATItemInventory())
      {
         for(const FGameplayTag& keyTag : IncorrectKeyTags)
         {
            if(inventory->HasItemOfCategory(keyTag))
            {
               return true;
            }
         }
      }
   }
   return false;
}

void FTATLockConfig::AddToPrompt(FInteractPrompt& prompt, FLockInteractContext lockContext, const ACharacter* character) const
{
   if(AlternateLock && AlternateLock->TryAddToPrompt(prompt, lockContext))
   {
      return;
   }
   
   const FTATLockInteractPrompts& promptText = UTATLockpickingSettings::GetLockpickingSettings()->InteractPrompts;
   if (lockContext.CanUnlockWithKey())
   {
      prompt.HoldAction = promptText.UnlockWithKeyPrompt;
      prompt.InteractStatusTag = LockedInteractStatusTag;
   }
   else if (lockContext.CanLockpick())
   {
      // TODO: I could potentially cache this elsewhere, but :shrug:
      prompt.PressAction = promptText.LockpickPrompt;
      prompt.ErrorMessage = _lockLevelPromptCache.Get([&]() { return FText::FormatNamed(promptText.LockLevelPrompt, TEXT("LockLevel"), _finalLockLevel); });
      prompt.InteractStatusTag = LockedInteractStatusTag;
   }
   else if (lockContext.CanLockWithKey())
   {
      prompt.HoldAction = promptText.LockWithKeyPrompt;
      prompt.InteractStatusTag = UnlockedInteractStatusTag;
   }
   else if (lockContext.CanLockWithoutKey())
   {
      prompt.HoldAction = promptText.LockWithoutKeyPrompt;
      prompt.InteractStatusTag = UnlockedInteractStatusTag;
   }
   else if (lockContext.bIsLockedInCurrentDirection && !lockContext.CanLockpick())
   {
      if (lockContext.RequiresKey())
      {
         if(!IncorrectKeyMessage.IsEmpty() && DoesCharacterHaveIncorrectKey(character))
         {
            prompt.ErrorMessage = IncorrectKeyMessage;
         }
         else
         {
            FText keyName = KeyItemInfo ? KeyItemInfo.GetDefaultObject()->Name : LOCTEXT("InvalidItem", "NULL KEY");
            prompt.ErrorMessage = FText::FormatNamed(promptText.UnlockRequiresKeyPrompt, TEXT("Key"), keyName);
         }
      }
      else if (lockContext.bAreAllSidesLocked || (!lockContext.bCanBePickedInCurrentDirection && lockContext.bCanInteractorLockpick))
      {
         prompt.ErrorMessage = promptText.CannotLockpickPrompt;
      }
      else
      {
         prompt.ErrorMessage = promptText.CannotLockpickWrongSidePrompt;
      }
      prompt.InteractStatusTag = LockedInteractStatusTag;
   }
}

bool FTATLockConfig::TryHandleInteractStart(TScriptInterface<ILockpickableInterface> lockable,
                                            ACharacter* interactingCharacter,
                                            const FLockInteractContext& lockContext,
                                            FInteractStartResult& outResult) const
{
   if(AlternateLock && AlternateLock->TryHandleInteractStart(lockable, interactingCharacter, lockContext, outResult))
   {
      return true;
   }
   
   if (lockContext.CanLockpick())
   {
      StartLockpicking(interactingCharacter, lockable);
      return true;
   }
   if (lockContext.CanLockWithKey() || lockContext.CanUnlockWithKey())
   {
      const auto settings = UTATLockpickingSettings::GetLockpickingSettings();
      outResult = FInteractStartResult::Wait(settings->UseKeyInteractTime);
      outResult.HoldAnimationTag = settings->UseKeyInteractAnimation;
      return true;
   }
   if (lockContext.CanLockWithoutKey())
   {
      const auto settings = UTATLockpickingSettings::GetLockpickingSettings();
      outResult = FInteractStartResult::Wait(settings->LockWithoutKeyInteractTime);
      outResult.HoldAnimationTag = settings->LockWithoutKeyInteractAnimation;
      outResult.HoldActionCues = settings->LockWithoutKeyHeldCues;
      return true;
   }
   
   return false;
}

void FTATLockConfig::HandleInteractComplete(TScriptInterface<ILockpickableInterface> lockable,
                                            ACharacter* interactingCharacter,
                                            const FLockInteractContext& lockContext) const
{
   const auto settings = UTATLockpickingSettings::GetLockpickingSettings();
   if (lockContext.CanLockWithKey())
   {
      LockConfigHelpers::TryExecuteCueOnActor(settings->LockWithKeyCue, interactingCharacter);
      lockable->LockWithKey();
   }
   else if (lockContext.CanUnlockWithKey())
   {
      LockConfigHelpers::TryExecuteCueOnActor(settings->UnlockWithKeyCue, interactingCharacter);
      lockable->Unlock();
   }
   else if (lockContext.CanLockWithoutKey())
   {
      LockConfigHelpers::TryExecuteCueOnActor(settings->LockWithoutKeyCue, interactingCharacter);
      if(interactingCharacter->HasAuthority())
      {
         lockable->Lock();
      }
   }
}

void FTATLockConfig::StartLockpicking(ACharacter* character, TScriptInterface<ILockpickableInterface> target) const
{
   FGameplayEventData eventData;
   eventData.OptionalObject = target.GetObject(); 
   eventData.EventMagnitude = GetFinalLockLevel();

   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   FGameplayTag eventTag = settings.Lockpick;

   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(character, eventTag, eventData);
}

void FTATLockConfig::InitializeAlternateLock(const AActor* owningActor)
{
   check(owningActor);
   AlternateLock = owningActor->FindComponentByInterface(UTATAlternateLockInterface::StaticClass());
}

#if WITH_EDITOR
void FTATLockConfig::CheckForErrors(TFunctionRef<void(FText)> reportError) const
{
   if(MinLockLevel > MaxLockLevel)
   {
      reportError(FText::Format(INVTEXT("MinLockLevel ({0}) > MaxLockLevel ({1})"),
         {FText::AsNumber(MinLockLevel), FText::AsNumber(MaxLockLevel)}));
   } 
}

#endif

#undef LOCTEXT_NAMESPACE
