// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

class UWorld;
class FMessageLog;

namespace TATQuestSpawnValidation
{
   void TryValidate(UWorld* world, FMessageLog& msgLog);
}
