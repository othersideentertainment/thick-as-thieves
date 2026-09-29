// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

class AActor;

namespace TATCommonMapCheck
{
   // runs shared map-check code against this actor
   // Here so that it can be called by TATUnrealEdEngine (editor) and TATEditorEngine (commandlet)
   void CheckActor(const AActor* actor);

   // runs shared map-check code against this world
   // Here so that it can be called by TATUnrealEdEngine (editor) and TATEditorEngine (commandlet)
   void CheckWorld(UWorld* world);

};
