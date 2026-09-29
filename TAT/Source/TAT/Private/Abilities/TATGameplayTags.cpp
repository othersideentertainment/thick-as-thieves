// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Status_LockpickingDisabled, "Status.LockpickingDisabled");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_PickupInteractionDisabled, "Status.PickupInteractionDisabled");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Interact_NPC_FullHold, "Ability.Interact.NPC.FullHold");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Behavior_AlertnessTransition, "AI.Behavior.AlertnessTransition");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Health_Full, "Status.Health.Full", "The character has full health");

UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Alertness_Neutral, "AI.Alertness.Neutral")
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Alertness_Suspicious, "AI.Alertness.Suspicious")
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Alertness_Alerted, "AI.Alertness.Alerted")
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Alertness_Combat, "AI.Alertness.Combat")

UE_DEFINE_GAMEPLAY_TAG(TAG_Status_SuspiciousAction, "Status.SuspiciousAction")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_SuspiciousAction_Expired, "Status.SuspiciousAction.Expired")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AnimSet_Disguise, "AnimSet.Disguise", "AnimSet to use when disguised as this character")

UE_DEFINE_GAMEPLAY_TAG(TAG_AI_TargetingGroup_Projectile, "AI.Behavior.TargetingGroup.Projectile");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_IndividualKnowledge_InteractingWithActor, "IndividualKnowledge.InteractingWithActor", "This tag will cause the NPC to ignore stims from the instigator");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Detection_Disabled, "Status.Detection.Disabled", "This will disable detection entirely");
