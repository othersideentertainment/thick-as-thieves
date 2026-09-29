// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "SaveGame/TATCharacterSaveId.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterSaveId)

FString FTATCharacterSaveId::ToString() const
{
   return FString::Printf(TEXT("FTATCharacterSaveId(%s)"),
      *StaticEnum<ETATCharacter>()->GetNameStringByValue((int64)Character));
}
