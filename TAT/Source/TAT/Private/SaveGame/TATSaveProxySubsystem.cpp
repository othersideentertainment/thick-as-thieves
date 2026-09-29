// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "SaveGame/TATSaveProxySubsystem.h"

// tat
#include "SaveGame/TATCharacterProgressionViewModel.h"
#include "SaveGame/TATSaveGame.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSaveProxySubsystem)

void UTATSaveProxySubsystem::Deinitialize()
{
   for (auto& it : _characterProgressionProxies)
   {
      if (UTATCharacterProgressionViewModel* proxy = it.Value)
      {
         proxy->Deinitialize();
      }
   }

   Super::Deinitialize();
}

UTATCharacterProgressionViewModel* UTATSaveProxySubsystem::GetCharacterProgressionProxy(FTATCharacterSaveId character)
{
   if (UTATCharacterProgressionViewModel* found = _characterProgressionProxies.FindRef(character))
   {
      return found;
   }

   // Not currently trying to solve flow where this isn't initialized yet
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (saveGame == nullptr)
   {
      return nullptr;
   }

   UTATCharacterProgressionViewModel* proxy = NewObject<UTATCharacterProgressionViewModel>();
   proxy->Initialize(saveGame, character);

   _characterProgressionProxies.Add(character, proxy);
   return proxy;
}
