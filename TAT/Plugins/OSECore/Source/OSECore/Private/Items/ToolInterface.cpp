// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolInterface)

DEFINE_LOG_CATEGORY(LogTools);

// Pure virtual default implementation. Derived classes can explicitly call this.
bool IToolInterface::IsReady() const { return false; }
bool IToolInterface::IsEquipped() const { return false; }
bool IToolInterface::IsStowed() const { return false; }

