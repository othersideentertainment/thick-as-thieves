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

#include "WwiseBrowserDataSource.h"

#include <Wwise/Stats/AudiokineticTools.h>

#include "AkUnrealAssetDataHelper.h"
#include "AkWaapiUtils.h"
#include "WaapiDataSource.h"
#include "WwiseProjectDatabaseSource.h"
#include "Templates/SharedPointer.h"
#include "Wwise/WwiseTreeItem.h"
#include "IAudiokineticTools.h"
#include "AssetManagement/AkAssetDatabase.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseReconcile.h"
#include "WwiseUnrealHelper.h"

bool FSoundBankStatusFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if (AreFiltersOff())
	{
		return true;
	}
	if (bFilters[NotInWwise] && Item->IsNotInWwiseOrSoundBank())
	{
		return true;
	}

	if (bFilters[DeletedInWwise] && Item->IsDeletedInWwise())
	{
		return true;
	}

	if (bFilters[NewInWwise] && Item->IsNewInWwise())
	{
		return true;
	}

	if (bFilters[RenamedInWwise] && Item->IsRenamedInWwise())
	{
		return true;
	}

	if (bFilters[MovedInWwise] && Item->IsMovedInWwise())
	{
		return true;
	}

	if (bFilters[UpToDate] && Item->IsSoundBankUpToDate())
	{
		return true;
	}

	return false;
}

bool FSoundBankStatusFilter::AreFiltersOff() const
{
	for (auto filter : bFilters)
	{
		if (filter)
		{
			return false;
		}
	}
	return true;
}

bool FUAssetStatusFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if (AreFiltersOff())
	{
		return true;
	}
	if(bFilters[UAssetMissing] && Item->IsUAssetMissing())
	{
		return true;
	}

	if(bFilters[UAssetOrphaned] && Item->IsUAssetOrphaned())
	{
		return true;
	}

	if(bFilters[NotInSoundBankOrUnreal] && Item->IsNotInSoundBankOrUnreal())
	{
		return true;
	}

	if(bFilters[MultipleUAssets] && Item->HasMultipleUAssets())
	{
		return true;
	}

	if (bFilters[RenamedInSoundBank] && Item->IsRenamedInSoundBank())
	{
		return true;
	}

	if (bFilters[UAssetNeedsUpdate] && Item->IsUAssetOutOfDate())
	{
		return true;
	}

	if(bFilters[UAssetUpToDate] && Item->IsUAssetUpToDate())
	{
		return true;
	}

	return false;
}

bool FUAssetStatusFilter::AreFiltersOff() const
{
	for (auto filter : bFilters)
	{
		if (filter)
		{
			return false;
		}
	}
	return true;
}

bool FWwiseTypeFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if(AreFiltersOff())
	{
		return true;
	}
	for(int i = 0; i < NumberOfWwiseTypes; i++)
	{
		if(bFilters[i] && Item->ItemType == GetExpectedType((EWwiseTypeFilter)i))
		{
			return true;
		}
	}
	return false;
}

bool FWwiseTypeFilter::AreFiltersOff() const
{
	for(auto bFilter : bFilters)
	{
		if(bFilter)
		{
			return false;
		}
	}
	return true;
}

EWwiseItemType::Type FWwiseTypeFilter::GetExpectedType(EWwiseTypeFilter Filter) const
{
	switch(Filter)
	{
	case AcousticTexture:
		return EWwiseItemType::AcousticTexture;
	case AudioDeviceShareSet:
		return EWwiseItemType::AudioDeviceShareSet;
	case Effects:
		return EWwiseItemType::EffectShareSet;
	case Events:
		return EWwiseItemType::Event;
	case GameParameters:
		return EWwiseItemType::GameParameter;
	case MasterMixerHierarchy:
		return EWwiseItemType::AuxBus;
	case State:
		return EWwiseItemType::State;
	case Switch:
		return EWwiseItemType::Switch;
	case Trigger:
		return EWwiseItemType::Trigger;
	default:
		break;
	}
	return EWwiseItemType::None;
}

FWwiseBrowserDataSource::FWwiseBrowserDataSource()
{
	WaapiDataSource = MakeUnique<FWaapiDataSource>();
	ProjectDBDataSource = MakeUnique<FWwiseProjectDatabaseDataSource>();
	UAssetDataSource = MakeUnique<FUAssetDataSource>();

	WaapiDataSource->Init();
	WaapiDataSource->WaapiDataSourceRefreshed.BindRaw(this, &FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed);
	WaapiDataSource->WwiseSelectionChange.BindRaw(this, &FWwiseBrowserDataSource::OnWwiseSelectionChange);
	WaapiDataSource->WwiseExpansionChange.BindRaw(this, &FWwiseBrowserDataSource::OnWwiseExpansionChange);

	ProjectDBDataSource->Init();
	ProjectDBDataSource->ProjectDatabaseDataSourceRefreshed.BindRaw(this, &FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed);

	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	auto& AssetRegistry = AssetRegistryModule->Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		OnFilesLoaded = AssetRegistry.OnFilesLoaded().AddRaw(this, &FWwiseBrowserDataSource::OnFilesFullyLoaded);
	}
	else
	{
		SetupAssetCallbacks();
	}
}

FWwiseBrowserDataSource::~FWwiseBrowserDataSource()
{
	WaapiDataSource->WaapiDataSourceRefreshed.Unbind();
	WaapiDataSource->WwiseSelectionChange.Unbind();
	WaapiDataSource->WwiseExpansionChange.Unbind();

	ProjectDBDataSource->ProjectDatabaseDataSourceRefreshed.Unbind();

	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	auto& AssetRegistry = AssetRegistryModule->Get();

	AssetRegistry.OnAssetAdded().Remove(OnAssetAdded);
	AssetRegistry.OnAssetRemoved().Remove(OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().Remove(OnAssetRenamed);
	AssetRegistry.OnAssetUpdated().Remove(OnAssetUpdated);
	AssetRegistry.OnFilesLoaded().Remove(OnFilesLoaded);

	if (PostEditorTickHandle.IsValid())
	{
		GEngine->OnPostEditorTick().Remove(PostEditorTickHandle);
		PostEditorTickHandle.Reset();
	}
}

void FWwiseBrowserDataSource::ConstructTree()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_2(TEXT("FWwiseBrowserDataSource::ConstructTree"))

	auto ConstructTask = [this]
	{
		if(ProjectDBDataSource->ConstructTree(false))
		{
			UAssetDataSource->ConstructItems();
			WaapiDataSource->ConstructTree(false);
			MergeDataSources();
		}
	};

	WwiseUnrealHelper::RunTaskInGameThread(ConstructTask);

}

bool FWwiseBrowserDataSource::AreFiltersOff()
{
	return WwiseTypeFilter.AreFiltersOff() &&
		UAssetStatusFilter.AreFiltersOff() &&
		SoundBankStatusFilter.AreFiltersOff();
}

void FWwiseBrowserDataSource::ApplyTextFilter(TSharedPtr<StringFilter> FilterText)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::ApplyTextFilter"))

	CurrentFilterText = FilterText;
	ConstructTree();
	if (!AreFiltersOff())
	{
		ApplyFilter(SoundBankStatusFilter, UAssetStatusFilter, WwiseTypeFilter);
	}

}

void FWwiseBrowserDataSource::ApplyFilter(FSoundBankStatusFilter SoundBankStatus,
										FUAssetStatusFilter UAssetStatus,
										FWwiseTypeFilter WwiseType)
{
	SoundBankStatusFilter = SoundBankStatus;
	UAssetStatusFilter = UAssetStatus;
	WwiseTypeFilter = WwiseType;

	RootItems.Empty();
	for (auto& RootItem : RootItemsUnfiltered)
	{
		FWwiseTreeItemPtr RootItemFiltered;
		ApplyFilter(RootItem, RootItemFiltered);
		RootItems.Add(RootItemFiltered);
	}
}

void FWwiseBrowserDataSource::ApplyFilter(FWwiseTreeItemPtr Item, FWwiseTreeItemPtr& OutItem)
{
	OutItem = MakeShared<FWwiseTreeItem>(*Item.Get());
	OutItem->EmptyChildren();
	for (auto CurrItem : Item->GetChildren())
	{
		if (!IsKeptInBrowser(CurrItem))
		{
			continue;
		}
		if (CurrItem->IsFolder())
		{
			FWwiseTreeItemPtr Children;
			ApplyFilter(CurrItem, Children);
			OutItem->AddChild(Children);
		}
		else
		{
			FWwiseTreeItemPtr Children = MakeShared<FWwiseTreeItem>(*CurrItem.Get());
			OutItem->AddChild(Children);
		}
	}
}

void FWwiseBrowserDataSource::ClearFilter()
{
	CurrentFilterText = {};
}

bool FWwiseBrowserDataSource::IsKeptInBrowser(FWwiseTreeItemPtr Item)
{
	if(AreFiltersOff())
	{
		return true;
	}

	if (Item->IsFolder())
	{
		for (auto Children : Item->GetChildren())
		{
			if (IsKeptInBrowser(Children))
			{
				return true;
			}
		}
		return false;
	}

	bool bWwiseFilterResult = true;
	if (GetWaapiConnectionStatus() == EWwiseConnectionStatus::Connected)
	{
		bWwiseFilterResult = SoundBankStatusFilter.IsKeptInBrowser(Item);
	}

	return  bWwiseFilterResult &&
		UAssetStatusFilter.IsKeptInBrowser(Item) &&
		WwiseTypeFilter.IsKeptInBrowser(Item);
}

FWwiseTreeItemPtr FWwiseBrowserDataSource::GetTreeRootForType(EWwiseItemType::Type ItemType, const FString& FilterText)
{
	FScopeLock AutoLock(&RootItemsLock);

	if (RootItemsUnfiltered.Num() < ItemType + 1)
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to get Item Type. Index out of range."));
		return MakeShared<FWwiseTreeItem>("", "", nullptr, EWwiseItemType::Type::None, FGuid());
	}
	if (AreFiltersOff())
	{
		return RootItemsUnfiltered[ItemType];
	}
	return RootItems[ItemType];
}

FText FWwiseBrowserDataSource::GetProjectName()
{
	if (ProjectDBDataSource.IsValid())
	{
		return ProjectDBDataSource->GetProjectName();
	}
	return {};
}

FText FWwiseBrowserDataSource::GetConnectedWwiseProjectName()
{
	return FText::FromString(WaapiDataSource->LoadProjectName());
}

int32 FWwiseBrowserDataSource::LoadChildren(FWwiseTreeItemPtr TreeItem, TArray<FWwiseTreeItemPtr>& OutChildren)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::LoadChildren"))

	UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Loading children for %s"), *TreeItem->FolderPath )

	TArray<FWwiseTreeItemPtr> WaapiChildren;
	TArray<FWwiseTreeItemPtr> SoundBanksChildren;

	FWwiseTreeItemPtr WaapiParent = WaapiDataSource->FindItemFromPath(TreeItem->FolderPath);
	FWwiseTreeItemPtr SoundBanksParent = ProjectDBDataSource->FindItemFromPath(TreeItem->FolderPath);

	WaapiDataSource->LoadChildren(WaapiParent);
	ProjectDBDataSource->LoadChildren(SoundBanksParent);

	//In this case, it is an orphaned uasset.
	if(!WaapiParent && !SoundBanksParent)
	{
		return TreeItem->GetChildren().Num();
	}
	TreeItem->EmptyChildren();

	if(SoundBanksParent || WaapiParent)
	{
		CreateUnifiedTree(SoundBanksParent, WaapiParent, TreeItem);

		TreeItem->SortChildren();

		return TreeItem->GetChildren().Num();
	}

	return 0;
}

void FWwiseBrowserDataSource::ClearEmptyChildren(FWwiseTreeItemPtr TreeItem)
{

	TArray<FWwiseTreeItemPtr> ChildrenToRemove;

	for (auto Child : *TreeItem->GetChildrenMutable())
	{
		if (Child->IsOfType({EWwiseItemType::None}))
		{
			ChildrenToRemove.Add(Child);
		}
	}

	TreeItem->RemoveChildren(ChildrenToRemove);
}

EWwiseConnectionStatus FWwiseBrowserDataSource::GetWaapiConnectionStatus() const
{
	if(!WaapiDataSource.IsValid())
	{
		return EWwiseConnectionStatus::WwiseNotOpen;
	}
	return WaapiDataSource->IsProjectLoaded();
}

FString FWwiseBrowserDataSource::GetItemWorkUnitPath(FWwiseTreeItemPtr InTreeItem)
{
	if (WaapiDataSource.IsValid())
	{
		return WaapiDataSource->GetItemWorkUnitPath(InTreeItem);
	}
	return {};
}

void FWwiseBrowserDataSource::SelectInWwiseProjectExplorer(TArray<FWwiseTreeItemPtr>& InTreeItem)
{
	if (WaapiDataSource.IsValid())
	{
		WaapiDataSource->SelectInProjectExplorer(InTreeItem);
	}
}

void FWwiseBrowserDataSource::HandleFindWwiseItemInProjectExplorerCommandExecute(
	const TArray<FWwiseTreeItemPtr>& SelectedItems) const
{
	WaapiDataSource->HandleFindWwiseItemInProjectExplorerCommandExecute(SelectedItems);
}

void FWwiseBrowserDataSource::MergeDataSources()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::MergeDataSources"))

	FWwiseTreeItemPtr WaapiRootItem;
	FWwiseTreeItemPtr SoundBanksRootItem;

	{
		FScopeLock AutoLock(&RootItemsLock);
		RootItemsUnfiltered.Empty();
	}

	for (int i = EWwiseItemType::Event; i <= EWwiseItemType::LastWwiseBrowserType; ++i)
	{

		if (!CurrentFilterText.IsValid() || CurrentFilterText->GetRawFilterText().IsEmpty())
		{
			SoundBanksRootItem = ProjectDBDataSource->GetRootItem(static_cast<EWwiseItemType::Type>(i));
		}

		else
		{
			SoundBanksRootItem = ProjectDBDataSource->LoadFilteredRootItem(static_cast<EWwiseItemType::Type>(i), CurrentFilterText);
		}

		FWwiseTreeItemPtr NewRootItem;

		if (SoundBanksRootItem)
		{
			NewRootItem = MakeShared<FWwiseTreeItem>(SoundBanksRootItem->DisplayName, SoundBanksRootItem->FolderPath, nullptr, SoundBanksRootItem->ItemType, SoundBanksRootItem->ItemId, SoundBanksRootItem->ShortId);
			NewRootItem->WwiseItemRef = SoundBanksRootItem->WwiseItemRef;
			UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Creating Tree for %s "), *SoundBanksRootItem->FolderPath)
		}

		if (!CurrentFilterText.IsValid() || CurrentFilterText->GetRawFilterText().IsEmpty())
		{
			WaapiRootItem = WaapiDataSource->GetRootItem(static_cast<EWwiseItemType::Type>(i));
		}

		else
		{
			WaapiRootItem = WaapiDataSource->LoadFilteredRootItem(static_cast<EWwiseItemType::Type>(i), CurrentFilterText);
		}


		if (WaapiRootItem)
		{
			if (!NewRootItem)
			{
				NewRootItem = MakeShared<FWwiseTreeItem>(WaapiRootItem->DisplayName, WaapiRootItem->FolderPath, nullptr, WaapiRootItem->ItemType, WaapiRootItem->ItemId, WaapiRootItem->ShortId);
				NewRootItem->WwiseItemRef = WaapiRootItem->WwiseItemRef;
			}

			NewRootItem->ItemId = WaapiRootItem->ItemId;
		}

		if(SoundBanksRootItem || WaapiRootItem)
		{
			CreateUnifiedTree(SoundBanksRootItem, WaapiRootItem, NewRootItem);
			UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Merging Tree for %s "), *NewRootItem->FolderPath);

			FScopeLock AutoLock(&RootItemsLock);
			RootItemsUnfiltered.Add(NewRootItem);
		}
	}

	// Add the Orphan UAssets
	{
		FScopeLock AutoLock(&RootItemsLock);
		FWwiseTreeItemPtr RootItem = MakeShared<FWwiseTreeItem>(FString("Orphaned UAssets"), FString("\\Orphaned UAssets"), nullptr, EWwiseItemType::Folder, FGuid());
		TArray<UAssetDataSourceInformation> OrphanAssets;
		UAssetDataSource->GetOrphanAssets(OrphanAssets);
		for(auto& OrphanAsset : OrphanAssets)
		{
			//Check if they should be filtered out by the text
			if (CurrentFilterText.IsValid() && !CurrentFilterText->GetRawFilterText().IsEmpty())
			{
				if (!OrphanAsset.Id.Name.ToString().Contains(CurrentFilterText->GetRawFilterText().ToString()))
				{
					continue;
				}
			}
			FWwiseTreeItemPtr NewItem = MakeShared<FWwiseTreeItem>(OrphanAsset.Id.Name.ToString(), FString(), RootItem, OrphanAsset.Type, FGuid());
			NewItem->Assets = OrphanAsset.AssetsData;
			NewItem->UAssetName = OrphanAsset.Id.Name;
			NewItem->ShortId = OrphanAsset.Id.ShortId;
			RootItem->AddChild(NewItem);

		}
		RootItemsUnfiltered.Add(RootItem);
	}


	WwiseBrowserDataSourceRefreshed.ExecuteIfBound();

}

void FWwiseBrowserDataSource::CreateUnifiedTree(const FWwiseTreeItemPtr& TreeItemRootSoundBank, const FWwiseTreeItemPtr& TreeItemRootWaapi, FWwiseTreeItemPtr& TreeItemRootDst)
{
	if (TreeItemRootSoundBank)
	{
		CreateProjectDBItem(TreeItemRootSoundBank, TreeItemRootDst);
		if (TreeItemRootWaapi)
		{
			CreateWaapiItem(TreeItemRootWaapi, TreeItemRootDst);
		}
	}
}

void FWwiseBrowserDataSource::CreateProjectDBItem(const FWwiseTreeItemPtr& TreeItemRootSoundBank, FWwiseTreeItemPtr& TreeItemRootDst)
{
	FString DefaultAssetName = TreeItemRootSoundBank->GetDefaultAssetName();
	TreeItemRootDst = MakeShared<FWwiseTreeItem>(TreeItemRootSoundBank->DisplayName, TreeItemRootSoundBank->FolderPath, nullptr, TreeItemRootSoundBank->ItemType, TreeItemRootSoundBank->ItemId, TreeItemRootSoundBank->ShortId);
	if(TreeItemRootSoundBank->ShouldDisplayInfo())
	{
		TArray<FAssetData> Assets;
		EWwiseItemType::Type Type;
		FName UAssetName;
		UAssetDataSource->GetAssetsInfo(TreeItemRootSoundBank->ItemId, TreeItemRootSoundBank->ShortId, DefaultAssetName, TreeItemRootSoundBank->GroupId, Type, UAssetName, Assets);
		TreeItemRootDst->Assets = Assets;
		TreeItemRootDst->UAssetName = UAssetName;
		TreeItemRootDst->bIsInWrongLocation = IsMoved(TreeItemRootDst);
	}
	TreeItemRootDst->WwiseItemRef = TreeItemRootSoundBank->WwiseItemRef;
	//State Groups have a None State that does not appear in Wwise. Set the Ref to true to avoid "Deleted in Wwise".
	if(TreeItemRootDst->ItemType == EWwiseItemType::State && TreeItemRootDst->DisplayName == FString("None"))
	{
		TreeItemRootDst->WaapiName = TreeItemRootDst->DisplayName;
		TreeItemRootDst->bWaapiRefExists = true;
	}
	for (auto& Child : TreeItemRootSoundBank->GetChildren())
	{
		FWwiseTreeItemPtr NewItem;
		CreateProjectDBItem(Child, NewItem);
		TreeItemRootDst->AddChild(NewItem);
	}
	TreeItemRootDst->SortChildren();
}

bool FWwiseBrowserDataSource::IsMoved(FWwiseTreeItemPtr CurrItem)
{
	if(!CurrItem->HasUniqueUAsset())
	{
		return false;
	}
	auto WwiseReconcile = IWwiseReconcile::Get();
	auto ProjectDatabase = FWwiseProjectDatabase::Get();
	if(UNLIKELY(!WwiseReconcile))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Could not load Wwise Reconcile Module."));
		return false;
	}
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Could not load project database"));
		return false;
	}
	FString Out;
	WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	WwiseAnyRef* Ref = nullptr;
	WwiseDBArray<WwiseAnyRef> Refs;
	WwiseDBGuid Guid = WwiseDBGuid(CurrItem->ItemId.A, CurrItem->ItemId.B, CurrItem->ItemId.C, CurrItem->ItemId.D);
	DataStructure.GetCurrentPlatformData()->Guids.MultiFind(WwiseDatabaseLocalizableGuidKey(Guid, 0), Refs);
	for(auto& InRef : Refs)
	{
		if(InRef.GetType() != WwiseRefType::SoundBank)
		{
			Ref = &InRef;
			break;
		}
	}
	return  Ref && WwiseReconcile->Get()->ShouldMove(*Ref, CurrItem->Assets[0], Out);
}

void FWwiseBrowserDataSource::CreateWaapiExclusiveItem(const FWwiseTreeItemPtr& WaapiItem, FWwiseTreeItemPtr& TreeItemRootDst)
{
	FWwiseTreeItemPtr CurrItem = MakeShared<FWwiseTreeItem>(WaapiItem->DisplayName, WaapiItem->FolderPath, nullptr, WaapiItem->ItemType, WaapiItem->ItemId, WaapiItem->ShortId);
	if(WaapiItem->ShouldDisplayInfo())
	{
		FString DefaultAssetName = WaapiItem->GetDefaultAssetName();
		TArray<FAssetData> Assets;
		FName AssetName;
		EWwiseItemType::Type ItemType;
		UAssetDataSource->GetAssetsInfo(WaapiItem->ItemId, WaapiItem->ShortId, DefaultAssetName, WaapiItem->GroupId, ItemType, AssetName, Assets);
		CurrItem->UAssetName = AssetName;
		CurrItem->Assets = Assets;
		CurrItem->bIsInWrongLocation = IsMoved(CurrItem);
	}
	CurrItem->WaapiName = WaapiItem->DisplayName;
	CurrItem->bWaapiRefExists = true;
	CurrItem->ChildCountInWwise = WaapiItem->ChildCountInWwise;
	TreeItemRootDst->AddChild(CurrItem);
}

void FWwiseBrowserDataSource::CreateWaapiItem(const FWwiseTreeItemPtr& TreeItemRootWaapi, FWwiseTreeItemPtr& TreeItemRootDst)
{
	FWwiseTreeItemPtr NewItem;
	for (const auto& WaapiItem : TreeItemRootWaapi->GetChildren())
	{
		NewItem = TreeItemRootDst->GetChild(WaapiItem->ItemId, WaapiItem->ShortId, WaapiItem->DisplayName);
		if (NewItem)
		{
			NewItem->WaapiName = WaapiItem->DisplayName;
			NewItem->bWaapiRefExists = true;
			if(NewItem->IsFolder())
			{
				NewItem->ItemId = WaapiItem->ItemId;
			}
			continue;
		}
		if (WaapiItem->IsFolder())
		{
			NewItem = MakeShared<FWwiseTreeItem>(WaapiItem->DisplayName, WaapiItem->FolderPath, nullptr, WaapiItem->ItemType, WaapiItem->ItemId, WaapiItem->ShortId);
			TreeItemRootDst->AddChild(NewItem);
			continue;
		}
		auto SoundBankItem = ProjectDBDataSource->FindItem(WaapiItem);
		//The item was moved in Wwise
		if (SoundBankItem)
		{
			auto Root = TreeItemRootDst->GetRoot();
			NewItem = Root->FindItemRecursive(WaapiItem);
			if (NewItem)
			{
				NewItem->WaapiName = WaapiItem->DisplayName;
				NewItem->bWaapiRefExists = true;
				NewItem->bSameLocation = false;
			}
		}
		else
		{
			CreateWaapiExclusiveItem(WaapiItem, TreeItemRootDst);
		}
	}
	TreeItemRootDst->SortChildren();
	for (auto CurrItem : TreeItemRootDst->GetChildren())
	{
		if (TreeItemRootWaapi)
		{
			if (FWwiseTreeItemPtr WaapiItem = TreeItemRootWaapi->GetChild(CurrItem->ItemId, CurrItem->ShortId, CurrItem->DisplayName))
			{
				CreateWaapiItem(WaapiItem, CurrItem);
			}
		}
	}
}

void FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed"))

	auto RefreshTask = [this]
	{
		if(ProjectDBDataSource->ConstructTree(false))
		{
			UAssetDataSource->ConstructItems();
			MergeDataSources();
		}
	};

	WwiseUnrealHelper::RunTaskInGameThread(RefreshTask);
}

void FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed"))

	auto RefreshTask = [this]
	{
			UAssetDataSource->ConstructItems();
			WaapiDataSource->ConstructTree(false);
			MergeDataSources();
	};

	WwiseUnrealHelper::RunTaskInGameThread(RefreshTask);
}

void FWwiseBrowserDataSource::SetupAssetCallbacks()
{
	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (AssetRegistryModule)
	{
		auto& AssetRegistry = AssetRegistryModule->Get();
		OnAssetAdded = AssetRegistry.OnAssetAdded().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetRemoved = AssetRegistry.OnAssetRemoved().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetRenamed = AssetRegistry.OnAssetRenamed().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetUpdated = AssetRegistry.OnAssetUpdated().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
	}
}

void FWwiseBrowserDataSource::OnFilesFullyLoaded()
{
	UE_LOG(LogAudiokineticTools, Log, TEXT("Asset registry files loaded"))
	// Should only be called from the AssetRegistry on the GameThread
	checkf(IsInGameThread(), TEXT("FWwiseBrowserDataSource::OnFilesFullyLoaded should only be called from the AssetRegistry on the GameThread"))

	if (OnFilesLoaded.IsValid())
	{
		FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		if (AssetRegistryModule)
		{
			auto& AssetRegistry = AssetRegistryModule->Get();
			AssetRegistry.OnFilesLoaded().Remove(OnFilesLoaded);
		}
	}
	SetupAssetCallbacks();
	UAssetDataSource->ConstructItems();
	MergeDataSources();
}

void FWwiseBrowserDataSource::OnTimerTick(float DeltaSeconds)
{
	AssetUpdateTimer -= DeltaSeconds;
	if (AssetUpdateTimer <= 0.f)
	{
		if(UAssetDataSource)
		{
			UAssetDataSource->ConstructItems();
		}
		MergeDataSources();
		if (PostEditorTickHandle.IsValid())
		{
			GEngine->OnPostEditorTick().Remove(PostEditorTickHandle);
			PostEditorTickHandle.Reset();
		}
	}
}

void FWwiseBrowserDataSource::OnUAssetSourceRefresh(const FAssetData& AssetData)
{
	if (AkUnrealAssetDataHelper::IsAssetAkAudioType(AssetData) && !AkUnrealAssetDataHelper::IsAssetTransient(AssetData))
	{
		checkf(IsInGameThread(), TEXT("FWwiseBrowserDataSource::OnUAsserSourceRefresh should only be called from the AssetRegistry on the GameThread"))

		if (!PostEditorTickHandle.IsValid())
		{
			PostEditorTickHandle = GEngine->OnPostEditorTick().AddRaw(this, &FWwiseBrowserDataSource::OnTimerTick);
		}

		AssetUpdateTimer = AssetTimerRefresh;
	}
}

// Required for handling Asset renaming
void FWwiseBrowserDataSource::OnUAssetSourceRefresh(const FAssetData& AssetData, const FString& /*OldPath*/)
{
	OnUAssetSourceRefresh(AssetData);
}

void FWwiseBrowserDataSource::OnWwiseSelectionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items)
{
	WwiseSelectionChange.ExecuteIfBound(Items);
}

void FWwiseBrowserDataSource::OnWwiseExpansionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items)
{
	WwiseExpansionChange.ExecuteIfBound(Items);
}
