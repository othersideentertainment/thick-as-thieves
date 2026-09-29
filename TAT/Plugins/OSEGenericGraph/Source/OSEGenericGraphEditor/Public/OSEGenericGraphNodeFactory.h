// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once
#include <EdGraphUtilities.h>
#include <EdGraph/EdGraphNode.h>

class FOSEGenericGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
   virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
