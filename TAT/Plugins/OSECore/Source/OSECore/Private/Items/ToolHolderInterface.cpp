// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolHolderInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolHolderInterface)

// Pure virtual default implementation. Derived classes can explicitly call this.
USceneComponent* IToolHolderInterface::GetToolRoot(EMeshPerspective MeshPerspective) const { return nullptr; }

