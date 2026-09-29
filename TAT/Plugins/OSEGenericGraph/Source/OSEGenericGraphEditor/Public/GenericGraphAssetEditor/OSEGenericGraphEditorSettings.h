// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEGenericGraphEditorSettings.generated.h"

UENUM(BlueprintType)
enum class EOSEGenericGraphAutoLayoutStrategy : uint8
{
   Tree,
   ForceDirected,
};

USTRUCT()
struct OSEGENERICGRAPHEDITOR_API FOSEGenericGraphEditorSettings
{
   GENERATED_BODY()

public:
   // If enabled, graph properties marked EditDefaultsOnly will be visible in the property editor
   UPROPERTY(EditDefaultsOnly, Category = "Graph Metadata")
   bool ShowAdvancedGraphProperties = false;

   UPROPERTY(EditDefaultsOnly, Category = "AutoArrange")
   float OptimalDistance = 100.f;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   EOSEGenericGraphAutoLayoutStrategy AutoLayoutStrategy = EOSEGenericGraphAutoLayoutStrategy::Tree;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   int32 MaxIteration = 50;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   bool bFirstPassOnly = false;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   bool bRandomInit = false;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   float InitTemperature = 10.f;

   UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
   float CoolDownRate = 10.f;
};
