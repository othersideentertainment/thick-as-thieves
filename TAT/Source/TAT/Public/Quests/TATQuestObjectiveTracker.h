// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

#include "TATQuestObjectiveTracker.generated.h"

class UTATPlayerObjective;
struct FMatchPersistentData;

struct FTATObjectiveTrackerParams
{
   virtual ~FTATObjectiveTrackerParams() = default;

#if DO_CHECK
   virtual const TCHAR* GetTypeName() const { return TEXT("NONE"); };
#endif
};

// Cheapo RTTI for checked casts
#if DO_CHECK
#define TAT_DEFINE_TRACKER_PARAMS(name) \
   inline static const TCHAR* kTypeName = TEXT(#name); \
   virtual const TCHAR* GetTypeName() const override { return kTypeName; }
#else
#define TAT_DEFINE_TRACKER_PARAMS(name)
#endif

// just for params to Initialize
struct FTATObjectiveTrackerContext
{
   APlayerState* PlayerState = nullptr;
   const FTATObjectiveTrackerParams* Params = nullptr;
   FGameplayTag QuestTag;
   TConstArrayView<TObjectPtr<UTATPlayerObjective>> ChildObjectives;

   template <typename T>
   const T* GetParams() const
   {
      check(Params);

#if DO_CHECK
      // NB: Using pointer equality here, because it should be identical. The string contents are just for diagnostics
      checkf(Params->GetTypeName() == T::kTypeName, TEXT("Tracker param types do not match: %s != %s"), Params->GetTypeName(), T::kTypeName);
#endif

      return static_cast<const T*>(Params);
   }
};


// Replicated state for the tracking of a quest objective
USTRUCT()
struct FTATQuestObjectiveState
{
   GENERATED_BODY()

   // Whether it is completed or
   UPROPERTY()
   bool IsComplete = false;

   // Optional partial progress
   // Meaning depends on the objective type
   // Only for display purposes
   UPROPERTY()
   int32 Progress = 0;

   friend bool operator==(FTATQuestObjectiveState, FTATQuestObjectiveState) = default;
};

// Class for tracking an objective state
// * Only exists on authority
// * Might be a use-case for blueprints, but will leave that to subclasses/later
//   no strong need for the external methods to be BP-ed
UCLASS(Abstract)
class TAT_API UTATQuestObjectiveTracker : public UObject
{
   GENERATED_BODY()

public:

   virtual void Initialize(const FTATObjectiveTrackerContext& context) {}
   bool IsComplete() const { return _state.IsComplete; }
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const;
   virtual void CheatComplete();

   DECLARE_DELEGATE_OneParam(FOnProgressChanged, const FTATQuestObjectiveState&)
   FOnProgressChanged OnProgressChanged;

protected:
   void _SetIsComplete(bool complete);
   void _SetProgress(FTATQuestObjectiveState newProgress);

private:
   bool _firstStateUpdate = true;
   FTATQuestObjectiveState _state;
};
