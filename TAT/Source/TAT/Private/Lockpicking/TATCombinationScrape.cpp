// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// tat
#include "Lockpicking/TATCombinationScrape.h"
#include "Lockpicking/TATCombinationLockComponent.h"

// ue
#include "Misc/UObjectToken.h"

namespace CombinationScrape
{
   template<typename TFunc>
   static void ForEachCombinationLock(const UWorld* world, TFunc&& handler)
   {
      for (UTATCombinationLockComponent* comboLock : TObjectRange<UTATCombinationLockComponent>(RF_ClassDefaultObject| RF_MirroredGarbage))
      {
         if (comboLock && comboLock->GetWorld() == world)
         {
            handler(comboLock);
         }
      }
   }
}

#if WITH_EDITOR
TSet<FName> CombinationScrape::GetLockCombinationNamesInWorld(const UWorld* world)
{
   TSet<FName> mapLockComboNames;
   ForEachCombinationLock(world, [&mapLockComboNames](const UTATCombinationLockComponent* comboLock) {
      const FName comboName = comboLock->GetCombinationName();
      if (comboName.IsValid())
      {
         mapLockComboNames.Add(comboName);
      }
   });
   return mapLockComboNames;
}

void CombinationScrape::CheckForDuplicateCombinations(const UWorld* world, FMessageLog& msgLog)
{
   TMap<FName, const UTATCombinationLockComponent*> comboToComponent;
   ForEachCombinationLock(world, [&comboToComponent, &msgLog](const UTATCombinationLockComponent* comboLock) {
      const FName comboName = comboLock->GetCombinationName();
      if (!comboName.IsValid() && comboLock->ShouldEnforceUniqueName())
      {
         return;
      }

      auto makeObjectToken = [](const UTATCombinationLockComponent* comboLock)
      {
         const AActor* owner = comboLock->GetOwner();
         return FUObjectToken::Create(owner, FText::FromString(owner->GetActorNameOrLabel()));
      };

      const UTATCombinationLockComponent* otherComboLock = comboToComponent.FindRef(comboLock->GetCombinationName());
      if (otherComboLock == nullptr)
      {
         comboToComponent.Add(comboName, comboLock);
      }
      else if(otherComboLock != comboLock)
      {
         msgLog.Warning()
            ->AddText(FText::Format(INVTEXT("Duplicate lock combination name: '{0}' used by multiple locks"), FText::FromName(comboName)))
            ->AddToken(makeObjectToken(comboLock))
            ->AddText(INVTEXT("and"))
            ->AddToken(makeObjectToken(otherComboLock))
            ->AddText(INVTEXT(". If you want this combination to be shared, set 'Allow Shared Name' to true"));
      }
   });
}

TArray<UActorComponent*, TInlineAllocator<4>> CombinationScrape::FindLocksWithCombination(const UWorld* world, FName combination)
{
   TArray<UActorComponent*, TInlineAllocator<4>> result;
   ForEachCombinationLock(world, [&result, combination](UTATCombinationLockComponent* comboLock) {
      if (comboLock->GetCombinationName() == combination)
      {
         result.AddUnique(comboLock);
      }
   });

   return result;
}

#endif
