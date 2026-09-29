// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Common/TATGameplayTagTableRow.h"

#if WITH_EDITOR
#include <DataTableEditorUtils.h>
#include <GameplayTagContainer.h>
#include <Logging/MessageLog.h>
#include <Logging/StructuredLog.h>
#include <Misc/UObjectToken.h>
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayTagTableRow)

#if WITH_EDITOR
void FTATGameplayTagTableRow::OnDataTableChanged(const UDataTable* dataTable, const FName rowName)
{
   static const FName kRowNameTag("RowNameTag");
   static const FName kInvalidRow("__INVALID__");

   FTATGameplayTagTableRow* row = dataTable->FindRow<FTATGameplayTagTableRow>(rowName, TEXT("OnDataTableChanged"));
   if (!row)
   {
      return;
   }

   const FString& propertyName = dataTable->RowStruct->GetMetaData(kRowNameTag);
   if (propertyName.IsEmpty())
   {
      UE_LOGFMT(LogDataTable, Error, "Data table row type '{StructName}' missing RowNameTag metadata", dataTable->RowStruct->GetName());
      return;
   }

   const FStructProperty* property = FindFProperty<FStructProperty>(dataTable->RowStruct, *propertyName);
   if (!property || !property->Struct->IsChildOf(FGameplayTag::StaticStruct()))
   {
      UE_LOGFMT(LogDataTable, Error, "Data table row type '{StructName}' missing gameplay tag property '{PropertyName}'", dataTable->RowStruct->GetName(), propertyName);
      return;
   }

   const FGameplayTag& rowNameTag = *property->ContainerPtrToValuePtr<FGameplayTag>(row);
   if (rowNameTag.GetTagName() != rowName)
   {
      UDataTable* dataTableMutable = const_cast<UDataTable*>(dataTable);

      FName newRowName = rowNameTag.GetTagName();
      int32 invalidIdx = 0;

      while (!FDataTableEditorUtils::RenameRow(dataTableMutable, rowName, newRowName))
      {
         newRowName = FName(kInvalidRow, invalidIdx);
         invalidIdx += 1;
      }

      FDataTableEditorUtils::SelectRow(dataTable, newRowName);

      if (invalidIdx > 0 && rowNameTag.IsValid())
      {
         FMessageLog messageLog("AssetCheck");

         const FText errorMessage = FText::Format(INVTEXT("Multiple data table row entries for '{0}'"), FText::FromName(rowNameTag.GetTagName()));
         messageLog.Error()->AddToken(FUObjectToken::Create(dataTable))->AddToken(FTextToken::Create(errorMessage));

         messageLog.Open(EMessageSeverity::Error);
      }
   }
}
#endif
