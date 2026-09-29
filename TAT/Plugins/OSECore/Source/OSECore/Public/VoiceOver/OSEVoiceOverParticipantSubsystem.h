// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "OSEVoiceOverParticipantSubsystem.generated.h"


// A subsystem for registering possible particpants for a voice-over conversation
//
// A simple thing to replace an expensive collision query, so feel free to replace with something later if desired.
UCLASS()
class OSECORE_API UOSEVoiceOverParticipantSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()
   
 public:

    virtual bool ShouldCreateSubsystem(UObject* outer) const override;

    void RegisterParticipant(AActor* actor);
    void UnregisterParticipant(AActor* actor);

    void FindPossibleParticipantsInRange(const FVector& origin, float radius, const AActor* exclude, const TFunctionRef<void(AActor*)>& handler) const;

protected:

   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   TArray<TWeakObjectPtr<AActor>> _participants;
};
