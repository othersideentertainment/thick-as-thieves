// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "OSEAbilityInfo.generated.h"

class UPaperSprite;

UCLASS(BlueprintType, Abstract, EditInlineNew)
class OSECORE_API UOSEAbilityMetadata : public UObject
{
   GENERATED_BODY()
};

// A strategy object for binding to an alternative source of progress
// Poll-only for now, but could imagine extending, or just wrapping in a task
//
// Meant to be used with CDO
UCLASS(BlueprintType, Abstract)
class OSECORE_API UOSEAbilityProgressSource : public UObject
{
   GENERATED_BODY()


public:

   virtual float GetProgress(const AActor* character) const { unimplemented(); return 0; }

   UFUNCTION(BlueprintCallable, Category = "Abilities")
   static float EvaluateProgress(TSubclassOf<UOSEAbilityProgressSource> source, const AActor* character);
};

//--------------------------------------------------------------------------------------------------
/// Structure defining the user-facing info for an ability
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAbilityInfo
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FText ToolName;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FText ToolDescription;

   // Hierarchical tag usable for sorting & collating Tools
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag ToolCategory;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool HideInUI = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
   TArray<UOSEAbilityMetadata*> AbilityMetadata;
   
   // Hide in UI if not ready
   // TODO: possibly convert to enum if there are multiple mutual exclusive options
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool HideIfNotReady = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> SourceSprite;

   // If this tag is applied by an effect w/ a duration we can pull the cooldown out of it and visualize it in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag CooldownTag;

   // If this tag is applied by an effect w/ a duration we can pull the duration of this tool out of it and visualize it in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag DurationTag;

   // If this tag is applied by an effect w/ a duration we can pull a ready state out of this tool out of it and visualize it in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag ReadyTag;

   // If this tag is applied by an effect that grants stacks we can pull the stack count up and visualize it in the UI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag StackTag;

   // Optional attribute to use as the "duration" if DurationTag is active
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
   FGameplayAttribute DurationAttribute;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
   FGameplayAttribute DurationMaxAttribute;

   // Optional source to use as the "duration" if DurationTag is active
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
   TSubclassOf<UOSEAbilityProgressSource> DurationProgressSource;

   // Used by the UI to data-drive where this ability is located in the HUD
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag HUDPositioningTag;

public:

   template<class T>
   const T* GetAbilityMetadataByClass() const
   {
      for (const UOSEAbilityMetadata* oseAbilityMetadata : AbilityMetadata)
      {
         if (const T* metadata = Cast<T>(oseAbilityMetadata))
         {
            return metadata;
         }
      }
      return nullptr;
   }

   bool operator==(const FOSEAbilityInfo& other) const
   {
      // I guess we're just doing an equality check for everything here since there's no unique part here?
      return ToolName.IdenticalTo(other.ToolName) &&
             ToolDescription.IdenticalTo(other.ToolDescription) &&
             ToolCategory == other.ToolCategory &&
             HideInUI == other.HideInUI &&
             HideIfNotReady == other.HideIfNotReady &&
             SourceSprite == other.SourceSprite &&
             CooldownTag == other.CooldownTag &&
             DurationTag == other.DurationTag &&
             ReadyTag == other.ReadyTag &&
             StackTag == other.StackTag && 
             HUDPositioningTag == other.HUDPositioningTag;
   }
};
