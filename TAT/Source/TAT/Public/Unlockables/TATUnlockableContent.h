// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATSaveGame.h"

// ue
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"


#include "TATUnlockableContent.generated.h"

// Defines the type of content being unlocked so that we may determine how to unlock it
UENUM(BlueprintType)
enum class EUnlockableContentType : uint8
{
   Thief = 0,
   Cosmetic,
   Gear,
   Map,
   Difficulty,
   CallingCard
};

// Defines a visual representation of an unlockable content represented on the black market screen
USTRUCT(BlueprintType)
struct TAT_API FTATUnlockableContent
{
   GENERATED_BODY()

public:
   // Unique identifier to find the entry
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(Categories="UnlockableCategory"))
   FGameplayTag Tag;
   
   // Name to show on the black market entry
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FText DisplayName;
   
   // Description to show on the black market entry
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FText Description;

   // The kind of Content this is
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EUnlockableContentType ContentType = EUnlockableContentType::Thief;

   // Texture to show on the black market entry
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSoftObjectPtr<UTexture2D> Texture;
   
   // The amount of Currency this item will cost in the black market
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   int32 Price = 0;

   // The requirements necessary to be able to unlock this
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FTATUnlockRequirement UnlockRequirement;

   FORCEINLINE bool operator==(FGameplayTag tag) const { return Tag == tag; }
};

// Data asset used to store a tag-identified collection of data used for representing unlockable content
UCLASS(Blueprintable)
class TAT_API UTATUnlockableContentDataAsset : public UDataAsset
{
   GENERATED_BODY()

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:
   // Collection of tag-identified unlockable content representations used to show the associated unlockable content on the black market
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "Tag"))
   TArray<FTATUnlockableContent> EntryTable;
   
   UFUNCTION(BlueprintCallable)
   FTATUnlockableContent GetEntryByTagName(FGameplayTag EntryTag);
};

UCLASS()
class TAT_API UTATUnlockableContentFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnCheckContentUnlockedComplete, const FTATUnlockableContent&, unlockableContent, bool, isContentUnlocked);
   DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnUnlockContentComplete, const FTATUnlockableContent&, unlockableContent, bool, isSuccessful);
   UFUNCTION(BlueprintPure, Category = "UnlockableContent", meta=(WorldContext=worldContext, Categories="UnlockableCategory"))
   static bool IsContentUnlockedForLocalPlayer(const UObject* worldContext, FGameplayTag unlockTag);

   // returns whether the unlock is purchasable
   // NOTE: since there are multiple reasons that this could be false, it may in practice want to be split depending on UI needs
   UFUNCTION(BlueprintCallable, Category = "UnlockableContent", meta=(WorldContext=worldContext))
   static bool IsUnlockPurchasableForLocalPlayer(const UObject* worldContext, const FTATUnlockableContent& unlockableContent);
   
   UFUNCTION(BlueprintCallable, Category = "UnlockableContent")
   static void IsContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsThiefContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsCosmeticContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsGearContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsMapContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsDifficultyContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   static void IsCallingCardContentUnlockedForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnCheckContentUnlockedComplete& onCheckContentUnlockedComplete);
   UFUNCTION(BlueprintCallable, Category = "UnlockableContent")
   static void BeginUnlockContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockThiefContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockCosmeticContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockGearContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockMapContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockDifficultyContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void BeginUnlockCallingCardContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);

private:
   static void UnlockThiefContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void UnlockCosmeticContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void UnlockGearContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void UnlockMapContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void UnlockDifficultyContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
   static void UnlockCallingCardContentForPlayer(APlayerController* playerController, const FTATUnlockableContent& unlockableContent, const FOnUnlockContentComplete& onUnlockContentComplete);
};
