// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue4
#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"

// self
#include "StimInfo.generated.h"

// How severe the stim is, heavy or light.
UENUM(BlueprintType)
enum class EStimSeverity : uint8
{
   None = 0,
   Light = 1,
   Heavy = 2,
};

// stim type
UENUM(BlueprintType)
enum class EStimType : uint8
{
   Audio,
   Visual,
   Physical,
   Damage,
};

// stim investigation state
UENUM(BlueprintType)
enum class EStimInvestigationState : uint8
{
   NotInvestigated,
   UnderInvestigation,
   Investigated,
};

USTRUCT(BlueprintType)
struct OSEAI_API FOSEStimSettings : public FTableRowBase
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, Category = "Stim")
   EStimSeverity Severity = EStimSeverity::Light;
};

USTRUCT(BlueprintType)
struct OSEAI_API FStimInfo
{
   GENERATED_BODY()

public:

   FStimInfo()
   {
   }

   FStimInfo(int32 id, const FVector& location, AActor* instigator, EStimType stimType, EStimSeverity severity, FName tag, float strength, float timestamp, int32 globalId = INDEX_NONE)
      : Id(id)
      , GlobalId(globalId)
      , Location(location)
      , Instigator(instigator)
      , Type(stimType)
      , Severity(severity)
      , Tag(tag)
      , Strength(strength)
      , Timestamp(timestamp)
   {
   }

   //---------------------------------------------------------------------------------------
   // Static
   //---------------------------------------------------------------------------------------
   
   static FStimInfo Invalid;

   //---------------------------------------------------------------------------------------
   // Stable data
   //---------------------------------------------------------------------------------------

   // Id local to the stim database that this stim is cached within.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   int32 Id = INDEX_NONE;

   // Optional global Id that can be used to match this stim, registered in one database
   // (by one AI) with that of another.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   int32 GlobalId = INDEX_NONE;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector Location = FAISystem::InvalidLocation;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TWeakObjectPtr<AActor> Instigator;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EStimType Type = EStimType::Audio;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EStimSeverity Severity = EStimSeverity::None;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FName Tag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float Strength = 0.0f;

   /// The in-game timestamp of when we perceived this stim.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float Timestamp = -1.0f;

   //---------------------------------------------------------------------------------------
   // Live data - can change after the stim enters the database
   //---------------------------------------------------------------------------------------

   /// Actors that perceived this stim
   TSet<TWeakObjectPtr<AActor>> PerceivedByActors;

   /// Actors that can attempt to respond to this stim
   TArray<TWeakObjectPtr<AActor>> ResponsiveActors;

   const AActor* GetNextResponsiveActor() const;
   void OnResponsiveActorCheckedForGoals(AActor* actor);

   /// Investigation state
   UPROPERTY(BlueprintReadOnly)
   EStimInvestigationState InvestigationState = EStimInvestigationState::NotInvestigated;

   /// Investigating actor
   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<AActor> InvestigatingActor;

   /// If set to false, this stim will not be allowed to be removed from the database
   bool CanBeForgotten = true;

   //---------------------------------------------------------------------------------------
   // Utility functions
   //---------------------------------------------------------------------------------------

   FString ToString() const;
   bool IsValid() const { return Id != INDEX_NONE; }
   
   //---------------------------------------------------------------------------------------
   // Comparisons
   //---------------------------------------------------------------------------------------
   
   FORCEINLINE bool operator==(const FStimInfo& other) const
   {
      return (Id == other.Id)
          && (GlobalId == other.GlobalId)
          && (Location == other.Location)
          && (Instigator == other.Instigator)
          && (Type == other.Type)
          && (Severity == other.Severity)
          && (Tag == other.Tag)
          && (Strength == other.Strength)
          && (Timestamp == other.Timestamp);
   }
   
   FORCEINLINE bool operator!=(const FStimInfo& other) const
   {
      return !operator==(other);
   }

   //---------------------------------------------------------------------------------------
   // Copy only stable data
   //---------------------------------------------------------------------------------------

   FORCEINLINE FStimInfo& operator=(const FStimInfo& other)
   {
      Id = other.Id;
      GlobalId = other.GlobalId;
      Location = other.Location;
      Instigator = other.Instigator;
      Type = other.Type;
      Severity = other.Severity;
      Tag = other.Tag;
      Strength = other.Strength;
      Timestamp = other.Timestamp;
      return *this;
   }
};
