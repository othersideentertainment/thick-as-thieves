// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "OSEVoiceOverLineRequestParams.generated.h"

class UOSEVoiceOverLine;

//---------------------------------------------------------------------------------------
/// FOSEVoiceOverLineRequestParams
///
/// A helper struct for external objects that can be used to feed calls to 
/// UOSEVoiceOverControllerComponent::AuthorityRequestVOLine.
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FOSEVoiceOverLineRequestParams
{
   GENERATED_BODY()

public:
   FOSEVoiceOverLineRequestParams() {}
   FOSEVoiceOverLineRequestParams(UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting, float timeInQueue = -1.0f);

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio")
   UOSEVoiceOverLine* Line = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio", meta = (Categories = "VoiceOver.Priority"))
   FGameplayTag PriorityTag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio")
   bool Interrupting = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio")
   float TimeInQueue = -1.0f;

   void AuthoritySubmitRequest(AActor* voiceActor) const;
};
