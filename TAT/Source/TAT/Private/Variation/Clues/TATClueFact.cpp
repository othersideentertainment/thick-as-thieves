// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueFact.h"

// tat
#include "TATClueTextUtils.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawnUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueFact)

namespace FactHelpers
{
   static TArray<FTATClueFactSpec> ConvertFromHandles(const TArray<FDataTableRowHandle>& handles)
   {
      TArray<FTATClueFactSpec> result;
      result.Reserve(handles.Num());
      for(const FDataTableRowHandle& handle : handles)
      {
         if(const FTATClueFactSpec* spec = handle.GetRow<FTATClueFactSpec>(TEXT("ClueFactHandle")))
         {
            result.Add(*spec);
         }
      }
      return result;
   }

   static const FText& GetLocationNameFromContext(const FTATClueContext& context)
   {
      if (const ITATClueLocationInterface* location = context.Location.Get())
      {
         return location->GetClueLocationName();
      }
      return FText::GetEmpty();
   }
}

FTATClueFactThunk::FTATClueFactThunk(const TArray<FTATClueFactSpec>& factSpecs, const FTATClueContext& context)
   : _specs(factSpecs)
   , _locationName(FactHelpers::GetLocationNameFromContext(context))
   , _sourceTag(context.SourceTag)
   , _sourceIndex(context.SourceIndex)
   , _formatParams(context.ExtraFormatParams) 
{
}

FTATClueFactThunk::FTATClueFactThunk(const TArray<FDataTableRowHandle>& factSpecs, const FTATClueContext& context)
   : _specs(FactHelpers::ConvertFromHandles(factSpecs))
   , _locationName(FactHelpers::GetLocationNameFromContext(context))
   , _sourceTag(context.SourceTag)
   , _sourceIndex(context.SourceIndex)
   , _formatParams(context.ExtraFormatParams)
{
}

FGameplayTag FTATClueFactThunk::GetFactTagAt(int32 index) const
{
   check(_specs.IsValidIndex(index));
   return _specs[index].FactTag;
}

FText FTATClueFactThunk::GetFactTextAt(int32 index, UObject* worldContext) const
{
   check(_specs.IsValidIndex(index));

   FTATClueTextFormatContext context;
   context.LocationName = _locationName;
   context.SourceTag = _sourceTag;
   context.ExtraFormatParams = _formatParams.Get();

   return TATClueTextUtils::FormatUsingContext(_specs[index].JournalText, context, worldContext);
}

const FTATClueFactSpec& FTATClueFactThunk::GetFactSpecAt(int32 index) const
{
   check(_specs.IsValidIndex(index));
   return _specs[index];
}

#if WITH_EDITOR
void ClueFactUtils::ValidateFactHandles(TConstArrayView<FDataTableRowHandle> factHandles, TFunctionRef<void(const FText&)> reportError)
{
   for(int i = 0; i < factHandles.Num(); ++i)
   {
      if(factHandles[i].GetRow<FTATClueFactSpec>(TEXT("ValidateClueFacts")) == nullptr)
      {
         reportError(FText::Format(INVTEXT("Invalid clue fact reference at index {0}"), i));
      }
   }
}
#endif

FGameplayTagContainer ClueFactUtils::TagsFromThunk(const FTATClueFactThunk* thunk)
{
   FGameplayTagContainer result;
   if (thunk)
   {
      for (int i = 0; i < thunk->GetFactCount(); ++i)
      {
         result.AddTag(thunk->GetFactTagAt(i));
      }
   }
   return result;
}
