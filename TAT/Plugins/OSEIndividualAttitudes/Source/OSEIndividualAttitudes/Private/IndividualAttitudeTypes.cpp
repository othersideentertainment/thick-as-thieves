// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "IndividualAttitudeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IndividualAttitudeTypes)

void FIndividualAttitude::SetTarget(AActor* target)
{
   _Target = target;
}

bool FIndividualAttitude::MatchesTarget(const AActor* target) const
{
   return _Target == target;
}

bool FIndividualAttitude::Copy(const FIndividualAttitude& attitude,
                               const EOSEAttitudeCopyRules& attitudeCopyRules,
                               const EOSEExpirationTimeCopyRules& expirationTimeCopyRules)
{
   bool changed = false;
   auto setAttitude = [](bool& hasChangedValue, EOSEIndividualAttitude& attitudeToChange, const EOSEIndividualAttitude valueToUse)
   {
      hasChangedValue = true;
      attitudeToChange = valueToUse;
   };
   auto setExpirationTime = [](bool& hasChangedValue, float& timeToChange, const float valueToUse)
   {
      hasChangedValue = true;
      timeToChange = valueToUse;
   };
   if(attitudeCopyRules != EOSEAttitudeCopyRules::Ignore && _Attitude != attitude._Attitude)
   {
      if (attitudeCopyRules == EOSEAttitudeCopyRules::PreferLeastHostile &&
         (_Attitude == EOSEIndividualAttitude::Unknown || _Attitude < attitude._Attitude))
      {
         setAttitude(changed, _Attitude, attitude._Attitude);
      }
      if (attitudeCopyRules == EOSEAttitudeCopyRules::PreferMostHostile &&
         (_Attitude == EOSEIndividualAttitude::Unknown || _Attitude > attitude._Attitude))
      {
         setAttitude(changed, _Attitude, attitude._Attitude);
      }
      if(attitudeCopyRules == EOSEAttitudeCopyRules::Overwrite)
      {
         setAttitude(changed, _Attitude, attitude._Attitude);
      }
   }
   if(expirationTimeCopyRules != EOSEExpirationTimeCopyRules::Ignore &&
      FMath::IsNearlyEqual(_WorldTimeToRemoveAttitude, attitude._WorldTimeToRemoveAttitude) == false)
   {
      if (expirationTimeCopyRules == EOSEExpirationTimeCopyRules::PreferGreater &&
         (FMath::IsNearlyEqual(_WorldTimeToRemoveAttitude, static_cast<float>(INDEX_NONE)) || _Attitude < attitude._Attitude))
      {
         setExpirationTime(changed, _WorldTimeToRemoveAttitude, attitude._WorldTimeToRemoveAttitude);
      }
      if (expirationTimeCopyRules == EOSEExpirationTimeCopyRules::PreferLower &&
         (_Attitude == EOSEIndividualAttitude::Unknown || _Attitude > attitude._Attitude))
      {
         setExpirationTime(changed, _WorldTimeToRemoveAttitude, attitude._WorldTimeToRemoveAttitude);
      }
      if(expirationTimeCopyRules == EOSEExpirationTimeCopyRules::Overwrite)
      {
         setExpirationTime(changed, _WorldTimeToRemoveAttitude, attitude._WorldTimeToRemoveAttitude);
      }
   }
   return changed;
}

EOSEIndividualAttitude FIndividualAttitude::GetAttitude() const
{
   return _Attitude;
}

void FIndividualAttitude::SetAttitude(const EOSEIndividualAttitude attitude)
{
   _Attitude = attitude;
}

void FIndividualAttitude::SetWorldTimeToRemove(const float worldTimeToRemoveAttitude)
{
   _WorldTimeToRemoveAttitude = worldTimeToRemoveAttitude;
}

bool FIndividualAttitude::IsExpired(const float worldTime) const
{
   if(_WorldTimeToRemoveAttitude >= 0)
      return _WorldTimeToRemoveAttitude <= worldTime;
   return false;
}
