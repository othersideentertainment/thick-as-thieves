// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEAutoLayoutStrategy.h"
#include "OSEForceDirectedLayoutStrategy.generated.h"

class UOSEGenericGraphNode;

UCLASS()
class OSEGENERICGRAPHEDITOR_API UOSEForceDirectedLayoutStrategy : public UOSEAutoLayoutStrategy
{
   GENERATED_BODY()
public:
   UOSEForceDirectedLayoutStrategy();
   virtual ~UOSEForceDirectedLayoutStrategy();

   virtual void Layout(UEdGraph* EdGraph) override;

protected:
   virtual FBox2D LayoutOneTree(UOSEGenericGraphNode* RootNode, const FBox2D& PreTreeBound);

protected:
   bool bRandomInit;
   float InitTemperature;
   float CoolDownRate;
};
