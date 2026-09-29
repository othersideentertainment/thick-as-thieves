// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Tests/EnvQueryTest_Attitude.h"

// ue
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"

// ose
#include "Character/OSETeamInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryTest_Attitude)

#define LOCTEXT_NAMESPACE "EnvQueryTest"

UEnvQueryTest_Attitude::UEnvQueryTest_Attitude()
{
   Cost = EEnvTestCost::Low;
   ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
   SetWorkOnFloatValues(false);

   desiredAttitude = EOSETeamAttitude::Hostile;
}

void UEnvQueryTest_Attitude::RunTest(FEnvQueryInstance& queryInstance) const
{
   AActor* queryActor = Cast<AActor>(queryInstance.Owner.Get());
   if (queryActor == nullptr)
   {
      return;
   }

   for (FEnvQueryInstance::ItemIterator it(this, queryInstance); it; ++it)
   {
      const AActor* itemActor = GetItemActor(queryInstance, it.GetIndex());
      const EOSETeamAttitude itemAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(queryActor, itemActor);
      const bool requiredValue = BoolValue.GetValue();
      const bool actualValue = itemAttitude == desiredAttitude;
      it.SetScore(TestPurpose, FilterType, requiredValue == actualValue, true);
   }
}

FText UEnvQueryTest_Attitude::GetDescriptionTitle() const
{
   return LOCTEXT("TeamAttitudeTest", "Team Attitude Test");
}

FText UEnvQueryTest_Attitude::GetDescriptionDetails() const
{
   UEnum* teamAttitudeEnum = StaticEnum<EOSETeamAttitude>();
   FString desiredAttitudeString = teamAttitudeEnum->GetDisplayNameTextByValue((int64)desiredAttitude).ToString();

   FFormatNamedArguments args;
   args.Add(TEXT("DesiredAttitude"), FText::FromString(desiredAttitudeString));

   FText desc = FText::Format(LOCTEXT("TeamAttitudeTest", "Desired Attitude: {DesiredAttitude}"), args);
   return desc;
}

#undef LOCTEXT_NAMESPACE
