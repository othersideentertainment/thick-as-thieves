// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
// tat
#include "TATLootTypes.h"
#include "Character/TATCharacterMetadata.h"

class ATATPlayerState;

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATCallingCard.generated.h"

UCLASS()
class TAT_API ATATCallingCard : public AActor
{
   GENERATED_BODY()
   
public:
   // Sets default values for this actor's properties
   ATATCallingCard();

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnCardMeshUpdated();

public:

   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

   UFUNCTION(BlueprintCallable, BlueprintPure, Category="Calling Card Info")
   FText GetCharacterName() const { return _characterName; }
   
   UFUNCTION(BlueprintCallable, BlueprintPure, Category="Calling Card Info")
   TSoftObjectPtr<UTexture2D> GetSymbolTexture() const { return _symbolTexture;}

   UFUNCTION(BlueprintCallable, BlueprintPure, Category="Calling Card Info")
   TSoftObjectPtr<UStaticMesh> GetCardStaticMesh() const {return _cardStaticMesh;};

   void SetDataFromPlayerState(const ATATPlayerState& playerState);

private:

   void _SetCharacterType(const ETATCharacter& characterType);   
   UFUNCTION()
   void _OnRep_CharacterType();
   void _UpdateCharacterType();

   void _SetCardLoadoutTag(const FGameplayTag& loadoutTag);
   UFUNCTION()
   void _OnRep_CardLoadoutTag();
   void _UpdateCardMesh();

   UPROPERTY(ReplicatedUsing = _OnRep_CardLoadoutTag)
   FGameplayTag _cardLoadoutTag;
   UPROPERTY(ReplicatedUsing=_OnRep_CharacterType)
   ETATCharacter _characterType;
   FText _characterName;
   UPROPERTY()
   TSoftObjectPtr<UTexture2D> _symbolTexture;
   UPROPERTY()
   TSoftObjectPtr<UStaticMesh> _cardStaticMesh;
};
