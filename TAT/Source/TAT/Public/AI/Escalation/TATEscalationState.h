// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "TATEscalationState.generated.h"
// Escalation States from the Escalation spec: https://otherside.atlassian.net/wiki/spaces/TVT/pages/3266543664/Guard+Awareness+Escalation+System
UENUM(BlueprintType)
enum class ETATEscalationState : uint8
{
   None UMETA(Hidden),
   Drowsy,
   Fresh,
   Mindful,
   Vigilant   
};
