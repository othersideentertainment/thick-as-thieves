// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include <Engine/DataTable.h>

#include "TATGameplayTagTableRow.generated.h"

/// Data table row that is identified by a gameplay tag, e.g.
/// @code
/// USTRUCT(BlueprintType, meta=(RowNameTag = "DataTag"))
/// struct FTATDataRow : public FTATGameplayTagTableRow
/// {
///    GENERATED_BODY()
///
///    UPROPERTY(EditAnywhere, BlueprintReadOnly)
///    FGameplayTag DataTag;
/// };
/// @endcode
USTRUCT()
struct FTATGameplayTagTableRow : public FTableRowBase
{
   GENERATED_BODY()

#if WITH_EDITOR
   virtual void OnDataTableChanged(const UDataTable* dataTable, const FName rowName) override;
#endif
};
