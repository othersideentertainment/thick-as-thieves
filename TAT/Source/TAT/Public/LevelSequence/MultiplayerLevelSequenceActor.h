// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "LevelSequenceActor.h"

#include "MultiplayerLevelSequenceActor.generated.h"

class UTATCinematicOverlayScreen;

USTRUCT(BlueprintType)
struct TAT_API FTATMultiplayerLevelSequenceCinematicOverlayParams
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   bool ShowCinematicOverlay = true;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   TSubclassOf<UTATCinematicOverlayScreen> OverlayScreenClass;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   bool AllowSequenceSkip = true;
};

USTRUCT(BlueprintType)
struct TAT_API FTATMultiplayerLevelSequenceCreateParams
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   ULevelSequence* LevelSequence = nullptr;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   FMovieSceneSequencePlaybackSettings PlaybackSettings;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   FLevelSequenceCameraSettings CameraSettings;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   FTATMultiplayerLevelSequenceCinematicOverlayParams CinematicOverlayParams;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   bool Replicate = true;
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
   bool DestroyWhenComplete = true;
   UPROPERTY()
   bool Configured = false;
};

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATMultiplayerLevelSequenceActor : public ALevelSequenceActor
{
   GENERATED_BODY()
   
public:   
   ATATMultiplayerLevelSequenceActor(const FObjectInitializer& init);

   // static
   UFUNCTION(BlueprintCallable, Category = "Game|Cinematic", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "OutActor"))
   static ULevelSequencePlayer* TATCreateLevelSequencePlayer(UObject* worldContextObject, const FTATMultiplayerLevelSequenceCreateParams& params, ATATMultiplayerLevelSequenceActor*& outActor);
   static ATATMultiplayerLevelSequenceActor* CreateLevelSequencePlayer(UObject* worldContextObject, const FTATMultiplayerLevelSequenceCreateParams& params);

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void Tick(float deltaSeconds) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

private:
   UFUNCTION()
   void _OnRep_LevelSequence();
   UFUNCTION()
   void _OnRep_Params();
   
   void _BindToOnSequenceFinished();
   void _TryInitializeLevelSequence();
   void _TryInitializeCinematicOverlay();
   void _TryCleanUpCinematicOverlay();

   void _AuthorityResetReadyForCutsceneChecks();
   void _AuthorityCheckForSequenceSkip();

   UFUNCTION()
   void _OnSequencePlayerFinished();

   bool _IsServer() const;

private:
   UPROPERTY(ReplicatedUsing = _OnRep_LevelSequence)
   ULevelSequence* _levelSequence;
   UPROPERTY(ReplicatedUsing = _OnRep_Params)
   FTATMultiplayerLevelSequenceCreateParams _params;
   bool _initialized = false;
   UPROPERTY(Transient)
   UTATCinematicOverlayScreen* _overlayScreen;
};
