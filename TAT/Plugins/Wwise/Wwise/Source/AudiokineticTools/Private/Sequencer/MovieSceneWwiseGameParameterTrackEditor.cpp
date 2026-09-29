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

#include "MovieSceneWwiseGameParameterTrackEditor.h"
#include "AkAudioDevice.h"
#include "AkRtpc.h"

#include "MovieScene.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneWwiseGameParameterTrack.h"
#include "MovieSceneWwiseGameParameterSection.h"

#include "SequencerUtilities.h"
#include "ISequencerSection.h"
#include "ISequencerObjectChangeListener.h"
#include "ISectionLayoutBuilder.h"
#include "SequencerSectionPainter.h"

#include "AkAudioStyle.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "ScopedTransaction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"

#include "Editor.h"

#define LOCTEXT_NAMESPACE "MovieSceneWwiseGameParameterTrackEditor"


/**
 * Class that draws a transform section in the sequencer
 */
class FMovieSceneWwiseGameParameterSection
	: public ISequencerSection
{
public:

	FMovieSceneWwiseGameParameterSection(UMovieSceneSection& InSection)
		: Section(Cast<UMovieSceneWwiseGameParameterSection>(&InSection))
	{ }

public:

	// ISequencerSection interface

	virtual UMovieSceneSection* GetSectionObject() override { return Section; }

	virtual FText GetSectionTitle() const override
	{
		return Section->GetGameParameter() ? FText::FromString(Section->GetGameParameter()->GetName()) : FText();
	}

	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override
	{
		return InPainter.PaintSectionBackground();
	}

private:

	/** The section we are visualizing */
	UMovieSceneWwiseGameParameterSection* Section;
};

FMovieSceneWwiseGameParameterTrackEditor::FMovieSceneWwiseGameParameterTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FKeyframeTrackEditor<UMovieSceneWwiseGameParameterTrack>(InSequencer)
{
}

TSharedRef<ISequencerTrackEditor> FMovieSceneWwiseGameParameterTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FMovieSceneWwiseGameParameterTrackEditor(InSequencer));
}

TSharedRef<ISequencerSection> FMovieSceneWwiseGameParameterTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShareable(new FMovieSceneWwiseGameParameterSection(SectionObject));
}

const FSlateBrush* FMovieSceneWwiseGameParameterTrackEditor::GetIconBrush() const
{
	return FAkAudioStyle::Get().GetBrush("Wwise.GameParameterIcon");
}



struct FRTPCSectionCreateDialogOptions
{
	UAkRtpc* RTPC = nullptr;
	bool OkClicked;

	FRTPCSectionCreateDialogOptions() : OkClicked(false) {}

	bool Validate()
	{
		return OkClicked && (RTPC != nullptr);
	}
};


class SCreateAkAudioRTPCSectionDialog
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCreateAkAudioRTPCSectionDialog) { }
	SLATE_END_ARGS()

	/** Construct this widget. */
	void Construct(const FArguments& InArgs, FRTPCSectionCreateDialogOptions& InOptions, TSharedRef<SWindow> InWindow)
	{
		Options = &InOptions;
		Window = InWindow;
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		FAssetPickerConfig AssetPickerConfig;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.Filter.ClassPaths.Add(UAkRtpc::StaticClass()->GetClassPathName());
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateLambda([&InOptions](const FAssetData& InRTPCAssetData) {
			if (InRTPCAssetData.IsValid())
			{
				InOptions.RTPC = CastChecked<UAkRtpc>(InRTPCAssetData.GetAsset());
			}
		});

		ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderImage(FAkAppStyle::Get().GetBrush("Menu.Background"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				.Padding(4)
				[
					SNew(SBorder)
					.BorderImage(FAkAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					.Content()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 2, 6, 2)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("WwiseGameParameter", "Game Parameter"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 2, 0)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.HeightOverride(300.f)
							[
								ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
							]
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(8)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAkAppStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FAkAppStyle::Get().GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FAkAppStyle::Get().GetFloat("StandardDialog.MinDesiredSlotHeight"))

					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FAkAppStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked_Lambda([this]() -> FReply { CloseDialog(true); return FReply::Handled(); })
						.Text(LOCTEXT("OkButtonLabel", "OK"))
					]

					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FAkAppStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked_Lambda([this]() -> FReply { CloseDialog(false); return FReply::Handled(); })
						.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
					]
				]
			]
		];
	}

protected:

	void CloseDialog(bool InOkClicked)
	{
		Options->OkClicked = InOkClicked;

		if (Window.IsValid())
		{
			Window.Pin()->RequestDestroyWindow();
		}
	}

private:

	FRTPCSectionCreateDialogOptions* Options;
	TWeakPtr<SWindow> Window;
};


bool ConfigureRTPCSection(FRTPCSectionCreateDialogOptions& Options)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
	.Title(LOCTEXT("CreateWwiseGameParameterTrackDialog", "Select a Game Parameter for this track"))
	.ClientSize(FVector2D(372, 418))
	.SupportsMinimize(false)
	.SupportsMaximize(false);

	Window->SetContent(SNew(SCreateAkAudioRTPCSectionDialog, Options, Window));
	GEditor->EditorAddModalWindow(Window);

	return Options.Validate();
}



void FMovieSceneWwiseGameParameterTrackEditor::TryAddWwiseGameParameterTrack(FCreateWwiseGameParameterTrack DoCreateWwiseGameParameterTrack)
{
	FRTPCSectionCreateDialogOptions Options;
	if (ConfigureRTPCSection(Options))
	{
		auto FocusedMovieScene = GetFocusedMovieScene();

		if (FocusedMovieScene == nullptr)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddAkAudioRTPCTrack_Transaction", "Add AkAudioRTPC Track"));
		FocusedMovieScene->Modify();

		auto NewTrack = DoCreateWwiseGameParameterTrack.Execute(FocusedMovieScene);
		ensure(NewTrack);

		auto NewSection = NewTrack->CreateNewSection();
		ensure(NewSection);

		auto RTPCSection = Cast<UMovieSceneWwiseGameParameterSection>(NewSection);
		ensure(RTPCSection);
		if (Options.RTPC)
		{
			RTPCSection->SetGameParameter(Options.RTPC);
		}

		NewSection->SetRange(TRange<FFrameNumber>::All());
		NewTrack->AddSection(*NewSection);

		GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

void FMovieSceneWwiseGameParameterTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	auto CreateAkAudioRTPCTrack = [=](UMovieScene* MovieScene)
	{
#if UE_5_2_OR_LATER
		auto NewTrack = MovieScene->AddTrack<UMovieSceneWwiseGameParameterTrack>();
#else
		auto NewTrack = MovieScene->AddMasterTrack<UMovieSceneWwiseGameParameterTrack>();
#endif
		if (NewTrack != nullptr)
		{
			NewTrack->SetIsAMasterTrack(true);
		}
		return NewTrack;
	};

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddAkAudioGameParameterTrack", "GameParameter"),
		LOCTEXT("AddAkAudioGameParameterMasterTrackTooltip", "Adds a master Game Parameter track."),
		FSlateIcon(FAkAudioStyle::GetStyleSetName(), "Wwise.GameParameterIcon"),
		FUIAction(FExecuteAction::CreateLambda([this, CreateAkAudioRTPCTrack = MoveTemp(CreateAkAudioRTPCTrack)]
		{
			TryAddWwiseGameParameterTrack(FCreateWwiseGameParameterTrack::CreateLambda(CreateAkAudioRTPCTrack));
		}))
	);
}

bool FMovieSceneWwiseGameParameterTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	static UClass* LevelSequenceClass = UClass::TryFindTypeSlow<UClass>(TEXT("/Script/LevelSequence.LevelSequence"), EFindFirstObjectOptions::ExactClass);
	return InSequence != nullptr && LevelSequenceClass != nullptr && InSequence->GetClass()->IsChildOf(LevelSequenceClass);
}


void FMovieSceneWwiseGameParameterTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
	auto ObjectBinding = ObjectBindings[0];
	if (!ObjectClass->IsChildOf(AActor::StaticClass()) && !ObjectClass->IsChildOf(USceneComponent::StaticClass()))
	{
		return;
	}

	auto CreateAkAudioRTPCTrack = [=](UMovieScene* MovieScene) { return MovieScene->AddTrack<UMovieSceneWwiseGameParameterTrack>(ObjectBinding); };

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddWwiseGameParameterTrack", "WwiseGameParameter"),
		LOCTEXT("AddWwiseGameParameterTrackTooltip", "Adds a Game Parameter track."),
		FSlateIcon(FAkAudioStyle::GetStyleSetName(), "Wwise.GameParameterIcon"),
		FUIAction(FExecuteAction::CreateLambda([this, CreateAkAudioRTPCTrack = MoveTemp(CreateAkAudioRTPCTrack)]
		{
			TryAddWwiseGameParameterTrack(FCreateWwiseGameParameterTrack::CreateLambda(CreateAkAudioRTPCTrack));
		}))
	);
}

#undef LOCTEXT_NAMESPACE
