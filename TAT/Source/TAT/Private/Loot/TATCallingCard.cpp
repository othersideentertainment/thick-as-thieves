// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATCallingCard.h"

//tat
#include "Player/TATPlayerState.h"

// ue
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Developer/TATOutfitSettings.h"
#include "CharacterCustomization/TATCharacterOutfits.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCallingCard)

// Sets default values
ATATCallingCard::ATATCallingCard()
{
   // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = false;
}

void ATATCallingCard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_InitialOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _characterType, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _cardLoadoutTag, params);
}

void ATATCallingCard::SetDataFromPlayerState(const ATATPlayerState& playerState)
{
   _SetCharacterType(playerState.GetTATCharacter());

   for (const FTATCharacterLoadoutEntry& loadoutEntry : playerState.GetCurrentOutfitLoadout())
   {
      if (loadoutEntry.OutfitSlot == ETATCharacterOutfitSlot::CallingCard)
      {
         _SetCardLoadoutTag(loadoutEntry.LoadoutTag);
         break;
      }
   }
}

void ATATCallingCard::_SetCharacterType(const ETATCharacter& characterType)
{
   if (_characterType == characterType)
   {
      return;
   }

   _characterType = characterType;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _characterType, this);

   _UpdateCharacterType();
}

void ATATCallingCard::_OnRep_CharacterType()
{
   _UpdateCharacterType();
}

void ATATCallingCard::_UpdateCharacterType()
{
   if (UTATCharactersMetadata* charactersMetadata = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset())
   {
      FTATCharacterMetadata characterMetadata = charactersMetadata->GetCharacterMetadata(_characterType);
      _characterName = characterMetadata.LocalizedName;
      _symbolTexture = characterMetadata.SymbolTexture;
   }
}

void ATATCallingCard::_SetCardLoadoutTag(const FGameplayTag& loadoutTag)
{
   if (_cardLoadoutTag == loadoutTag)
   {
      return;
   }

   _cardLoadoutTag = loadoutTag;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _cardLoadoutTag, this);

   _UpdateCardMesh();
}

void ATATCallingCard::_OnRep_CardLoadoutTag()
{
   _UpdateCardMesh();
}

void ATATCallingCard::_UpdateCardMesh()
{
   // No need to update this on the server
   if (GetNetMode() == NM_DedicatedServer)
   {
      return;
   }

   TSoftObjectPtr<UDataTable> outfitMetadata = UTATOutfitSettings::Get().OutfitMetadataTable;
   TWeakObjectPtr<ATATCallingCard> weakThis(this);

   UAssetManager::GetStreamableManager().RequestAsyncLoad(outfitMetadata.ToSoftObjectPath(), [outfitMetadata, weakThis] 
      {
         ATATCallingCard* self = weakThis.Get();
         if (IsValid(self))
         {
            const FTATOutfitsMetadataTableRow* loadoutMetadata = UTATOutfitSettings::Get().FindOutfitMetadata(self->_cardLoadoutTag, outfitMetadata.Get());

            if (loadoutMetadata != nullptr)
            {
               if (!loadoutMetadata->OutfitStaticMesh.IsNull())
               {
                  self->_cardStaticMesh = loadoutMetadata->OutfitStaticMesh;
                  self->BP_OnCardMeshUpdated();
               }
            }
         }
      });   
}
