# Interaction System

## Purpose

### _(What problem we are trying to solve, and why we're trying to solve it)_

Implement a generic, data-driven interaction system that can be used to allow the player to interact with items or other objects in the game world.

Possible use cases include:

* Picking up items
* Opening/closing doors/windows
* Toggling switches
* Picking locks

Even within these use cases, design may desire the ability to "be interacting" with an item for a certain amount of time, or use a certain amount of energy, to "finish" interacting with the item.

## Requirements

### _(Both technical and design requirements here. This directly informs the implementation approach, making sure all requirements are addressed)_

* Building Content With Interactables
  * Make sure interactables are easy to add to levels and create from scratch
  * Make sure interactables have all appropriate parameters exposed in a way that design can tweak behavior of BOTH:
    * The Player when Interacting
    * The Object being Interacted With
* Animation
  * Play an appropriate - and possibly unique - animation when player interacts with something.
  * Include IK when appropriate to guide the animation. For example, if we have a grasping animation, make sure IK is used to steer the hand to the location of the item rather than grasping empty air.
* Selection and Visuals
  * Design-adjustable range for players to to be able to interact with something in the world
  * Trace from current camera view to potential interactable items
  * Distinguish interactable items in the world with a highlight / outline / other visual effect when the player is able to interact with an item, and is selected
  * Option to also display context-sensitive UI prompt
* Technical
  * Full functionality in both C++ and Blueprints (so we can leverage them either way)
  * Input action (IsInteracting) network synced on the player, with appropriate animation blueprint accessible functions
  * Resulting interaction behavior network synced with server authority
  * Design-adjustable energy drain for certain interactable actions

## Implementation

### _(The steps and technical details to meet the requirements.  I also like to add rough "1d", "2d", etc estimates to these sections. This example is a rough outline; more detail would be added here as needed.)_

* Audit various UE4 approaches to implementing interactables, preferably UDN sources with Epic employee responses
* Implement C++ `UINTERFACE()` class for interactable objects
* Implement Blueprint version of C++ interface, derived from it to benefit from any code changes
* For player interaction:
  * C++ and BP event for `OnPlayerInteractStart`
  * C++ and BP event for `OnPlayerInteractStop`
  * C++/BP accessor for how long player has been interacting
  * Get current target interactable?
* For the object being interacted with:
  * C++/BP event for `OnTargetInteractStart`
  * C++/BP event for `OnTargetInteractStop`
  * C++/BP logic for "successful interaction"
  * Base-level implementation of network syncing to ensure all clients see the same behavior
  * Make sure the current interacted state of the object is appropriately reflected via the network for late joining / join in progress

## Open Questions

### _(Unanswered questions either from a design or technical side. This is a good place to put feedback from other engineers, or even say "we won't be addressing this until X")_

* What use cases are we missing?
* What other design-adjustable values will design want?
* How do we resolve overlapping interactable item priority?
* Ability to filter / prevent the player from interacting with the object based on conditions? For example, a mission objective object "becomes" interactable at the appropriate part of a quest, and now becomes highlightable
* What happens if the player moves away from interactable before req interaction time / energy expenditure?

