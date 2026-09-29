// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

/// Mapping from a custom key type (like FGameplayTag) to a data table row name.
/// Intended to support quick data table lookups using any key field in the data table's struct
template<typename RowT, typename KeyT = FGameplayTag>
class TTATDataTableMap
{
public:
   using RowType = RowT;
   using KeyType = KeyT;

private:
   static constexpr const TCHAR* kContextString = TEXT("TTATDataTableMap");

   TMap<KeyType, FName> _keyToRowNameMap;

public:
   TTATDataTableMap() = default;
   TTATDataTableMap(const UDataTable* dataTable, TFunctionRef<KeyType(const RowType&)> rowToKeyFunc) { Reset(dataTable, rowToKeyFunc); }
   TTATDataTableMap(const TTATDataTableMap& rhs) = default;
   TTATDataTableMap(TTATDataTableMap&& rhs) noexcept = default;

   /// Gets the key-to-name map
   FORCEINLINE const TMap<KeyType, FName>& GetKeyToRowNameMap() const
   {
      return _keyToRowNameMap;
   }

   /// Gets the row count
   FORCEINLINE int32 Num() const
   {
      return _keyToRowNameMap.Num();
   }

   /// Checks if the key to row name map contains the specified key
   FORCEINLINE bool Contains(const KeyType& key) const
   {
      return _keyToRowNameMap.Contains(key);
   }

   /// Checks if the key to row name map contains the specified key, and also that the key maps to a valid entry in the data table
   bool Contains(const UDataTable* dataTable, const KeyType& key) const
   {
      if (dataTable != nullptr)
      {
         if (const FName* rowName = _keyToRowNameMap.Find(key))
         {
            return dataTable->GetRowMap().Contains(*rowName);
         }
      }
      return false;
   }

   /// Finds a data table row by key, returning null if the data table has no such key
   const RowType* Find(const UDataTable* dataTable, const KeyType& key) const
   {
      if (!_IsDataTableValid(dataTable))
      {
         return nullptr;
      }
      if (const FName* rowName = _keyToRowNameMap.Find(key))
      {
         constexpr bool warnIfMissing = false;
         return dataTable->FindRow<RowType>(*rowName, kContextString, warnIfMissing);
      }
      return nullptr;
   }

   /// Finds a data table row by calling the predicate function with each key/row pair, returning a pointer to the first row where the predicate function returned true
   const RowType* FindByPredicate(const UDataTable* dataTable, TFunctionRef<bool(const KeyType&, const RowType&)> predicate) const
   {
      if (!_IsDataTableValid(dataTable))
      {
         return nullptr;
      }
      for (const auto& pair : _keyToRowNameMap)
      {
         constexpr bool warnIfMissing = false;
         if (const RowType* row = dataTable->FindRow<RowType>(pair.Value, kContextString, warnIfMissing))
         {
            if (predicate(pair.Key, *row))
            {
               return row;
            }
         }
      }
      return nullptr;
   }

   /// Returns a reference to the data table row with the specified key. Asserts on failure.
   const RowType& GetRefChecked(const UDataTable* dataTable, const KeyType& key) const
   {
      check(dataTable != nullptr);
      _CheckDataTableRowTypeMatches(dataTable);
      const FName* rowName = _keyToRowNameMap.Find(key);
      check(rowName != nullptr);
      constexpr bool warnIfMissing = false;
      const RowType* row = dataTable->FindRow<RowType>(*rowName, kContextString, warnIfMissing);
      checkf(row != nullptr, TEXT("Failed to row with name '%s' in data table %s"), *rowName->ToString(), *dataTable->GetName());
      return *row;
   }

   /// Calls a callback for each row in the data table
   void ForeachRow(const UDataTable* dataTable, TFunctionRef<void(const KeyType&, const RowType&)> callback) const
   {
      if (!_IsDataTableValid(dataTable))
      {
         return;
      }
      for (const auto& pair : _keyToRowNameMap)
      {
         constexpr bool warnIfMissing = false;
         if (const RowType* row = dataTable->FindRow<RowType>(pair.Value, kContextString, warnIfMissing))
         {
            callback(pair.Key, *row);
         }
      }
   }

   /// Rebuilds the key to row name map using the specified data table
   void Reset(const UDataTable* dataTable, TFunctionRef<KeyType(const RowType&)> rowToKeyFunc)
   {
      _keyToRowNameMap.Reset();
      if (_IsDataTableValid(dataTable))
      {
         _keyToRowNameMap.Reserve(dataTable->GetRowMap().Num());
         dataTable->ForeachRow<RowType>(kContextString, [this, &rowToKeyFunc](const FName& rowName, const RowType& rowValue)
         {
            _keyToRowNameMap.Add(rowToKeyFunc(rowValue), rowName);
         });
      }
   }

private:
   static void _CheckDataTableRowTypeMatches(const UDataTable* dataTable)
   {
      check(dataTable != nullptr);
      check(dataTable->RowStruct != nullptr);
      checkf(dataTable->RowStruct->IsChildOf(RowType::StaticStruct()),
         TEXT("Expected data table with row type %s, got row type %s"), *RowType::StaticStruct()->GetName(), *dataTable->RowStruct->GetName());
   }

   static bool _IsDataTableValid(const UDataTable* dataTable)
   {
      if (dataTable == nullptr)
      {
         return false;
      }
      _CheckDataTableRowTypeMatches(dataTable);
      return true;
   }
};
