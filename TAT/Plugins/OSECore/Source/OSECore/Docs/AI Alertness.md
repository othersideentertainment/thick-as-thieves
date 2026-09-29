# Alertness System

## Purpose

To track how "Alert" a guard is.  The alertness system will provide an enumerated value of an actors alertness and keep a history of how the actor became alerted. 
## Design Reference

- https://docs.google.com/document/d/1B-Cy3CxVPV4JHwkFtbhhGBJJHQ_vRet54NbJw8NJP-E/edit
- Alertness is basically threat Level in the doc.  Changing to Alertness because I plan to use threat levels to track how threatning a given actor is to a guard.  Hopefully can update design to match


## Requirements

- Enumerated discrete alertness levels
- Floating point alertness accumulator
- History of how we reached our level of alertness
- Blueprint interface for changing alertness
- C++ interface for changing alertness
- logic for detecting alertness changes
- PIE visible debug for Alertness

## Implementation

- Going to create a Generic reasoner that will run periodically over all known actors and will have operators to increase alertness if the criteria for being alerted is met.  ( This might get split to optimize so that rules for audio only run on audio actors etc.  Initially will be one mega ruleset. )
- Additionally calls will be added to script functions to slam alertness to given values.  If you script an actor to attack that will likely slam the actors alertness to fully alerted/threatened.

### Alert level triggers/stims

- Object changes.  Window open. door unlocked.  door open.  etc.
- Audio foot steps falling.  door opening etc.

#### Object changes more details 
- When an actors state changes so that AI should notice that it has changed.  The actor that changes will turn on an ai perception component so that AI's can see that actor.  In addition the observable knowledge of that actor will be updated to reflect that something has changed.  On the dectecting actor there will be inference rules on the generic knowlege reasoner that will evaluate if the observable has changed since last processed and if it has it will check what the alertness change should be.  

### Alert level history 
- Any change in the alertlevel will add a record to the alertness history.  This will be an array of entries on the actor tracking how that actor became alerted and when.  This array will be viewable in the UE4 editor for quick and easy debugging