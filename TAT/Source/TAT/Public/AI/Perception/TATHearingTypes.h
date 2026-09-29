// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Perception/OSEStimDatabase.h"
#include "Character/OSETeamInterface.h"

// ue5
#include "AI/Perception/StimInfo.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "TATHearingTypes.generated.h"

/// How does the hearing event propagate through the world
UENUM(BlueprintType)
enum class ETATHearingEventPropagationMethod : uint8
{
   // quiet noise, LOS will be validated before an AI hears this
   RequireDirectLOS = 0,

   // loud noise, all AI in range will hear it, even though walls
   CheckOnlyDistance = 1,

   // N.B. Not currently implemented
   // No LoS required for these to be heard. So functionally the same as CheckOnlyDistance
   PropogateThroughOpenPortals = 2,

   Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct TAT_API FTATHearingEventStimSettings : public FTableRowBase
{
   GENERATED_BODY()

public:
   FTATHearingEventStimSettings();

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim", meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag Tag;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   EStimSeverity Severity = EStimSeverity::Light;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   ETATHearingEventPropagationMethod VolumeLevel = ETATHearingEventPropagationMethod::RequireDirectLOS;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   FOSEAISenseAffiliationFilter HeardBy;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   float MaxRange = 1500.0f;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   float InnerInvestigationRange = {150.f};

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim")
   float OuterInvestigationRange = {800.f};

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stim", meta = (Categories = "AI.Behaviors"))
   FGameplayTagContainer Behaviors;

   UPROPERTY(EditAnywhere, Category = "Stim", meta = (DisplayName = "Stim Lookup Flags", Bitmask, BitmaskEnum = "/Script/OSEAI.EStimDatabaseQueryBitmaskValues"))
   uint32 QueryBitmask {
      static_cast<uint32>(
         EStimDatabaseQueryBitmaskValues::CheckEverything
         )
   };
   
   bool IsValid() const { return Tag.IsValid(); }
};

