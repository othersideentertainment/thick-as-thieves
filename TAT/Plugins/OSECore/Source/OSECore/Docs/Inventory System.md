# Inventory System

## Purpose

Implement a flexible inventory system to support a variety of use cases

## Requirements

### Defining Inventory and Items

* Add an **inventory** to any Actor (player, AI, "world" objects, etc)
* Create inventory **items** from any Actor
* Create default inventory **loadout**, configured per supported actor

### Run-time Access

* Support adding items at run-time
* Support removing items at run-time
* Support equipping / unequipping items
* Support querying items
  * Index in inventory?
  * Predefined types of items?

### Common Functionality

* Support most base functionality from Unity Prototype (`ItemObject.cs`)
* Built in tracing to target appropriate objects if the item requires it
* Energy drain 
* Max / remaining uses

### HUD and UI

* Easy access to the HUD if the inventory owner has one (for example, a possessed player pawn)
* Context-sensitive cursor
* UI integration to display currently selected tool, remaining uses, etc

### Network Support

* Fully networked and synced
* Server-owned and managed
* Responsive on clients

### Visuals

* Support 1st person representation of items
  * Attach to owners 1st person representation
  * Relative offset and rotation from attachment
* Support 3rd person representation of items
  * Attach to owners 1st person representation
  * Relative offset and rotation from attachment
* Support *no visual representation* for equipped items
* Support equip animations on both inventory owner and item
* Optional: Support unequip animations on both inventory owner and item

### Other Requirements

* Allow full implementation in C++
* Allow full implementation in Blueprints
* Allow mixed implementation

## Implementation

### Items

* Items must implement `IItemInterface`, a UE4 Interface type
  * Any Actor can be an item if this interface is supported
  * Interface can be inherited from C++, OR implemented in Blueprints
* Items must replicate across the network
* Items must use net owner relevancy
* Base `ItemActor` C++ class is an abstract actor that implements the interface in C++
  * Build full items off of this, or use as a sample
  * Useful to iron out division of responsibility among systems, adding support for common requirements, etc

### Inventory

* The actual inventory is an Actor Component
* Add component to an actor
* Define default inventory configuration per instance
* Server spawns default items on initialization
* Items owner is assigned to the owner of the Inventory Component
* Changes to inventory must be replicated to clients

## Open Questions / Issues

* Attachment relative offset / rotation not being respected
* Attachment breaks when unpossessing owner on client
* Attachment changes required tick order


