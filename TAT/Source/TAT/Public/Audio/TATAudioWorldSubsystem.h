// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATWorldAudio.h"

// ue4
#include "CoreMinimal.h"

#include "TATAudioWorldSubsystem.generated.h"

class UTATWorldAudio;

UCLASS(Config = Game)
class TAT_API UTATAudioWorldSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   UTATAudioWorldSubsystem();

   // from UWorldSubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual void OnWorldBeginPlay(UWorld& world) override;
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;

   UFUNCTION(BlueprintPure, Category = "Audio|TAT")
   UTATWorldAudio* GetWorldAudio() const { return _worldAudio; }

private:
   UPROPERTY(Config)
   TSoftClassPtr<UTATWorldAudio> _worldAudioClass;

   UPROPERTY(Transient)
   TSubclassOf<UTATWorldAudio> _loadedWorldAudioClass;
   
   UPROPERTY(Transient)
   UTATWorldAudio* _worldAudio = nullptr;
};
