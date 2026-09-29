// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ose
#include "VoiceOver/OSEVoiceOverBase.h"

// ue4
#include "AkGameplayTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"

#include "OSEVoiceOverLine.generated.h"

class UAkComponent;


USTRUCT()
struct FOSEVoiceVerbToCategoryTableRow : public FTableRowBase
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "VoiceVerb"))
   FGameplayTag VoiceVerb;

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "VoiceIdentity"))
   FGameplayTag VoiceIdentity;
};

USTRUCT()
struct FOSEVoiceIdentityInfoTableRow : public FTableRowBase
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "VoiceIdentity"))
   FGameplayTag VoiceIdentity;
};

USTRUCT(BlueprintType)
struct FOSEVoiceOverLineData
{
   GENERATED_BODY()

   // Audio event carrying the VO to play
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TObjectPtr<UAkAudioEvent> AudioEvent = nullptr;

   //TODO: Subtitle FText, animations, etc.
};

USTRUCT(BlueprintType)
struct FOSEVoiceOverLineIdentityData
{
   GENERATED_BODY()

   // The audio event used to play any of the external source lines contained within.
   // TODO: remove once we update all VOLs to use AkEvents rather than external sources
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   UAkAudioEvent* AudioEvent = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = AudioEvent))
   TArray<FOSEVoiceOverLineData> Lines;

   int GetRandomLineIndex(FRandomStream& randomStream);
private:
   // Shuffle this array to randomize which order line gets played in. 
   // Using a shuffled order prevents duplicates playing one after another.
   UPROPERTY(Transient)
   TArray<int32> _lineShuffling;

   UPROPERTY(Transient)
   int _lineIndex = 0;
};

//---------------------------------------------------------------------------------------
/// UOSEVoiceOverLine
///
/// An asset to contain randomly selected lines mapped to a voice identity.
/// Can be sent in a VO Request to the VOController which will select from the contained identities
/// and choose a random line to play from within.
//---------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSECORE_API UOSEVoiceOverLine
   : public UOSEVoiceOverBucketItem
{
   GENERATED_BODY()
public:
   UPROPERTY(BlueprintReadonly, EditAnywhere, meta = (Categories = "VoiceVerb"))
   FGameplayTag VoiceVerb;
   
   UPROPERTY(BlueprintReadonly, EditAnywhere, meta = (Categories = "VoiceIdentity", ForceInlineRow))
   TMap<FGameplayTag, FOSEVoiceOverLineIdentityData> Identities;

   //Chooses a random voice line that's associated with the character. Avoids playing two of the same voice lines in a row.
   FOSEVoiceOverLineData ResolveVoiceLineData(FRandomStream& randomStream, const UAkComponent* voComponent);

   FOSEVoiceOverLineIdentityData* GetIdentityData(const UAkComponent* voComponent);
   const FOSEVoiceOverLineIdentityData* GetIdentityData(const UAkComponent* voComponent) const;

   // Returns true if this VoiceOverLine will work with a given actor.
   UFUNCTION(BlueprintPure)
   bool HasAnyLines(const AActor* actor) const;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
#endif
};

