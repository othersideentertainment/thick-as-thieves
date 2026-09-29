// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"

#include "TATQuestRewardType.generated.h"

class UPaperSprite;
struct FTATQuestRewardContext;

UCLASS(Abstract)
class TAT_API UTATQuestRewardType : public UObject
{
   GENERATED_BODY()
   
public:

   virtual TSoftObjectPtr<UPaperSprite> GetIcon(const UObject* worldContext) const;
   virtual const FText& GetName(const UObject* worldContext) const;
   virtual void Grant(int32 quantity, const FTATQuestRewardContext& context) const {}
};
