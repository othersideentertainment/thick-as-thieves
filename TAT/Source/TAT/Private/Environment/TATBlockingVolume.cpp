// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATBlockingVolume.h"

// ue
#include "Components/BrushComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBlockingVolume)

// Sets default values
ATATBlockingVolume::ATATBlockingVolume()
{
/*
+---------------------+----------------+----------------------------------------------------+
|         Tag         |    Location    |                    Description                     |
+---------------------+----------------+----------------------------------------------------+
| Interact            | Component      | Opts the component in to be an interactable part   |
|                     |                | an interactable actor if hit by an interaction     |
|                     |                | trace, and also shows the interact highlight       |
| InteractNoHighlight | Component      | Opts the component in to be an interactable part   |
|                     |                | an interactable actor if hit by an interaction     |
|                     |                | trace, but is not highlighted                      |
| hotwire_notarget    | Actor          | Prevents the actor from being targeted by a wire   |
|                     |                | tool                                               |
| PopsWanderers       | Actor          | Wanderers destroy themselves on overlap with an    |
|                     |                | actor with this tag (possibly defunct)             |
| DestroyProjectiles  | Component      | Projectiles that hit a component with this tag     |
|                     |                | will be destroyed, and prevented from spawning     |
|                     |                | other actors. Use-case: Safe room barriers         |
| NoDamage            | Component      | Hits to a component with this tag will not count   |
|                     |                | as damage. Example: Wall sections that are part of |
|                     |                | a window actor.                                    |
| NotForClient        | Component      | Excludes component from client builds              |
| NotForServer        | Component      | Excludes component from server builds              |
| XrayExclude         | Mesh Component | Excludes a mesh component from the Xray render     |
|                     |                | pass                                               |
| XrayDepthOnly       | Mesh Component | Allows a mesh component to participate in Xray     |
|                     |                | depth testing so other primitives don't render on  |
|                     |                | top, while being excluded from the Xray render     |
|                     |                | pass itself                                        |
+---------------------+----------------+----------------------------------------------------+*/
   static const FName kNoTargetTag("hotwire_notarget");
   Tags.Add(kNoTargetTag);

   // from ABlockingVolume
   static const FName InvisibleWall_NAME(TEXT("InvisibleWall"));
   GetBrushComponent()->SetCanEverAffectNavigation(true);
   GetBrushComponent()->SetCollisionProfileName(InvisibleWall_NAME);
}

