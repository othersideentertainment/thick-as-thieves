// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueLocationInterface.h"

// ue
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueLocationInterface)

const FText& UTATDummyClueLocation::GetClueLocationName() const
{
   return FText::GetEmpty();
}

const FGameplayTag& UTATDummyClueLocation::GetClueLocationTag() const
{
   return FGameplayTag::EmptyTag;
}
