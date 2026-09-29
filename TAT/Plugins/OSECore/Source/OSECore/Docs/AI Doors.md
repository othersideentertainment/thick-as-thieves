# AI Doors

## Purpose

Doors.. They are just doors and the AI needs to be able to open shut and move through them


## Requirements

- AI need to be able to path find through doors
- AI need to fail to path find through locked doors
- AI need to be able to discover that a door is locked
- AI need to be able to move through doors

## Implementation

- There are a few UE4 musts for this to work.
  - Navlink proxy.  ( container for all nav info )
  - NavlinkCustom component.  ( Adds callbacks for using and can path ( uquerier* ) functionality
- Knowledge component to store observable state changes 
- AI perception component to enable AI's to see when doors are ajar
- Smart object component to control using the door ( Adds ability to play custom smart object anims and behaviors)

## Questions
- How much should live on navlink vs the door?  Perhaps players don't need all this and could place on the link itself?  

