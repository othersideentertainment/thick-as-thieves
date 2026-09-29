// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Interactables/TATToggleResolver.h"
#include "Quests/TATQuestFormatParamSource.h"

// ue
#include "CoreMinimal.h"

#include "TATShuffledToggleBindings.generated.h"

// A collection of utilities for randomly shuffling target toggles
// between switches or similar, and having clues about them.
//
// It does this by having an asset that can deterministically
// generate the assignments given the map seed, and have the quest
// and actors independently do this.

// Just to hang editor customization on
USTRUCT()
struct FTATShuffledToggleBindingRef
{
   GENERATED_BODY()

   UPROPERTY(EditInstanceOnly)
   FName Identifier;
};

USTRUCT()
struct FTATShuffledToggleTargetRef
{
   GENERATED_BODY()

   UPROPERTY(EditInstanceOnly)
   FName Identifier;
};

USTRUCT()
struct FTATShuffledToggleBinding
{
   GENERATED_BODY()

   // Internal identifier to reference this in other data
   UPROPERTY(EditAnywhere)
   FName Identifier;

   // This display name of the binding to be injected into clues
   UPROPERTY(EditAnywhere)
   FText DisplayName;
   
   bool operator==(const FTATShuffledToggleBindingRef& ref) const
   {
      return Identifier == ref.Identifier;
   }
};

USTRUCT()
struct FTATShuffledToggleTarget
{
   GENERATED_BODY()

   // Internal identifier to reference this in other data
   UPROPERTY(EditAnywhere)
   FName Identifier;

   // The toggle actor that a binding could resolve to
   // NOTE: If the target is in a level instance, it _must_
   //       be in the same level as the actor that will refer to it.
   UPROPERTY(EditAnywhere, meta = (AllowedClasses="/Script/OSEInteraction.OSEToggleInterface"))
   TSoftObjectPtr<AActor> TargetActor;

   bool operator==(const FTATShuffledToggleTargetRef& ref) const
   {
      return Identifier == ref.Identifier;
   }
};

struct FTATShuffledToggleResult
{
   struct FEntry
   {
      uint8 BindingIndex = 0;
      uint8 TargetIndex = 0;
   };

   TArray<FEntry, TInlineAllocator<16>> Selections;

   int32 FindTargetForBindingIndex(int32 inputIndex) const;
   int32 FindBindingForTargetIndex(int32 targetIndex) const;
};

UCLASS()
class TAT_API UTATShuffledToggleSet : public UDataAsset
{
   GENERATED_BODY()


public:
   // The set logical things that will get toggles assigned to them
   // (e.g. switches)
   UPROPERTY(EditAnywhere, meta = (TitleProperty="{Identifier} [{DisplayName}]"))
   TArray<FTATShuffledToggleBinding> Bindings;

   // The possible target toggles
   UPROPERTY(EditAnywhere, meta = (TitleProperty="{Identifier}"))
   TArray<FTATShuffledToggleTarget> Targets;

   FTATShuffledToggleResult ChooseResult(int32 seed) const;

   const FTATShuffledToggleTarget* FindTargetForBinding(const FTATShuffledToggleResult& result, FTATShuffledToggleBindingRef bindingName) const;
   const FTATShuffledToggleBinding* FindBindingForTarget(const FTATShuffledToggleResult& result, FTATShuffledToggleTargetRef targetName) const;
   
};

USTRUCT()
struct FTATShuffledToggleFormatParam
{
   GENERATED_BODY()

   // The Identifier of the Target in the ShuffledToggleSet to find the binding for
   UPROPERTY(EditAnywhere)
   FTATShuffledToggleTargetRef Target;

   // The text replacement key that the binding display name is injected as
   UPROPERTY(EditAnywhere)
   FString TextReplacementKey;
};

// Output format params for shuffle bindings that were assigned to specific targets
// e.g. which switch disables a specific thing
USTRUCT(DisplayName="Shuffled Toggle Binding")
struct FTATQuestFormatParamSource_ShuffledToggleBinding : public FTATQuestFormatParamSource
{
   GENERATED_BODY()

   UPROPERTY(EditInstanceOnly)
   TObjectPtr<UTATShuffledToggleSet> ShuffleSet;

   UPROPERTY(EditInstanceOnly, meta = (TitleProperty = "`{{TextReplacementKey}`} => [Binding for target: {Target}]"))
   TArray<FTATShuffledToggleFormatParam> Params;

   virtual void AddTextReplacement(const FParams& params, TFunctionRef<void (const FString&, const FText&)> addFormatParam) const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const override;
#endif
};

// Find the toggle that is mapped to the specified binding in the shuffled toggle set
USTRUCT(DisplayName = "Shuffled Binding")
struct FTATToggleResolver_ShuffledBinding : public FTATToggleResolver
{
   GENERATED_BODY()
   
   UPROPERTY(EditInstanceOnly)
   TObjectPtr<UTATShuffledToggleSet> ShuffleSet;

   // The Identifier of the binding in the ShuffleSet to get the target for
   UPROPERTY(EditInstanceOnly)
   FTATShuffledToggleBindingRef Binding;

   
   virtual TScriptInterface<IOSEToggleInterface> ResolveToggle(AActor* context) const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const override;
#endif
};
