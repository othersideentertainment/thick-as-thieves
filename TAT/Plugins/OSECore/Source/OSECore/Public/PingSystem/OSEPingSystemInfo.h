// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "GameplayTagContainer.h"
#include "Engine/CollisionProfile.h"

#include "OSEPingSystemInfo.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSEPingSystem, Log, All);

class AOSEPingActor;
class UPaperSprite;
class UTexture2D;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPingInfo
{
   GENERATED_BODY()

   FOSEPingInfo();

   // Name to display in the UI
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FText Name;

   // Text to display in the event highway
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FText EventHighwayText;
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping Info", meta = (InlineEditConditionToggle))
   bool UseSprite = true;

   // Icon to display in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping Info", meta = (DisplayThumbnail = "true"), meta = (EditCondition = "UseSprite"))
   UPaperSprite* SourceSprite = nullptr;

   // Material to display in the UI and/or spawn as part of a decal
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping Info", meta = (DisplayThumbnail = "true"), meta = (EditCondition = "!UseSprite"))
   UTexture2D* SourceTexture = nullptr;

   // Optional material for things like decals
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping Info", meta = (DisplayThumbnail = "true"))
   UMaterialInterface* Material = nullptr;

   // Audio switch for this ping
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FString AudioSwitch;

   // Color of the UI widget
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FLinearColor Color;

   // Are we allowed to focus this ping?  Sprays don't have responses, so there's no need to focus, for instance...
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   bool AllowFocus = true;

   // If another player drops this ping, what are my available responses to it?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info", meta = (EditCondition = "AllowFocus"))
   TArray<FGameplayTag> Responses;

   // If another player drops this ping, what is my default response if I click on it?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info", meta = (EditCondition = "AllowFocus"))
   FGameplayTag DefaultResponse;

   // Do we show this ping in the radial menu when there is no context?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   bool ShowInMenu = false;

   // How many pings of this type do we want to persist in the world?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"), Category = "Ping Info")
   int NumSimultaneousAllowed = 1;

   // Override the default ping actor class for this ping type
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   TSubclassOf<AOSEPingActor> OverridePingActorClass;

   // Which trace profile should we use when specifically attempting to use this ping?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FCollisionProfileName TraceProfile;

   // What is the max range we should use when specifically attempting to use this ping?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   float MaxRange = 9999999.0f;

   // What's the priority of this tag - in a situation where multiple different tag types are possible, the highest priority will be the default
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   int Priority { 1 };
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPingResponseInfo
{
   GENERATED_BODY()

   // Name to display in the UI
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FText Name;

   // Text to display in the event highway
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FText EventHighwayText;
   
   // Icon to display in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping Info", meta = (DisplayThumbnail = "true"))
   UPaperSprite* SourceSprite = nullptr;

   // Audio switch for this ping
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ping Info")
   FString AudioSwitch;
};

UCLASS(BlueprintType, Abstract, EditInlineNew)
class OSECORE_API UOSEPingPageRequirement : public UObject
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   virtual bool EvaluateRequirement(const APlayerController* playerController) const { return false; };
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPingPage
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   TMap<FGameplayTag, FOSEPingInfo> SprayInfo;

   // Requirements for this page to be displayed in the radial menu. Empty array = always allowed.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Ping Info")
   TArray<UOSEPingPageRequirement*> PageRequirements;
};

UCLASS(BlueprintType)
class OSECORE_API UOSEPingSystemInfoAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   FGameplayTag DefaultSingleInputPingTag;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   FGameplayTag DefaultDoubleInputPingTag;

   UPROPERTY(EditAnywhere, Category = "Loading")
   TSubclassOf<AOSEPingActor> DefaultPingActorClass;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System", meta = (Categories = "PingSystem.Context"))
   TMap<FGameplayTag, FOSEPingInfo> PingInfo;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System", meta = (Categories = "PingSystem.Context"))
   TArray<FOSEPingPage> SprayPages;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System", meta = (Categories = "PingSystem.Context"))
   TMap<FGameplayTag, FOSEPingResponseInfo> PingResponseInfo;   

   // hard refs to the sprite textures used for pings so they're loaded along w/ this asset.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   TArray<UTexture2D*> SpriteTextures;

   UFUNCTION(BlueprintPure, Category = "Ping System")
   void FindPingInfoFromPingTag(FGameplayTag pingTag, FOSEPingInfo& outPingInfo, bool& found) const;
   const FOSEPingInfo* FindPingInfoFromPingTag(const FGameplayTag& pingTag) const;

   UFUNCTION(BlueprintPure, Category = "Ping System")
   void FindPingResponseInfoFromResponseTag(FGameplayTag responseTag, FOSEPingResponseInfo& outPingResponseInfo, bool& found) const;
   const FOSEPingResponseInfo* FindPingResponseInfoFromResponseTag(const FGameplayTag& responseTag) const;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
#endif

private:

};
