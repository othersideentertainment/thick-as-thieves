/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#include "Wwise/WwiseProjectDatabaseImpl.h"
#include "Wwise/WwiseTreeItem.h"
#include "Wwise/Ref/WwiseAnyRef.h"
#include "Wwise/WwiseReconcileModule.h"

struct FScopedSlowTask;

#define LOCTEXT_NAMESPACE "AkAudio"

enum class EWwiseReconcileOperationFlags
{
	None = 0,
	Create = 1 << 0,
	UpdateExisting = 1 << 1,
	RenameExisting = 1 << 2,
	Delete = 1 << 3,
	Move = 1 << 4,
	All = Create | UpdateExisting | Delete | RenameExisting | Move
};

ENUM_CLASS_FLAGS(EWwiseReconcileOperationFlags)

struct FWwiseNewAsset
{
	const WwiseAnyRef* WwiseAnyRef = nullptr;
	bool bAssetExists = false;
};

struct FWwiseReconcileItem
{
	EWwiseReconcileOperationFlags OperationRequired = EWwiseReconcileOperationFlags::None;
	FAssetData Asset;
	FWwiseNewAsset WwiseAnyRef;
	FGuid ItemId;
	FString MovedPath;
	bool operator==(const FWwiseReconcileItem& Other) const
	{
		if(ItemId.IsValid())
		{
			return ItemId == Other.ItemId;
		}
		else
		{
			return Asset.AssetName == Other.Asset.AssetName;
		}
	}

	FText GetOperationText() const
	{
		if (EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::UpdateExisting))
		{
			if(EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::RenameExisting))
			{
				if(EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::Move))
				{
					return LOCTEXT("WwiseReconcileUpdateRenameMove", "Update, Rename and Move");
				}
				return LOCTEXT("WwiseReconcileUpdateRename", "Update and Rename");
			}
			if(EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::Move))
			{
				return LOCTEXT("WwiseReconcileUpdateMove", "Update and Move");
			}
		}
		if(EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::RenameExisting))
		{
			if(EnumHasAnyFlags(OperationRequired, EWwiseReconcileOperationFlags::Move))
			{
				return LOCTEXT("WwiseReconcileRenameMove", "Rename and Move");
			}
		}
		switch (OperationRequired)
		{
		case EWwiseReconcileOperationFlags::Create:
			return LOCTEXT("WwiseReconcileCreate", "Create");
		case EWwiseReconcileOperationFlags::UpdateExisting:
			return LOCTEXT("WwiseReconcileUpdate", "Update");
		case EWwiseReconcileOperationFlags::RenameExisting:
			return LOCTEXT("WwiseReconcileRename", "Rename");
		case EWwiseReconcileOperationFlags::Delete:
			return LOCTEXT("WwiseReconcileDelete", "Delete");
		case EWwiseReconcileOperationFlags::Move:
			return LOCTEXT("WwiseReconcileDelete", "Move");
		default:
			break;
		}
		return LOCTEXT("WwiseReconcileEmpty", "");
	}
};

class WWISERECONCILE_API IWwiseReconcile
{
protected:
	TMap<FGuid, FWwiseNewAsset> GuidToWwiseRef;

	TArray<FWwiseReconcileItem> AssetsToUpdate;
	TArray<FAssetData> AssetsToRename;
	TArray<FAssetData> AssetsToDelete;
	TArray<FWwiseReconcileItem> AssetsToMove;
	TArray<FWwiseNewAsset> AssetsToCreate;

	virtual bool IsAssetOutOfDate(const FAssetData& AssetData, const WwiseAnyRef& WwiseRef) = 0;
	virtual void GetAllWwiseRefs() = 0;
	friend class FWwiseReconcileModule;
public:
	IWwiseReconcile() {};
	virtual ~IWwiseReconcile(){};

	inline static IWwiseReconcile* Get()
	{
		if (auto* Module = IWwiseReconcileModule::GetModule())
		{
			return Module->GetReconcile();
		}
		return nullptr;
	}
	static IWwiseReconcile* Instantiate()
	{
		if (auto* Module = IWwiseReconcileModule::GetModule())
		{
			return Module->InstantiateReconcile();
		}
		return nullptr;
	}
	
	virtual FString GetAssetPackagePath(const WwiseAnyRef& WwiseRef) = 0;
	virtual void GetAllAssets(TArray<FWwiseReconcileItem>& ReconcileItems) = 0;
	virtual TArray<FAssetData> CreateAssets(FScopedSlowTask& SlowTask) = 0;
	virtual TArray<FAssetData> UpdateExistingAssets(FScopedSlowTask& SlowTask) = 0;
	virtual void ConvertWwiseItemTypeToReconcileItem(const TArray<TSharedPtr<FWwiseTreeItem>>& InWwiseItems, TArray<FWwiseReconcileItem>& OutReconcileItems, EWwiseReconcileOperationFlags OperationFlags = EWwiseReconcileOperationFlags::All, bool bFirstLevel = true) = 0;
	virtual bool RenameExistingAssets(FScopedSlowTask& SlowTask) = 0;
	virtual int GetNumberOfAssets() = 0;
	virtual int32 DeleteAssets(FScopedSlowTask& SlowTask) = 0;
	virtual int32 MoveAssets(FScopedSlowTask& SlowTask) = 0;
	virtual UClass* GetUClassFromWwiseRefType(WwiseRefType RefType) = 0;
	virtual void GetAssetChanges(TArray<FWwiseReconcileItem>& ReconcileItems, EWwiseReconcileOperationFlags OperationFlags = EWwiseReconcileOperationFlags::All) = 0;
	virtual bool AddToDelete(FWwiseReconcileItem& Item) = 0;
	virtual bool AddToCreate(FWwiseReconcileItem& Item) = 0;
	virtual bool AddToRename(FWwiseReconcileItem& Item) = 0;
	virtual bool AddToUpdate(FWwiseReconcileItem& Item) = 0;
	virtual bool AddToMove(FWwiseReconcileItem& Item) = 0;
	virtual bool ShouldMove(const WwiseAnyRef& Ref, FAssetData InAssetPath, FString& OutNewAssetPath) = 0;
	
	bool ReconcileAssets(EWwiseReconcileOperationFlags OperationFlags = EWwiseReconcileOperationFlags::All);

	DECLARE_MULTICAST_DELEGATE(FOnWwiseReconcileDoneDelegate);
	// Delegate to fire when a Reconcile operation is complete
	FOnWwiseReconcileDoneDelegate OnWwiseAssetsReconciled;
};

#undef LOCTEXT_NAMESPACE