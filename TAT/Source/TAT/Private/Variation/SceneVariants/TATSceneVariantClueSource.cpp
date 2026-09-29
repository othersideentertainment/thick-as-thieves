// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneVariantClueSource.h"

// tat
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATClueSpawnTypes.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

void TATSceneVariantClueSource::AddClueSourcesFromVariants(TConstArrayView<const UTATSceneVariantConfig*> variants,
                                                           TArray<FTATPendingClueSource>& outSources)
{
   for(const UTATSceneVariantConfig* variant : variants)
   {
      check(variant);
      const TSoftObjectPtr<UTATClueSet>& clueSet = variant->GetClueSet();
      if(!clueSet.IsNull())
      {
         // NOTE: TWeakInterfacePtr does not support being initialized with a const uobject, even if it is TWeakInterfacePtr<const ...>
         //       Since ITATClueLocationInterface has a const interface, I have left it non-const, since it still would have
         //       required a const_cast to make it happy. const_cast is safe, if ick, since the source object is not const
         const TWeakInterfacePtr<ITATClueLocationInterface> location = const_cast<UTATSceneVariantConfig*>(variant);
         outSources.Add(FTATPendingClueSource{
            .ClueSet = clueSet,
            .Location = location,
            .SourceTag = FGameplayTag(),
         });
      }
   }
}
