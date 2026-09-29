// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATToolTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolTypes)

#if WITH_EDITOR
void FTATGearMetadataTableRow::OnDataTableChanged(const UDataTable* dataTable, const FName rowName)
{
   OnMetadataDataTableChanged.Broadcast();
}
#endif
