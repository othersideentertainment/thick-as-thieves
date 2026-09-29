// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATShuffledToggleBindings.h"

// tat
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Online/TATGameState.h"

// ose
#include "Interactables/OSEToggleInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATShuffledToggleBindings)

int32 FTATShuffledToggleResult::FindTargetForBindingIndex(int32 inputIndex) const
{
   for(const FEntry& entry : Selections)
   {
      if(entry.BindingIndex == inputIndex)
      {
         return entry.TargetIndex;
      }
   }

   return INDEX_NONE;
}

int32 FTATShuffledToggleResult::FindBindingForTargetIndex(int32 targetIndex) const
{
   for(const FEntry& entry : Selections)
   {
      if(entry.TargetIndex == targetIndex)
      {
         return entry.BindingIndex;
      }
   }
   
   return INDEX_NONE;
}

FTATShuffledToggleResult UTATShuffledToggleSet::ChooseResult(int32 seed) const
{
   using FIndexArray = TArray<uint8, TInlineAllocator<16>>;
   FIndexArray remainingBindings;
   FIndexArray remainingTargets;

   auto fillIndexes = [](FIndexArray& array, int32 count)
   {
      array.Reserve(count);
      for(int i = 0; i < count; i++)
      {
         array.Add(i);
      }
   };

   fillIndexes(remainingBindings, Bindings.Num());
   fillIndexes(remainingTargets, Targets.Num());

   const int32 combinedSeed = SeedHelpers::MakeSeedForName(GetFName(), seed);
   FRandomStream randomStream(combinedSeed);

   FTATShuffledToggleResult result;
   while(!remainingBindings.IsEmpty() && !remainingTargets.IsEmpty())
   {
      const int32 bindingIndexIndex = randomStream.RandHelper(remainingBindings.Num());
      const int32 targetIndexIndex = randomStream.RandHelper(remainingTargets.Num());

      result.Selections.Emplace(
         IntCastChecked<uint8>(remainingBindings[bindingIndexIndex]),
         IntCastChecked<uint8>(remainingTargets[targetIndexIndex])
      );

      remainingBindings.RemoveAtSwap(bindingIndexIndex);
      remainingTargets.RemoveAtSwap(targetIndexIndex);
   }

   return result;
}

const FTATShuffledToggleTarget* UTATShuffledToggleSet::FindTargetForBinding(const FTATShuffledToggleResult& result, FTATShuffledToggleBindingRef bindingName) const
{
   const int32 bindingIndex = Bindings.IndexOfByKey(bindingName);
   if(bindingIndex < 0)
   {
      return nullptr;
   }

   const int32 targetIndex = result.FindTargetForBindingIndex(bindingIndex);
   return Targets.IsValidIndex(targetIndex) ? &Targets[targetIndex] : nullptr;
}

const FTATShuffledToggleBinding* UTATShuffledToggleSet::FindBindingForTarget(const FTATShuffledToggleResult& result, FTATShuffledToggleTargetRef targetName) const
{
   const int32 targetIndex = Targets.IndexOfByKey(targetName);
   if(targetIndex < 0)
   {
      return nullptr;
   }

   const int32 bindingIndex = result.FindTargetForBindingIndex(targetIndex);
   return Bindings.IsValidIndex(bindingIndex) ? &Bindings[bindingIndex] : nullptr;
}

void FTATQuestFormatParamSource_ShuffledToggleBinding::AddTextReplacement(const FParams& params, TFunctionRef<void(const FString&, const FText&)> addFormatParam) const
{
   if(ShuffleSet == nullptr)
   {
      return;
   }
   
   FTATShuffledToggleResult result = ShuffleSet->ChooseResult(params.MapSeed);
   for(const FTATShuffledToggleFormatParam& param : Params)
   {
      if(const FTATShuffledToggleBinding* binding = ShuffleSet->FindBindingForTarget(result, param.Target))
      {
         addFormatParam(param.TextReplacementKey, binding->DisplayName);
      }
   }
}

#if WITH_EDITOR
void FTATQuestFormatParamSource_ShuffledToggleBinding::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ShuffleSet == nullptr)
   {
      reportError(INVTEXT("ShuffleSet is null"));
      return;
   }

   for(int i = 0; i < Params.Num(); ++i)
   {
      const FTATShuffledToggleFormatParam& param = Params[i];
      if(!ShuffleSet->Targets.Contains(param.Target))
      {
         reportError(FText::FormatOrdered(INVTEXT("Params[{0}]: Binding '{1}' is not in ShuffleSet '{2}'"),
            i,
            FText::FromName(param.Target.Identifier),
            FText::FromString(ShuffleSet.GetName())));
      }

      if(param.TextReplacementKey.IsEmpty())
      {
         reportError(FText::FormatOrdered(INVTEXT("Params[{0}]: TextReplacementKey is empty"), i));
      }
   }
}
#endif

TScriptInterface<IOSEToggleInterface> FTATToggleResolver_ShuffledBinding::ResolveToggle(AActor* context) const
{
   check(context);
   if(ShuffleSet == nullptr)
   {
      return nullptr;
   }
   
   // NB: seed guaranteed to be available by BeginPlay
   const int32 mapSeed = context->GetWorld()->GetGameStateChecked<ATATGameState>()->GetMapSeed();
   FTATShuffledToggleResult result = ShuffleSet->ChooseResult(mapSeed);
   const FTATShuffledToggleTarget* target = ShuffleSet->FindTargetForBinding(result, Binding);
   if(target == nullptr)
   {
      return nullptr;
   }
   
   TScriptInterface<IOSEToggleInterface> toggle = target->TargetActor.Get();
   if (toggle == nullptr)
   {
      // SoftObjectPtrs don't actually work for actors in level instances,
      // so if the initial lookup fails, try looking it up via the level of the
      // requesting actor.
      const FSoftObjectPath& targetPath = target->TargetActor.ToSoftObjectPath();
      FSoftObjectPath myPath = context;
      FSoftObjectPath transplantedPath{ myPath.GetAssetPath(), targetPath.GetSubPathString() };

      toggle = transplantedPath.ResolveObject();
   }

   return toggle;
}

#if WITH_EDITOR
void FTATToggleResolver_ShuffledBinding::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ShuffleSet == nullptr)
   {
      reportError(INVTEXT("ShuffleSet is null"));
   }
   else if(!ShuffleSet->Bindings.Contains(Binding))
   {
      reportError(FText::FormatOrdered(INVTEXT("Binding '{0}' is not in ShuffleSet '{1}'"),
         FText::FromName(Binding.Identifier),
         FText::FromString(ShuffleSet.GetName())));
   }
}
#endif
