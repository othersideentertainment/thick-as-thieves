// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/Graph/OSEAnimTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimTypes)

DEFINE_LOG_CATEGORY(LogOSEAnimation);

FOSEAnimStateFlags::FOSEAnimStateFlags()
   : IsUnknown    (false)
   , IsWalking    (false)
   , IsFalling    (false)
   , IsFlying     (false)
   , IsSwimming   (false)
   , IsMantling   (false)
   , IsScrambling   (false)
{ }

FOSEAnimStateFlags::FOSEAnimStateFlags(EOSEAnimState inState)
   : IsUnknown    (inState == EOSEAnimState::Unknown)
   , IsWalking    (inState == EOSEAnimState::Walking)
   , IsFalling    (inState == EOSEAnimState::Falling)
   , IsFlying     (inState == EOSEAnimState::Flying)
   , IsSwimming   (inState == EOSEAnimState::Swimming)
   , IsMantling   (inState == EOSEAnimState::Mantling)
   , IsScrambling   (inState == EOSEAnimState::Scrambling)
{ }

