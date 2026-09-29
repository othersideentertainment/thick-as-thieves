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

#include "Wwise/Packaging/WwiseAssetLibraryDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Tasks/Task.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/Packaging/WwiseAssetLibrary.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilter.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilteringSharedData.h"
#include "Wwise/Packaging/WwiseAssetLibraryProcessor.h"
#include "Wwise/WwisePluginStyle.h"

#include <inttypes.h>

#include "ISettingsModule.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Wwise/WwiseGuidConverter.h"
#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Packaging/WwisePackagingSettings.h"

#define LOCTEXT_NAMESPACE "WwisePackaging"

const FName FWwiseAssetLibraryDetailsCustomization::Column::Type{ "Type" };
const FName FWwiseAssetLibraryDetailsCustomization::Column::Name{ "Name" };
const FName FWwiseAssetLibraryDetailsCustomization::Column::SourcePath{ "SourcePath" };
const FName FWwiseAssetLibraryDetailsCustomization::Column::Language{ "Language" };
const FName FWwiseAssetLibraryDetailsCustomization::Column::ShortId{ "ShortId" };

FWwiseAssetLibraryDetailsCustomization::FWwiseAssetLibraryDetailsCustomization()
{
	FilteredAssetsTextFilter = MakeShareable(new FFilteredAssetsTextFilter(
		FFilteredAssetsTextFilter::FItemToStringArray::CreateStatic(
			&FWwiseAssetLibraryDetailsCustomization::PopulateSearchStrings)));

	WorkerEvent->Trigger();
}

FWwiseAssetLibraryDetailsCustomization::~FWwiseAssetLibraryDetailsCustomization()
{
}

TSharedRef<IDetailCustomization> FWwiseAssetLibraryDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FWwiseAssetLibraryDetailsCustomization());
}

void FWwiseAssetLibraryDetailsCustomization::PendingDelete()
{
	*IsAliveRef = false;
	
	while(true)
	{
		ClaimWorkingThread();
		if (WorkersWaiting > 0)
		{
			ReleaseWorkingThread();
		}
		else
		{
			break;
		}
	}

	OnRefreshFilterDelegate.Reset();
	OnPackagingSettingsChangedDelegate.Reset();
}

void FWwiseAssetLibraryDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& DefaultCategory{ DetailBuilder.EditCategory(TEXT("Default")) };
	IDetailCategoryBuilder& PreviewCategory{ DetailBuilder.EditCategory(
		TEXT("Preview"),
		LOCTEXT("Preview", "Preview"),
		ECategoryPriority::TypeSpecific)
	};

	DefaultCategory
		.SetSortOrder(1);
	PreviewCategory
    	.InitiallyCollapsed(false)
    	.SetSortOrder(2);

	const auto InfoHandle{ DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UWwiseFilterableAssetLibrary, Info), UWwiseFilterableAssetLibrary::StaticClass()) };
	const auto FiltersHandle{ DetailBuilder.GetProperty("Info.Filters", UWwiseFilterableAssetLibrary::StaticClass()) };
	const auto SharedFiltersHandle{ DetailBuilder.GetProperty("Info.SharedFilters", UWwiseFilterableAssetLibrary::StaticClass()) };

	// Remove the default "WwiseAssetLibraryInfo" structure
	DetailBuilder.HideProperty(InfoHandle);

	// Add the Filters in root, and auto-expand them.
	DefaultCategory.AddProperty(FiltersHandle)
		.ShouldAutoExpand();
	DefaultCategory.AddProperty(SharedFiltersHandle)
		.ShouldAutoExpand();

	// Add the Filtered Assets list
	FilteredAssetsTreeView = GenerateTreeView();
	PreviewCategory.AddCustomRow(LOCTEXT("FilteredAssets", "Filtered Assets"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						GenerateSearchBar()
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.f)
					[
						GenerateWidgetCount()
					]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
							.Visibility_Raw(this, &FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsVisible)
							.IsEnabled_Raw(this, &FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsEnabled)
							.IsChecked_Raw(this, &FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsChecked)
							.OnCheckStateChanged_Raw(this, &FWwiseAssetLibraryDetailsCustomization::HonorPackagingSettingsStateChanged)
							.ToolTipText(LOCTEXT("HonorPackagingSettingsTooltip", "Display the actual contents of the Asset Library, according to its placement in the Asset Library list in the Wwise Packaging settings."))
							.Padding(4.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
									.VAlign(VAlign_Center)
									.AutoWidth()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("HonorPackagingSettingsText", "Apply Wwise Packaging Settings"))
									]
								+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SButton)
											.VAlign(VAlign_Center)
											.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
											.OnClicked_Raw(this, &FWwiseAssetLibraryDetailsCustomization::HonorPackagingSettingsLinkClicked)
											.ToolTipText(LOCTEXT("HonorPackagingSettingsLinkTooltip", "Open Packaging Settings"))
											[
												SNew(SImage)
													.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
											]
									]
							]
					]
			]
		+ SVerticalBox::Slot()
			.MaxHeight(400.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
					[
						FilteredAssetsTreeView.ToSharedRef()	
					]
				+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SProgressBar)
							.ToolTipText(LOCTEXT("PreparingTooltip", "Preparing..."))
							.Visibility_Raw(this, &FWwiseAssetLibraryDetailsCustomization::OnProgressBarVisibilityCheck)
					]
			]
	];
}

void FWwiseAssetLibraryDetailsCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	WeakDetailBuilder = DetailBuilder;

	if (DetailBuilder.IsValid())
	{
		if (!OnRefreshFilterDelegate.IsValid())
		{
			OnRefreshFilterDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
				this, &FWwiseAssetLibraryDetailsCustomization::OnObjectPropertyChanged);
		}
		if (!OnPackagingSettingsChangedDelegate.IsValid())
		{
			UWwisePackagingSettings* PackagingSettings = GetMutableDefault<UWwisePackagingSettings>();
			if (LIKELY(PackagingSettings))
			{
				OnPackagingSettingsChangedDelegate = PackagingSettings->OnSettingsChanged.AddSP(
					this, &FWwiseAssetLibraryDetailsCustomization::OnSettingsChanged);
			}
		}

		if (auto* Filterable = GetFilterableAssetLibrary())
		{
			if (!Filterable)
			{
				bIsAssetLibrary = false;
			}
			else
			{
				auto* AssetLibrary = Cast<UWwiseAssetLibrary>(Filterable);
				bIsAssetLibrary = (AssetLibrary != nullptr);
			}
		}
	}
	
	UpdateAssetLibraryInSettings();
	RebuildFilteredAssets();

	CustomizeDetails(*DetailBuilder);
}

TSharedPtr<STreeView<FWwiseAssetLibraryDetailsCustomization::FFilteredAssetRef>> FWwiseAssetLibraryDetailsCustomization::GenerateTreeView()
{
	return SNew(STreeView<FFilteredAssetRef>)
		.TreeItemsSource(&DisplayedAssets)
		.SelectionMode(ESelectionMode::None)
		.HeaderRow(GenerateHeaderRow())
		.OnGenerateRow_Lambda([this](FFilteredAssetRef Item, const TSharedRef<STableViewBase>& OwnerTable)
		{
			return SNew(SDetailsTableRow, Item, OwnerTable);
		})
		.OnGetChildren_Lambda([](auto TreeItem, auto& Out) {})
		.AllowOverscroll(EAllowOverscroll::No);
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::GenerateSearchBar()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
		.MaxWidth(500)
		[
			SNew(SSearchBox)
				.HintText(LOCTEXT("SearchHint", "Search..."))
				.OnTextChanged(this, &FWwiseAssetLibraryDetailsCustomization::OnSearchBoxChanged)
				.SelectAllTextWhenFocused(false)
				.DelayChangeNotificationsWhileTyping(true)
		];
}

TSharedRef<SHeaderRow> FWwiseAssetLibraryDetailsCustomization::GenerateHeaderRow()
{
	return SNew(SHeaderRow)
	+ SHeaderRow::Column(Column::Type)
		.FixedWidth(48.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.SortMode_Lambda([this] { return SortColumn == Column::Type ? SortMode : EColumnSortMode::None; })
		.OnSort(this, &FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("FilteredAssets_Header_Type", "Type"))
		]
	+ SHeaderRow::Column(Column::Name)
		.FillWidth(0.5f)
		.HAlignHeader(HAlign_Left)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Left)
		.VAlignCell(VAlign_Center)
		.SortMode_Lambda([this] { return SortColumn == Column::Name ? SortMode : EColumnSortMode::None; })
		.OnSort(this, &FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("FilteredAssets_Header_Name", "Asset Name"))
		]
	+ SHeaderRow::Column(Column::SourcePath)
		.FillWidth(0.5f)
		.HAlignHeader(HAlign_Left)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Left)
		.VAlignCell(VAlign_Center)
		.SortMode_Lambda([this] { return SortColumn == Column::SourcePath ? SortMode : EColumnSortMode::None; })
		.OnSort(this, &FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("FilteredAssets_Header_SourcePath", "Source Path"))
		]
	+ SHeaderRow::Column(Column::Language)
		.ManualWidth(110.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Left)
		.VAlignCell(VAlign_Center)
		.SortMode_Lambda([this] { return SortColumn == Column::Language ? SortMode : EColumnSortMode::None; })
		.OnSort(this, &FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("FilteredAssets_Header_Language", "Language"))
		]
	+ SHeaderRow::Column(Column::ShortId)
		.ManualWidth(90.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Right)
		.VAlignCell(VAlign_Center)
		.SortMode_Lambda([this] { return SortColumn == Column::ShortId ? SortMode : EColumnSortMode::None; })
		.OnSort(this, &FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("FilteredAssets_Header_ShortId", "ID"))
		];
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::GenerateWidgetCount()
{
	TSharedRef<STextBlock> textBlock = SNew(STextBlock)
	.Text(FText::FromString(FString::Printf(TEXT("%" PRIu32 "/" "%" PRIu32), FilteredAssetsCount, TotalAssetsCount)))
	.Margin(FMargin(2, 0, 4, 0));
	FilteredAssetsCountView = textBlock;
	return textBlock;
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	auto OwnerTable{ OwnerTablePtr.Pin() };
	if (!OwnerTable)
	{
		return SNew(SBox);
	}

	if (!ItemPtr.IsValid())
	{
		return SNew(SBox);
	}
	auto& Item{ *ItemPtr };


	if (InColumnName == Column::Type)
	{
		return GenerateWidgetType(Item);
	}
	else if (InColumnName == Column::Name)
	{
		return GenerateWidgetName(Item);
	}
	else if (InColumnName == Column::SourcePath)
	{
		return GenerateWidgetSourcePath(Item);
	}
	else if (InColumnName == Column::Language)
	{
		return GenerateWidgetLanguage(Item);
	}
	else if (InColumnName == Column::ShortId)
	{
		return GenerateWidgetShortId(Item);
	}
	
	return SNew(SBox);
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetType(const FWwiseAssetLibraryTreeViewRef& Item)
{
	auto WwiseStyle{ FWwisePluginStyle::Get() };
	if (!WwiseStyle.IsValid())
	{
		return SNew(SBox);
	}

	switch(Item.Type)
	{
	case EWwiseAssetLibraryRefType::InitBank:
		return SNew(SImage)
			.Image(WwiseStyle->GetBrush(FWwisePluginStyle::InitBankIconName));
	case EWwiseAssetLibraryRefType::SoundBank:
		return SNew(SImage)
			.Image(WwiseStyle->GetBrush(FWwisePluginStyle::SoundBankIconName));
	case EWwiseAssetLibraryRefType::Media:
		return SNew(SImage)
			.Image(WwiseStyle->GetBrush(FWwisePluginStyle::MediaIconName));
	case EWwiseAssetLibraryRefType::ExternalSource:
		return SNew(SImage)
			.Image(WwiseStyle->GetBrush(FWwisePluginStyle::ExternalSourceIconName));
	case EWwiseAssetLibraryRefType::Unknown:
	default:
		return SNew(SBox);
	}
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetName(const FWwiseAssetLibraryTreeViewRef& Item)
{
	return SNew(STextBlock)
		.Text(FText::FromName(Item.Name))
		.Margin(FMargin(2, 0, 2, 0));

}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetSourcePath(const FWwiseAssetLibraryTreeViewRef& Item)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Item.SourcePath))
		.Margin(FMargin(2, 0, 2, 0));

}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetLanguage(const FWwiseAssetLibraryTreeViewRef& Item)
{
	return SNew(STextBlock)
		.Text(FText::FromName(Item.LanguageName))
		.Margin(FMargin(2, 0, 2, 0));
}

TSharedRef<SWidget> FWwiseAssetLibraryDetailsCustomization::SDetailsTableRow::GenerateWidgetShortId(const FWwiseAssetLibraryTreeViewRef& Item)
{
	return SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("%" PRIu32), (uint32)Item.Id)))
		.Margin(FMargin(2, 0, 4, 0));
}

UWwiseFilterableAssetLibrary* FWwiseAssetLibraryDetailsCustomization::GetFilterableAssetLibrary() const
{
	if (!WeakDetailBuilder.IsValid())
	{
		return nullptr;
	}

	auto DetailBuilder{ WeakDetailBuilder.Pin() };
	if (UNLIKELY(!DetailBuilder.IsValid()))
	{
		return nullptr;
	}

	TArray<TWeakObjectPtr<UWwiseFilterableAssetLibrary>> ObjectsBeingCustomized{
		DetailBuilder->GetObjectsOfTypeBeingCustomized<UWwiseFilterableAssetLibrary>()
	};

	if (ObjectsBeingCustomized.Num() == 0)
	{
		return nullptr;
	}
	return ObjectsBeingCustomized.Last().Get();
}

void FWwiseAssetLibraryDetailsCustomization::OnObjectPropertyChanged(UObject* Object, struct FPropertyChangedEvent&)
{
	if (!Object->IsA(UWwiseAssetLibraryFilter::StaticClass())
		&& !Object->IsA(UWwiseFilterableAssetLibrary::StaticClass()))
	{
		return;
	}

	RebuildFilteredAssets();
}

void FWwiseAssetLibraryDetailsCustomization::OnSettingsChanged(UWwisePackagingSettings* PackagingSettings)
{
	UpdateAssetLibraryInSettings(PackagingSettings);
}

EVisibility FWwiseAssetLibraryDetailsCustomization::OnProgressBarVisibilityCheck() const
{
	if (WorkerEvent->Wait(0, true))
	{
		return EVisibility::Collapsed;
	}
	else
	{
		return EVisibility::Visible;
	}
}

void FWwiseAssetLibraryDetailsCustomization::OnSearchBoxChanged(const FText& InSearchText)
{
	if (FilteredAssetsTextFilter.IsValid())
	{
		FilteredAssetsTextFilter->SetRawFilterText(InSearchText);

		ApplySearchAndSort();
	}
}

void FWwiseAssetLibraryDetailsCustomization::PopulateSearchStrings(const FFilteredAssetRef& Asset, OUT TArray<FString>& OutSearchStrings)
{
	OutSearchStrings.Add(
		FString::Printf(TEXT("%s %s %s %" PRIu32),
			*Asset->Name.ToString(),
			*Asset->SourcePath,
			*Asset->LanguageName.ToString(),
			(uint32)Asset->Id));
}

void FWwiseAssetLibraryDetailsCustomization::OnSortModeChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode)
{
	SortColumn = Column;
	SortMode = Mode;
	ApplySearchAndSort();
}

void FWwiseAssetLibraryDetailsCustomization::RebuildFilteredAssets()
{
	// Don't rebuild twice in a row
	if (bRebuildAlreadyRequested)
	{
		return;
	}
	bRebuildAlreadyRequested = true;

	AsyncTask(ENamedThreads::GameThread, [this, IsAliveRefCopy = IsAliveRef]() mutable
	{
		if (UNLIKELY(!*IsAliveRefCopy))
		{
			return;
		}
		
		ClaimWorkingThread();
		if (!WeakDetailBuilder.IsValid())
		{
			ReleaseWorkingThread();
			return;
		}

		if (UWwiseFilterableAssetLibrary* AssetLibrary { GetFilterableAssetLibrary() })
		{
			AssetLibrary->Info.PreloadFilters();
		}

		ReleaseWorkingThread();
		UE::Tasks::Launch(TEXT("FWwiseAssetLibraryDetailsCustomization::RebuildFilteredAssets"), [this, IsAliveRefCopy = IsAliveRef]
		{
			if (UNLIKELY(!*IsAliveRefCopy))
			{
				return;
			}
			ClaimWorkingThread();
			bRebuildAlreadyRequested = false;

			if (!WeakDetailBuilder.IsValid())
			{
				ReleaseWorkingThread();
				return;
			}

			UWwiseFilterableAssetLibrary* AssetLibrary { GetFilterableAssetLibrary() };
			if (!AssetLibrary)
			{
				ReleaseWorkingThread();
				return;
			}

			TUniquePtr<FWwiseAssetLibraryFilteringSharedData> FilteringSharedData;
			
			const bool bResult {
				CalculateFilteredAssets(AssetLibrary, FilteringSharedData)
				&& CopyFilteredAssets(FilteringSharedData)
			};
			ReleaseWorkingThread();

			if (bResult)
			{
				ApplySearchAndSort();
			}
		});
	});
}

bool FWwiseAssetLibraryDetailsCustomization::CalculateFilteredAssets(UWwiseFilterableAssetLibrary* AssetLibrary, TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData)
{
	auto* ProjectDB{ FWwiseProjectDatabase::Get() };
	if (UNLIKELY(!ProjectDB))
	{
		return false;
	}
	
	FWwiseAssetLibraryProcessor* Processor{ FWwiseAssetLibraryProcessor::Get() };
	if(UNLIKELY(!Processor))
	{
		return false;
	}

	FilteringSharedData = TUniquePtr<FWwiseAssetLibraryFilteringSharedData>{ Processor->InstantiateSharedData(*ProjectDB) };
	Processor->RetrieveAssetMap(*FilteringSharedData);

	if (IsPackagingSettingsHonored())
	{
		FilterPackagingSettings(Processor, FilteringSharedData, AssetLibrary);
	}	

	Processor->FilterLibraryAssets(*FilteringSharedData, AssetLibrary->Info, false, false);
	return true;
}

bool FWwiseAssetLibraryDetailsCustomization::CopyFilteredAssets(TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData)
{
	auto* ProjectDB{ FWwiseProjectDatabase::Get() };
	if (UNLIKELY(!ProjectDB))
	{
		return false;
	}

	FWwiseAssetLibraryProcessor* Processor{ FWwiseAssetLibraryProcessor::Get() };
	if (UNLIKELY(!Processor))
	{
		return false;
	}

	WwiseDataStructureScopeLock DB(*ProjectDB);

	// Count all pre-filtered assets
	TSet<FWwiseAssetLibraryTreeViewRef> TotalDeduplicatedAssets;
	TotalDeduplicatedAssets.Reserve(FilteringSharedData->Sources.Num());
	FWwiseAssetLibraryRef NewRef;
	for (const auto& SourceRef : FilteringSharedData->Sources)
	{
		FString SourcePath;
		Processor->CreateAssetLibraryRef(NewRef, SourceRef);
		switch (NewRef.Type)
		{
		case EWwiseAssetLibraryRefType::InitBank:
		case EWwiseAssetLibraryRefType::SoundBank:
		{
			const auto Ref {
				DB.GetSoundBank({
					NewRef.Guid,
					(uint32)NewRef.Id,
					NewRef.Name,
					(uint32)NewRef.SoundBankId
				}, NewRef.LanguageId)
			};
			if (!Ref.IsValid())
			{
				break;
			}
			SourcePath = *Ref.GetSoundBank()->Path;
			break;
		}
		case EWwiseAssetLibraryRefType::Media:
		{
			const auto Ref {
				DB.GetMediaFile({
					NewRef.Guid,
					(uint32)NewRef.Id,
					NewRef.Name,
					(uint32)NewRef.SoundBankId
				})
			};
			if (!Ref.IsValid())
			{
				break;
			}
			SourcePath = *Ref.GetMedia()->Path;
			break;
		}
		}
		FName LanguageName{*DB.GetLanguageName(NewRef.LanguageId)};
		if (LanguageName.IsNone())
		{
			LanguageName = TEXT("SFX");
		}
		FWwiseAssetLibraryTreeViewRef Ref{NewRef, SourcePath, LanguageName};
		TotalDeduplicatedAssets.Add(Ref);
	}

	// Compile list of (de-duplicated) filtered assets
	FilteredAssets.Reset(FilteringSharedData->FilteredAssets.Num());

	TSet<FWwiseAssetLibraryTreeViewRef> SeenAssets;
	for (const auto& FilteredAsset : FilteringSharedData->FilteredAssets)
	{
		FString SourcePath;
		switch (FilteredAsset.Type)
		{
		case EWwiseAssetLibraryRefType::InitBank:
		case EWwiseAssetLibraryRefType::SoundBank:
		{
			const auto Ref {
				DB.GetSoundBank({
					FilteredAsset.Guid,
					(uint32)FilteredAsset.Id,
					FilteredAsset.Name,
					(uint32)FilteredAsset.SoundBankId
				}, FilteredAsset.LanguageId)
			};
			if (!Ref.IsValid())
			{
				break;
			}
			SourcePath = *Ref.GetSoundBank()->Path;
			break;
		}
		case EWwiseAssetLibraryRefType::Media:
		{
			const auto Ref {
				DB.GetMediaFile({
					FilteredAsset.Guid,
					(uint32)FilteredAsset.Id,
					FilteredAsset.Name,
					(uint32)FilteredAsset.SoundBankId
				})
			};
			if (!Ref.IsValid())
			{
				break;
			}
			SourcePath = *Ref.GetMedia()->Path;
			break;
		}
		}
		
		FName LanguageName{ *DB.GetLanguageName(FilteredAsset.LanguageId) };
		if (LanguageName.IsNone())
		{
			LanguageName = TEXT("SFX");
		}

		{
			FWwiseAssetLibraryTreeViewRef Ref{FilteredAsset, SourcePath, LanguageName};
			bool bIsAlreadyInSetPtr{false};
			SeenAssets.Add(Ref, &bIsAlreadyInSetPtr);

			if (!bIsAlreadyInSetPtr)
			{
				FilteredAssets.Emplace(new FWwiseAssetLibraryTreeViewRef{MoveTemp(Ref)});
			}
		}
	}

	TotalAssetsCount = TotalDeduplicatedAssets.Num();
	FilteredAssetsCount = FilteredAssets.Num();
	return true;
}

void FWwiseAssetLibraryDetailsCustomization::ApplySearchAndSort()
{
	// Don't rebuild twice in a row
	if (bSearchAndSortAlreadyRequested)
	{
		return;
	}
	bSearchAndSortAlreadyRequested = true;
	UE::Tasks::Launch(TEXT("FWwiseAssetLibraryDetailsCustomization::ApplySearchAndSort"), [this, IsAliveRefCopy = IsAliveRef]
	{
		if (UNLIKELY(!*IsAliveRefCopy))
		{
			return;
		}
		ClaimWorkingThread();
		bSearchAndSortAlreadyRequested = false;
		
		TArray<FFilteredAssetRef> NewDisplayedAssets;

		// Apply Search filter
		if (FilteredAssetsTextFilter.IsValid())
		{
			NewDisplayedAssets.Reset(FilteredAssets.Num());
			for (const auto& FilteredAsset : FilteredAssets)
			{
				if (FilteredAssetsTextFilter->PassesFilter(FilteredAsset))
				{
					NewDisplayedAssets.Emplace(FilteredAsset);
				}
			}
		}
		else
		{
			NewDisplayedAssets = FilteredAssets;
		}

		// Apply Sort
		if (SortColumn == Column::Type)
		{
			NewDisplayedAssets.Sort([this](const FFilteredAssetRef& a, const FFilteredAssetRef& b)
			{
				if (a->Type == b->Type)
				{
					return SortMode == EColumnSortMode::Ascending ? a->Name.Compare(b->Name) < 0 : a->Name.Compare(b->Name) > 0;
				}
				return SortMode == EColumnSortMode::Ascending ? a->Type < b->Type : a->Type > b->Type;
			});
		} else if (SortColumn == Column::Name)
		{
			NewDisplayedAssets.Sort([this](const FFilteredAssetRef& a, const FFilteredAssetRef& b)
			{
				const auto Result{ a->Name.Compare(b->Name) };
				if (Result == 0)
				{
					return SortMode == EColumnSortMode::Ascending ? a->LanguageName.ToString() < b->LanguageName.ToString() : a->LanguageName.ToString() > b->LanguageName.ToString();
				}
				return SortMode == EColumnSortMode::Ascending ? Result < 0 : Result > 0;
			});
		}
		else if (SortColumn == Column::Language)
		{
			NewDisplayedAssets.Sort([this](const FFilteredAssetRef& a, const FFilteredAssetRef& b)
			{
				const auto Result{ a->LanguageName.ToString().Compare(b->LanguageName.ToString()) };
				if (Result == 0)
				{
					return SortMode == EColumnSortMode::Ascending ? a->Name.Compare(b->Name) < 0 : a->Name.Compare(b->Name) > 0;
				}
				return SortMode == EColumnSortMode::Ascending ? Result < 0 : Result > 0;
			});
		}
		else if (SortColumn == Column::ShortId)
		{
			NewDisplayedAssets.Sort([this](const FFilteredAssetRef& a, const FFilteredAssetRef& b)
			{
				return SortMode == EColumnSortMode::Ascending ? (uint32)a->Id < (uint32)b->Id : (uint32)a->Id > (uint32)b->Id;
			});
		}

		AsyncTask(ENamedThreads::GameThread, [this, NewDisplayedAssets = MoveTemp(NewDisplayedAssets), IsAliveRefCopy = IsAliveRef]() mutable
		{
			if (UNLIKELY(!*IsAliveRefCopy))
			{
				return;
			}
			DisplayedAssets = MoveTemp(NewDisplayedAssets);
			RefreshTree();
		});
		
		ReleaseWorkingThread();
	});
}

void FWwiseAssetLibraryDetailsCustomization::RefreshTree()
{
	if (FilteredAssetsTreeView.IsValid())
	{
		FilteredAssetsTreeView->RequestTreeRefresh();
	}

	//Update the item count in preview
	const FString CountText = FString::Printf(TEXT("%u/%u"), FilteredAssetsCount, TotalAssetsCount);
	FilteredAssetsCountView->SetText(FText::FromString(CountText));
	FilteredAssetsCountView->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void FWwiseAssetLibraryDetailsCustomization::UpdateAssetLibraryInSettings(const UWwisePackagingSettings* PackagingSettings)
{
	if (!PackagingSettings)
	{
		PackagingSettings = GetDefault<UWwisePackagingSettings>();
		if (UNLIKELY(!PackagingSettings))
		{
			return;
		}
	}

	UWwiseFilterableAssetLibrary* FilterablePtr { GetFilterableAssetLibrary() };
	if (!FilterablePtr)
	{
		return;
	}
	UWwiseAssetLibrary* AssetLibrary{ Cast<UWwiseAssetLibrary>(FilterablePtr) };
	if (!AssetLibrary)
	{
		return;
	}

	bool bNewValue{ false };
	
	if (PackagingSettings->bPackageAsBulkData)
	{
		for (auto& AssetLibrarySoftPtr : PackagingSettings->AssetLibraries)
		{
			if (AssetLibrarySoftPtr.IsNull())
			{
				continue;
			}

			UWwiseAssetLibrary* PackagedAssetLibrary;
			if (!AssetLibrarySoftPtr.IsValid())
			{
				PackagedAssetLibrary = AssetLibrarySoftPtr.LoadSynchronous();
			}
			else
			{
				PackagedAssetLibrary = AssetLibrarySoftPtr.Get();
			}
			
			if (!PackagedAssetLibrary)
			{
				continue;
			}

			if (PackagedAssetLibrary == AssetLibrary)
			{
				bNewValue = true;
				// Do not break here. We must Get() all of the Asset Libraries in game thread.
			}
		}
	}
	
	bAssetLibraryInSettings = bNewValue;
	if(bAssetLibraryInSettings)
	{
		RebuildFilteredAssets();
	}
}

EVisibility FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsVisible() const
{
	return bIsAssetLibrary ? EVisibility::Visible : EVisibility::Hidden;
}

bool FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsEnabled() const
{
	return bAssetLibraryInSettings;
}

ECheckBoxState FWwiseAssetLibraryDetailsCustomization::IsHonorPackagingSettingsChecked() const
{
	return bHonorPackagingSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FWwiseAssetLibraryDetailsCustomization::HonorPackagingSettingsStateChanged(ECheckBoxState CheckBoxState)
{
	bHonorPackagingSettings = (CheckBoxState == ECheckBoxState::Checked);

	RebuildFilteredAssets();
}

FReply FWwiseAssetLibraryDetailsCustomization::HonorPackagingSettingsLinkClicked() const
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
		.ShowViewer(FName("Project"), FName("Wwise"), FName("Wwise Packaging"));
	return FReply::Handled();
}

bool FWwiseAssetLibraryDetailsCustomization::IsPackagingSettingsHonored() const
{
	return bIsAssetLibrary && bAssetLibraryInSettings && bHonorPackagingSettings;
}

void FWwiseAssetLibraryDetailsCustomization::FilterPackagingSettings(FWwiseAssetLibraryProcessor* Processor,
	const TUniquePtr<FWwiseAssetLibraryFilteringSharedData>& FilteringSharedData, UWwiseFilterableAssetLibrary* StopBefore) const
{
	const UWwisePackagingSettings* PackagingSettings = GetDefault<UWwisePackagingSettings>();
	if (UNLIKELY(!PackagingSettings))
	{
		return;
	}
	const auto& AssetLibraryArray{ PackagingSettings->AssetLibraries };
	for (int Iter = 0; Iter < AssetLibraryArray.Num(); ++Iter)
	{
		const auto& AssetLibrarySoftPtr { AssetLibraryArray[Iter] };
		if (AssetLibrarySoftPtr.IsNull())
		{
			continue;
		}
		auto* AssetLibraryPtr = AssetLibrarySoftPtr.Get();
		if (UNLIKELY(!AssetLibraryPtr))
		{
			continue;
		}

		if (AssetLibraryPtr == StopBefore)
		{
			break;
		}

		Processor->FilterLibraryAssets(*FilteringSharedData, AssetLibraryPtr->Info,
			!AssetLibraryPtr->bFallthrough && Iter < AssetLibraryArray.Num() - 1, false);
	}
}

void FWwiseAssetLibraryDetailsCustomization::ClaimWorkingThread()
{
	++WorkersWaiting;
	while(true)
	{
		WorkerEvent->Wait();
		uint32 Expected{ 0 };
		if (WorkingThreadId.compare_exchange_strong(Expected, FPlatformTLS::GetCurrentThreadId()))
		{
			WorkerEvent->Reset();
			break;
		}
	}
	--WorkersWaiting;
}

void FWwiseAssetLibraryDetailsCustomization::ReleaseWorkingThread()
{
	WorkingThreadId.store(0);
	WorkerEvent->Trigger();
}

#undef LOCTEXT_NAMESPACE
