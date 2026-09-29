// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Unlockables/TATUnlockableContent.h"

//tat
#include "Analytics/TATAnalyticsManager.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUnlockableContent)

namespace UnlockableHelpers
{
   static bool IsUnlockPurchasable(const UTATSaveGame* saveGame, const FTATUnlockableContent& unlockableContent)
   {
      if (saveGame->GetMoney() < unlockableContent.Price)
      {
         return false;
      }

      if (saveGame->GetAvailabilityFromUnlockRequirement(unlockableContent.UnlockRequirement) != ETATFeatureAvailabilityToPlayer::Available)
      {
         return false;
      }

      if(saveGame->HasUnlockedContent(unlockableContent.Tag))
      {
         return false;
      }

      return true;
   }
}

#if WITH_EDITOR
EDataValidationResult UTATUnlockableContentDataAsset::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);
   TArray<FGameplayTag> foundTags;
   for (const FTATUnlockableContent& entry : EntryTable)
   {
      // Entry with invalid tag
      FGameplayTag tag = entry.Tag;
      if (!tag.IsValid())
      {
         context.AddError(FText::FromString(TEXT("Entry with invalid Tag detected!")));
      }
      else
      {
         // Track usage count for all discovered tags
         if (foundTags.Contains(tag))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate entries found with Tag [%s]!")
            , *tag.ToString())));
         }
         else
         {
            foundTags.Add(tag);
         }
      }

      // No texture assigned
      if (entry.Texture.IsNull())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Entry with Tag [%s] with invalid Texture!")
         , *tag.ToString())));
      }
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR



FTATUnlockableContent UTATUnlockableContentDataAsset::GetEntryByTagName(FGameplayTag EntryTag)
{
   for (const FTATUnlockableContent& entry : EntryTable)
   {
      if (entry.Tag == EntryTag)
      {
         return entry;
      }
   }
   
   return FTATUnlockableContent();
}

bool UTATUnlockableContentFunctionLibrary::IsContentUnlockedForLocalPlayer(const UObject* worldContext, FGameplayTag unlockTag)
{
   if (const UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(worldContext))
   {
      return saveGame->HasUnlockedContent(unlockTag);
   }

   return false;
}

bool UTATUnlockableContentFunctionLibrary::IsUnlockPurchasableForLocalPlayer(const UObject* worldContext,
   const FTATUnlockableContent& unlockableContent)
{
   if (const UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(worldContext))
   {
      return UnlockableHelpers::IsUnlockPurchasable(saveGame, unlockableContent);
   }

   return false;
}

void UTATUnlockableContentFunctionLibrary::IsContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete)
{
   onCheckContentUnlockedComplete.ExecuteIfBound(unlockableContent, IsContentUnlockedForLocalPlayer(playerController, unlockableContent.Tag));
}



void UTATUnlockableContentFunctionLibrary::BeginUnlockContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete)
{
   auto trySaveUnlock = [playerController, &unlockableContent]() -> bool
   {
      UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(playerController);
      if (!ensure(saveGame))
      {
         return false;
      }

      if(!UnlockableHelpers::IsUnlockPurchasable(saveGame, unlockableContent))
      {
         return false;
      }

      saveGame->UpdateMoney(-unlockableContent.Price);
      saveGame->AddUnlock(unlockableContent.Tag);
      
      if (playerController)
      {
         if (const UGameInstance* gameInstance = playerController->GetGameInstance())
         {
            if (UTATAnalyticsManager* analyticsManager = gameInstance->GetSubsystem<UTATAnalyticsManager>())
            {
               analyticsManager->HandleContentUnlocked(unlockableContent);
            }
         }
      }
      return true;
   };

   const bool successful = trySaveUnlock();
   onUnlockContentComplete.ExecuteIfBound(unlockableContent, successful);
}
