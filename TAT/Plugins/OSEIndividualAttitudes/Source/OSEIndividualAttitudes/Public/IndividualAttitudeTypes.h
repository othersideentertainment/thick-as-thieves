// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
#include "Net/Serialization/FastArraySerializer.h"

#include "IndividualAttitudeTypes.generated.h"

class UOSEIndividualAttitudeComponent;

UENUM()
enum class EOSEIndividualAttitude : uint8
{
   Unknown,
   Hostile,
   Neutral,
   Friendly
};

UENUM()
enum class EOSEAttitudeCopyRules : uint8
{
   Ignore,
   PreferLeastHostile,
   Overwrite,
   PreferMostHostile,
};

UENUM()
enum class EOSEExpirationTimeCopyRules : uint8
{
   Ignore,
   PreferLower,
   Overwrite,
   PreferGreater,
};

USTRUCT()
struct OSEINDIVIDUALATTITUDES_API FIndividualAttitude
{
   GENERATED_BODY()
public:
   
   void SetTarget(AActor* target);
   bool MatchesTarget(const AActor* target) const;

   // returns true if anything was copied
   bool Copy(const FIndividualAttitude& attitude,
             const EOSEAttitudeCopyRules& attitudeCopyRules = EOSEAttitudeCopyRules::Overwrite,
             const EOSEExpirationTimeCopyRules& expirationTimeCopyRules = EOSEExpirationTimeCopyRules::Overwrite);
   
   EOSEIndividualAttitude GetAttitude() const;
   void SetAttitude(EOSEIndividualAttitude attitude);

   void SetWorldTimeToRemove(float worldTimeToRemoveAttitude);
   float GetWorldTimeToRemove() const { return _WorldTimeToRemoveAttitude; }
   
   bool IsExpired(float worldTime) const;   
   AActor* GetTarget() const { return _Target.Get(); }
private:
   UPROPERTY()
   TWeakObjectPtr<AActor> _Target { nullptr };

   UPROPERTY()
   float _WorldTimeToRemoveAttitude { INDEX_NONE };

   UPROPERTY()
   EOSEIndividualAttitude _Attitude { EOSEIndividualAttitude::Unknown };
};
