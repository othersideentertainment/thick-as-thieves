// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"

// ue4

// self
#include "OSEStimDatabase.generated.h"

UENUM(BlueprintType, meta= (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EStimDatabaseQueryBitmaskValues : uint8
{
   None = 0 UMETA(Hidden),
   CheckLocation = 1 << 0,
   CheckInstigator = 1 << 1,
   CheckStimType = 1 << 2,
   CheckSeverity = 1 << 3,
   CheckTag = 1 << 4,
   CheckStrength = 1 << 5,
   CheckGlobalID = 1 << 6,
   CheckEverythingButGlobalID = CheckLocation | CheckInstigator | CheckStimType | CheckSeverity | CheckTag | CheckStrength  UMETA(Hidden),
   CheckEverything = CheckLocation | CheckInstigator | CheckStimType | CheckSeverity | CheckTag | CheckStrength | CheckGlobalID  UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EStimDatabaseQueryBitmaskValues);

USTRUCT(BlueprintType)
struct OSEAI_API FStimDatabaseQuery
{
   GENERATED_BODY()

   FStimDatabaseQuery()
   {
   };

#define GENERATE_REQUIRE_FUNCTION(type, fieldName, valueName, bitmaskValue)\
   void Require##fieldName(type val)\
   {\
      _##valueName = val;\
      _queryBitmask = _queryBitmask | static_cast<uint8>(bitmaskValue);\
   }

   GENERATE_REQUIRE_FUNCTION(const FVector&, Location, location, EStimDatabaseQueryBitmaskValues::CheckLocation)
   GENERATE_REQUIRE_FUNCTION(AActor*, Instigator, instigator, EStimDatabaseQueryBitmaskValues::CheckInstigator)
   GENERATE_REQUIRE_FUNCTION(const EStimType, StimType, stimType, EStimDatabaseQueryBitmaskValues::CheckStimType)
   GENERATE_REQUIRE_FUNCTION(const FName, Tag, tag, EStimDatabaseQueryBitmaskValues::CheckTag)
   GENERATE_REQUIRE_FUNCTION(const EStimSeverity, Severity, severity, EStimDatabaseQueryBitmaskValues::CheckSeverity)
   GENERATE_REQUIRE_FUNCTION(const float, Strength, strength, EStimDatabaseQueryBitmaskValues::CheckStrength)
   GENERATE_REQUIRE_FUNCTION(const int32, GlobalId, globalId, EStimDatabaseQueryBitmaskValues::CheckGlobalID)
#undef GENERATE_REQUIRE_FUNCTION

   bool QueryMatches(const FStimInfo& stimInfo) const
   {
      bool matches = true;
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckLocation))
      {
         matches &= FVector::PointsAreSame(_location, stimInfo.Location);         
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckInstigator))
      {
         matches &= _instigator == stimInfo.Instigator;         
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckStimType))
      {
         matches &= _stimType == stimInfo.Type;         
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckTag))
      {
         matches &= _tag == stimInfo.Tag;         
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckSeverity))
      {
         matches &= _severity == stimInfo.Severity;         
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(_queryBitmask), EStimDatabaseQueryBitmaskValues::CheckStrength))
      {
         matches &= _strength == stimInfo.Strength;         
      }
      // We always check the global ID, the game layer checks if the global ID flag is checked and returns the same one per instigator if needed.
      matches &= _globalId == stimInfo.GlobalId;         
      return matches;
   }
private:
   uint8 _queryBitmask { static_cast<uint8>(EStimDatabaseQueryBitmaskValues::None) };
   //TODO: Maybe a bitmask would be better for this?
   FVector _location { FVector::ZeroVector };   
   UPROPERTY()
   AActor* _instigator { nullptr };
   EStimType _stimType { EStimType::Audio };
   FName _tag { FName(NAME_None) };
   EStimSeverity _severity { EStimSeverity::None };
   float _strength { 0.f };
   int32 _globalId { INDEX_NONE };   
};

UCLASS(BlueprintType)
class OSEAI_API UOSEStimDatabase : public UObject
{
   GENERATED_BODY()

public:
   // tick should be called by the db owner
   void UpdateDatabase(float deltaTime, float now);

   // all stims
   const TArray<FStimInfo>& GetKnownStims() const { return _stims; }
   TArray<FStimInfo>& GetKnownStims() { return const_cast<TArray<FStimInfo>&>(const_cast<const UOSEStimDatabase*>(this)->GetKnownStims()); }

   // add
   FStimInfo& AddStimInfo(AActor* perceivedBy, const FVector& location, AActor* instigator, EStimType stimType, FName tag, EStimSeverity stimSeverity, float strength, int32 globalId = INDEX_NONE);
   void AddStimPerceivedActor(FStimInfo& stim, AActor* perceivedBy);

   // find by id
   const FStimInfo* FindStimInfo(int stimId) const;
   FStimInfo* FindStimInfo(int stimId) { return const_cast<FStimInfo*>(const_cast<const UOSEStimDatabase*>(this)->FindStimInfo(stimId)); }

   // find by info
   const FStimInfo* FindStimInfo(const FVector& location,
                                 AActor* instigator,
                                 const EStimType stimType,
                                 const FName tag,
                                 const EStimSeverity severity,
                                 const float strength,
                                 const int32 globalId = INDEX_NONE,
                                 const bool checkGlobalID = false) const
   {
      FStimDatabaseQuery query;
      query.RequireLocation(location);
      query.RequireInstigator(instigator);
      query.RequireStimType(stimType);
      query.RequireTag(tag);
      query.RequireSeverity(severity);
      query.RequireStrength(strength);
      if(checkGlobalID)
      {
         query.RequireGlobalId(globalId);
      }
      return FindStimInfo(query);
   }
   FStimInfo* FindStimInfo(const FVector& location,
                           AActor* instigator,
                           const EStimType stimType,
                           const FName tag,
                           const EStimSeverity severity,
                           const float strength,
                           const int32 globalId = INDEX_NONE,
                           const bool checkGlobalID = false)
   {
      return const_cast<FStimInfo*>(
         const_cast<const UOSEStimDatabase*>(this)->FindStimInfo(
         location,
         instigator,
         stimType,
         tag,
         severity,
         strength,
         globalId,
         checkGlobalID
      ));
   }

   const FStimInfo* FindStimInfo(const FStimDatabaseQuery& query) const;
   FStimInfo* FindStimInfo(const FStimDatabaseQuery& query) { return const_cast<FStimInfo*>(const_cast<const UOSEStimDatabase*>(this)->FindStimInfo(query)); }

   
   // newest stim
   const FStimInfo* FindNewestStim() const;

private:
   UPROPERTY(Transient)
   TArray<FStimInfo> _stims;

   uint32 _nextStimId = 0;
};
