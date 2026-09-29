# Guard Behavior

## Purpose

Behavior for AI to run to move to and communicate information to other AI actors

## Design Reference



## Requirements

- Target AI to communicate with
- Dialogue model with info to communicate
- Condition to end behavior on successful communication
- Dialogue action to notify that info has been communicated

## Implementation

- Behavior tree with corresponding request type for communicate
- Reasoner behavior tree service that will grab dialogue model off request
- behavior tree logic for moving to other actor to communicate
- smart object search to find communication supporting smart objects ( guards talking smartobject etc)


