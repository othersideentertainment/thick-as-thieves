// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATGameViewportClient.h"

// TAT
#include "Common/TATVersionEdition.h"
#include "Developer/TATProjectSettings.h"
#include "UI/Slate/TATWatermarkWidget.h"
#include "TATEnvironmentConfig.h"

// OSE
#include "Online/OSEGameState.h"
#include "Player/OSEPlayerState.h"

// UE
#include "Widgets/SCompoundWidget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameViewportClient)

class STATBuildVersionWidget : public SCompoundWidget
{
public:
   SLATE_BEGIN_ARGS(STATBuildVersionWidget) {}
   SLATE_END_ARGS()

   void Construct(const FArguments& _args)
   {


      FSlateFontInfo fontInfo = FCoreStyle::GetDefaultFontStyle("Italic", 18);
      FSlateColor color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

       ChildSlot
       [
           SNew(SBox)
           .HAlign(HAlign_Left)
           .VAlign(VAlign_Bottom)
           .Padding(FMargin(12.0f, 0.0f, 0.0f, 12.0f))
           [
               SNew(SHorizontalBox)

               + SHorizontalBox::Slot()
               .AutoWidth()
               .Padding(0.0f, 0.0f, 16.0f, 0.0f)
               [
                   SNew(STextBlock)
                   .Text(FText::FromString(FString::Printf(TEXT("Build Version: %s"), *UTATVersion::GetBuildVersionString())))
                   .ColorAndOpacity(color)
                   .Font(fontInfo)
                   .ShadowOffset(FVector2D(1.0f, 1.0f))
                   .ShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.5f))
               ]

               + SHorizontalBox::Slot()
               .AutoWidth()
               .Padding(0.0f, 0.0f, 16.0f, 0.0f)
               [
                   SNew(STextBlock)
                   .Text(FText::FromString(FString::Printf(TEXT("Build CL: %d"), UTATVersion::GetBuildChangelistNumber())))
                   .ColorAndOpacity(color)
                   .Font(fontInfo)
                   .ShadowOffset(FVector2D(1.0f, 1.0f))
                   .ShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.5f))
               ]

               + SHorizontalBox::Slot()
               .AutoWidth()
               .Padding(0.0f, 0.0f, 16.0f, 0.0f)
               [
                   SNew(STextBlock)
                   .Text(FText::FromString(FString::Printf(TEXT("Build Date: %s"), *UTATVersion::GetBuildDate())))
                   .ColorAndOpacity(color)
                   .Font(fontInfo)
                   .ShadowOffset(FVector2D(1.0f, 1.0f))
                   .ShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.5f))
               ]
              
              + SHorizontalBox::Slot()
              .AutoWidth()
               [
                   SNew(STextBlock)
                   .Visibility(TATEnvironmentConfig::ShouldShowEnvironmentName() ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                   .Text(FText::FromString(FString::Printf(TEXT("Env: %s"), *TATEnvironmentConfig::GetEnvironmentName())))
                   .ColorAndOpacity(color)
                   .Font(fontInfo)
                   .ShadowOffset(FVector2D(1.0f, 1.0f))
                   .ShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.5f))
               ]
           ]
       ];

      SetVisibility(EVisibility::HitTestInvisible);
   }
};

void UTATGameViewportClient::Activated(FViewport* InViewport, const FWindowActivateEvent& InActivateEvent)
{
   if (auto* const oseGameState = AOSEGameState::GetOSEGameState(GetWorld()))
   {
      _OnGameStateSet(oseGameState);
   }
   else
   {
      GetWorld()->GameStateSetEvent.AddUObject(this, &UTATGameViewportClient::_OnGameStateSet);
   }


   TSharedPtr<STATBuildVersionWidget> versionWidget = SNew(STATBuildVersionWidget);
   AddViewportWidgetContent(versionWidget.ToSharedRef(), MAX_int32);

}

void UTATGameViewportClient::ShowWatermark(bool bShow)
{
   if (bShow)
   {
      if (!_watermarkWidget.IsValid())
      {
         _watermarkWidget = SNew(STATWatermarkWidget)
            .Username(FText::FromString(_playerName))
            .UniqueId(FText::FromString(_playerId));
         AddViewportWidgetContent(_watermarkWidget.ToSharedRef(), MAX_int32);
      }

      _watermarkWidget->SetVisibility(EVisibility::HitTestInvisible);
   }
   else
   {
      if (!_watermarkWidget.IsValid())
      {
         return;
      }
      _watermarkWidget->SetVisibility(EVisibility::Collapsed);
   }
}

void UTATGameViewportClient::_OnLocalPlayerStateAdded(AOSEPlayerState* _ps)
{
   _playerName = _ps->GetPlayerName();
   _playerId = _ps->GetUniqueId().ToString();

   if (!_watermarkWidget.IsValid())
   {
#if TAT_SHOW_WATERMARK
      const bool bShowWatermark = true;
#else
      const bool bShowWatermark = UTATProjectSettings::GetTATSettings()->ShowWatermark;
#endif
      ShowWatermark(bShowWatermark);
   }
}

void UTATGameViewportClient::_OnGameStateSet(AGameStateBase* gameState)
{
   if (auto* const oseGameState = Cast<AOSEGameState>(gameState))
   {
      for (auto* const osePlayerState : oseGameState->GetOSEPlayerStates())
      {
         if (osePlayerState->IsLocalPlayerState())
         {
            _OnLocalPlayerStateAdded(osePlayerState);
            break;
         }
      }

      if (_playerName.IsEmpty())
      {
         oseGameState->OnLocalPlayerStateAdded.AddUniqueDynamic(this, &UTATGameViewportClient::_OnLocalPlayerStateAdded);
      }
      else
      {
#if TAT_SHOW_WATERMARK
         const bool bShowWatermark = true;
#else
         const bool bShowWatermark = UTATProjectSettings::GetTATSettings()->ShowWatermark;
#endif
         ShowWatermark(bShowWatermark);
      }
   }
   else
   {
      // LOG
   }
}
