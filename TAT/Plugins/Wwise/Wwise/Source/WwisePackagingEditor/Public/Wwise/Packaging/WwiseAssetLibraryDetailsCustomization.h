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

#include "IDetailCustomization.h"
#include "WwiseAssetLibraryProcessor.h"
#include "Misc/TextFilter.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "Wwise/Packaging/WwiseFilterableAssetLibrary.h"

#include "WwiseAssetLibraryDetailsCustomization.generated.h"

class UWwisePackagingSettings;
/**
 * This is a helper adding the Language Name that is slow to calculate in sorting order.
 */
USTRUCT()
struct WWISEPACKAGINGEDITOR_API FWwiseAssetLibraryTreeViewRef
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	EWwiseAssetLibraryRefType Type { EWwiseAssetLibraryRefType::Unknown };

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	int Id { 0 };

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	FName Name {};

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	FString SourcePath {};

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	FName LanguageName {};

	FWwiseAssetLibraryTreeViewRef() {}
	FWwiseAssetLibraryTreeViewRef(const FWwiseAssetLibraryRef& Ref, const FString& InSourcePath, const FName& InLanguageName)
	{
		Type = Ref.Type;
		Id = Ref.Id;
		Name = Ref.Name;
		SourcePath = InSourcePath;
		LanguageName = InLanguageName;
	}
	
	virtual ~FWwiseAssetLibraryTreeViewRef() {}

	bool operator==(const FWwiseAssetLibraryTreeViewRef& Rhs) const
	{
		return Type == Rhs.Type && Id == Rhs.Id && Name == Rhs.Name
			&& SourcePath == Rhs.SourcePath && LanguageName == Rhs.LanguageName;
	}
	bool operator!=(const FWwiseAssetLibraryTreeViewRef& Rhs) const
	{
		return !(*this == Rhs);
	}
};
inline uint32 GetTypeHash(const FWwiseAssetLibraryTreeViewRef& Key)
{
	return
		HashCombine(GetTypeHash(Key.Type),
		HashCombine(GetTypeHash(Key.Id),
		HashCombine(GetTypeHash(Key.Name),
		HashCombine(GetTypeHash(Key.SourcePath),
			GetTypeHash(Key.LanguageName)))));
}

/**
 * FWwiseAssetLibraryDetailsCustomization
 */
class FWwiseAssetLibraryDetailsCustomization : public IDetailCustomization
{
public:
	FWwiseAssetLibraryDetailsCustomization();
	virtual ~FWwiseAssetLibraryDetailsCustomization() override;
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void PendingDelete() override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	// End of IDetailCustomization interface

protected:
	using FFilteredAssetRef = TSharedRef<FWwiseAssetLibraryTreeViewRef>;
	using FFilteredAssetPtr = TSharedPtr<FWwiseAssetLibraryTreeViewRef>;
	using FFilteredAssetsTextFilter = TTextFilter<const FFilteredAssetRef&>;
	
	// SDetailsTableRow definition
	virtual TSharedPtr<STreeView<FFilteredAssetRef>> GenerateTreeView();
	virtual TSharedRef<SWidget> GenerateSearchBar();
	virtual TSharedRef<SHeaderRow> GenerateHeaderRow();
	virtual TSharedRef<SWidget> GenerateWidgetCount();

	struct Column
	{
		static const FName Type;
		static const FName Name;
		static const FName SourcePath;
		static const FName Language;
		static const FName ShortId;
	};

	class SDetailsTableRow: public SMultiColumnTableRow<FFilteredAssetRef>
	{
		using Super = SMultiColumnTableRow<FFilteredAssetRef>;
	public:
		SLATE_BEGIN_ARGS(SDetailsTableRow) {}
		SLATE_END_ARGS()

		using SMultiColumnTableRow::SMultiColumnTableRow;
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
		virtual TSharedRef<SWidget> GenerateWidgetType(const FWwiseAssetLibraryTreeViewRef& Item);
		virtual TSharedRef<SWidget> GenerateWidgetName(const FWwiseAssetLibraryTreeViewRef& Item);
		virtual TSharedRef<SWidget> GenerateWidgetSourcePath(const FWwiseAssetLibraryTreeViewRef& Item);
		virtual TSharedRef<SWidget> GenerateWidgetLanguage(const FWwiseAssetLibraryTreeViewRef& Item);
		virtual TSharedRef<SWidget> GenerateWidgetShortId(const FWwiseAssetLibraryTreeViewRef& Item);

		void Construct(const FArguments& InArgs, FFilteredAssetRef Item, const TSharedRef<STableViewBase>& OwnerTableView)
		{
			ItemPtr = Item.ToSharedPtr();
			Super::Construct({}, OwnerTableView);
		}

	protected:
		FFilteredAssetPtr ItemPtr;
	};
	// End of SDetailsTableRow definition

	// A pointer to the previous Detail Builder
	TWeakPtr<IDetailLayoutBuilder> WeakDetailBuilder;

	// The total number of assets
	int32 TotalAssetsCount;

	// The count of filtered assets
	int32 FilteredAssetsCount;

	// A list of the filtered assets, as used to search/filter and sort the Table.
	TArray<FFilteredAssetRef> FilteredAssets;

	// A list of the filtered assets, once all the sorting and searching is applied
	TArray<FFilteredAssetRef> DisplayedAssets;

	// A delegate set to receive notifications when a Filter is updated (Bound to OnRefreshFilter)
	FDelegateHandle OnRefreshFilterDelegate;

	// A delegate set to receive notifications when the Packaging settings are modified
	FDelegateHandle OnPackagingSettingsChangedDelegate;

	// Shared element telling whether the class instance is deleted or not
	TSharedRef<bool, ESPMode::ThreadSafe> IsAliveRef { new bool{true} };

	// Event that tells whether there's currently an ongoing RebuildFilteredAssets (Waitable) or not (Triggered).
	FEventRef WorkerEvent{ EEventMode::ManualReset };

	// The event thread that got its act
	std::atomic<uint32> WorkingThreadId{ 0 };

	// Number of operations waiting to be executed
	std::atomic<uint32> WorkersWaiting{ 0 };

	// Is a rebuild already requested (we don't want to do it twice).
	bool bRebuildAlreadyRequested{ false };

	// Is an search and sort already requested (we don't want to do it twice).
	bool bSearchAndSortAlreadyRequested{ false };

	// Tree view for the filtered assets
	TSharedPtr<STreeView<FFilteredAssetRef>> FilteredAssetsTreeView;

	// Search box text filter 
	TSharedPtr<FFilteredAssetsTextFilter> FilteredAssetsTextFilter;

	// Text Block to display the filtered assets count
	TSharedPtr<STextBlock> FilteredAssetsCountView;

	// Sort Column
	FName SortColumn{ Column::Type };

	// Sort Mode
	EColumnSortMode::Type SortMode{ EColumnSortMode::Ascending };

	// Is this asset an Asset Library. Calculated in CustomizeDetails.
	bool bIsAssetLibrary{ false };
	
	// Whether the Asset Library is represented in the Packaging Settings or not. Calculated in UpdateAssetLibraryInSettings.
	bool bAssetLibraryInSettings{ false };

	// Show what will be packaged in this asset.
	bool bHonorPackagingSettings{ true };

	virtual UWwiseFilterableAssetLibrary* GetFilterableAssetLibrary() const;
	virtual void OnObjectPropertyChanged(UObject* Object, struct FPropertyChangedEvent&);
	virtual void OnSettingsChanged(UWwisePackagingSettings* PackagingSettings);
	virtual EVisibility OnProgressBarVisibilityCheck() const;
	virtual void OnSearchBoxChanged(const FText& InSearchText);
	static void PopulateSearchStrings(const FFilteredAssetRef& Asset, OUT TArray<FString>& OutSearchStrings);
	virtual void OnSortModeChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	
	virtual void RebuildFilteredAssets();
	virtual bool CalculateFilteredAssets(UWwiseFilterableAssetLibrary* AssetLibrary, TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData);
	virtual bool CopyFilteredAssets(TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData);
	virtual void ApplySearchAndSort();
	virtual void RefreshTree();

	virtual void UpdateAssetLibraryInSettings(const UWwisePackagingSettings* PackagingSettings = nullptr);
	virtual EVisibility IsHonorPackagingSettingsVisible() const;
	virtual bool IsHonorPackagingSettingsEnabled() const;
	virtual ECheckBoxState IsHonorPackagingSettingsChecked() const;
	virtual void HonorPackagingSettingsStateChanged(ECheckBoxState CheckBoxState);
	virtual FReply HonorPackagingSettingsLinkClicked() const;
	virtual bool IsPackagingSettingsHonored() const;
	virtual void FilterPackagingSettings(FWwiseAssetLibraryProcessor* Processor,
		const TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData, UWwiseFilterableAssetLibrary* StopBefore) const;

	virtual void ClaimWorkingThread();
	virtual void ReleaseWorkingThread();
};
