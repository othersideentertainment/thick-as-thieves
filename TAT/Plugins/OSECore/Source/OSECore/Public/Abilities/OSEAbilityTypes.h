// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "OSEAbilityTypes.generated.h"

/** Game-specific override of effect context */
USTRUCT()
struct OSECORE_API FOSEGameplayEffectContext : public FGameplayEffectContext
{
   GENERATED_BODY()

   FOSEGameplayEffectContext()
   {
   }

   FOSEGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
      : Super(InInstigator, InEffectCauser)
   {
   }

   virtual ~FOSEGameplayEffectContext()
   {
   }

   /** Extract context from handle, validating type */
   static FOSEGameplayEffectContext* GetFromHandle(FGameplayEffectContextHandle& handle);
   static const FOSEGameplayEffectContext* GetFromHandle(const FGameplayEffectContextHandle& handle);

   /** Finds the character of the instigator that created this effect */
   class AOSECharacterBase* GetInstigatorCharacter() const;

   /** Setter for Event Tags */
   void SetEventTags(const FGameplayTagContainer& tags) { SupplementalEventTags = tags; };

   /** Gets tags descriptive of the current GameplayEvent, if any. */
   FGameplayTagContainer& GetEventTags() { return SupplementalEventTags; };

   // Overrides
   virtual bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess) override;
   virtual UScriptStruct* GetScriptStruct() const override;
   virtual FGameplayEffectContext* Duplicate() const override;
   virtual FString ToString() const override;
   virtual AActor* GetOriginalInstigator() const override;

protected:

   /** GameplayEffectContexts are passed with GameplayEvents, not (despite the name)
   just GameplayEffects. Rather than coopt some other tag container, this will
   contain tags descriptive of the event in progress in that case. As a container,
   this can contain multiple descriptive tags to supplement the EventTag. */
   UPROPERTY()
   FGameplayTagContainer SupplementalEventTags;
   /* What's this for, you might be asking? The GameplayEvent's EventTag tells you
      what happened (e.g., an Ability failed to activate), while this might answer
      other questions like why it happened (which in this example might be multiple 
      reasons - the FAbilityFailedDelegate gets a _container_ of tags describing this). */

};

template<>
struct TStructOpsTypeTraits< FOSEGameplayEffectContext > : public TStructOpsTypeTraitsBase2< FOSEGameplayEffectContext >
{
   enum
   {
      WithNetSerializer = true,
      WithCopy = true      // Necessary so that TSharedPtr<FHitResult> Data is copied around
   };
};

// Misc shared delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaitTargetEventDelegate, FGameplayTag, eventTag, const FGameplayAbilityTargetDataHandle&, data);
